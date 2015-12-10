#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
struct dir* get_dir (const char *path);
char* get_filename (const char *path);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();
  cache_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  cache_flush_all ();
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir)
{
  block_sector_t inode_sector = 0;
  struct dir *dir = get_dir (name);
  // if (inode_is_dir(file_get_inode(dir)))
    // dir = dir_open(file_get_inode(dir));
  name = get_filename(name);
  // if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) TODO: not sure if need
  bool success = (dir != NULL
              && free_map_allocate (1, &inode_sector));
  // printf("in filesys_create, name: %s, success: %d\n", name, success);
  if (is_dir)
    success = success && dir_create (inode_sector, 16); // TODO: change 16 to initial_size when extensible files are done
  else
    success = success && inode_create (inode_sector, initial_size, is_dir);
  // printf("in filesys_create, after dir/inode create, success: %d\n", success);
  success = success && dir_add (dir, name, inode_sector);
  // printf("in filesys_create, after dir_add, success: %d\n", success);
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);
  free(name);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  // printf("start: in filesys open name: %s\n", name);
  struct dir *dir = get_dir (name);
  name = get_filename(name);
  struct inode *inode = NULL;
  // printf("middle: in filesys open name: %s\n", name);
  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);
  free(name);
  if (!inode) {
    return NULL;
  }
  // printf("in filesys_open, about to return opened file\n");
  if (!inode_is_dir(dir_get_inode(dir)))
    return file_open(inode);
  return dir_open(inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = get_dir (name);
  name = get_filename(name);
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);
  free(name);
  return success;
}

/* Changes the current working directory of the process to dir, which may be
    relative or absolute. Returns true if successful, false on failure. */
bool
filesys_chdir (const char *name)
{
  // printf("start of filesys chdir, name: %s\n", name);
  // struct dir *dir = get_dir (name);
  // if (!dir)
  //   return false;
  // struct thread *current = thread_current();
  // dir_close(current->cwd);
  // current->cwd = dir;
  // return true;

  struct dir *dir = get_dir (name);
    name = get_filename(name);
    struct inode *inode = NULL;
    if (dir != NULL)
      dir_lookup (dir, name, &inode);
    dir_close(dir);
    free(name);
    dir = dir_open(inode);
    if (dir)
      {
        struct thread *current = thread_current();
        dir_close(current->cwd);
        current->cwd = dir;
        return true;
      }
    return false;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

struct dir*
get_dir (const char *path)
{
  if (strcmp(path, "/") == 0) {
    printf("in get dir just opening root\n");
    return dir_open_root ();
  }

  struct inode *inode;
  struct dir *dir;
  struct thread *current = thread_current ();
  char path_copy[strlen(path) + 1];
  memcpy(path_copy, path, strlen(path) + 1);
  char *saveptr, *token, *next_token;

  if (strcmp(&path_copy[0], "/") == 0 || !current->cwd) {
    // printf("setting dir to root\n");
    dir = dir_open_root ();
  }
  else {
    // printf("reopening cwd as dir\n");
    dir = dir_reopen (current->cwd);
  }

  for (token = strtok_r (path_copy, "/", &saveptr); token != NULL; token = next_token)
    {
      next_token = strtok_r (NULL, "/", &saveptr);
      if (!dir) {
        return NULL;
      }
      if (dir_lookup (dir, token, &inode))
        {
          if (inode_is_dir(inode) && next_token != NULL) //(strcmp(token, get_filename(path)) != 0)
            {
              dir_close (dir);
              dir = dir_open (inode);
            }
          else {
            inode_close(inode);
            break;
          }
        }
      else {
        break;
      }
    }
  return dir;
}

char*
get_filename (const char *path)
{
  char path_copy[strlen(path) + 1];
  memcpy(path_copy, path, strlen(path) + 1);
  char *saveptr;
  char *token = strtok_r (path_copy, "/", &saveptr);
  char *next_token = strtok_r (NULL, "/", &saveptr);

  if (strcmp(path, "/") == 0)
    return *path_copy;

  while (next_token != NULL) {
    token = next_token;
    next_token = strtok_r (NULL, "/", &saveptr);
  }

  if (!token)
    token = "";
  char *file_name = malloc(strlen(token) + 1);
  memcpy(file_name, token, strlen(token) + 1);
  return file_name;
}
