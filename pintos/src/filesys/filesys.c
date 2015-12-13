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
struct dir* get_dir (const char *path, bool to_remove, bool chdir);
char* get_filename (const char *path);
bool path_is_dir (const char *path);

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
  struct dir *dir = get_dir (name, true, false);
  char *file_name = get_filename(name);
  bool success = (dir != NULL
              && free_map_allocate (1, &inode_sector));
  if (is_dir)
    {
      block_sector_t parent_sector = inode_get_inumber(dir_get_inode(dir));
      success = success && dir_create (parent_sector, inode_sector, 16); // TODO: change 16 to initial_size when extensible files are done
    }
  else
    {
      success = success && inode_create (inode_sector, initial_size, is_dir);
    }
  success = success && dir_add (dir, file_name, inode_sector);
  inode_add_parent(inode_sector, inode_get_inumber(dir_get_inode(dir)));
  struct inode *inode = inode_open(inode_sector);
  inode_close(inode);
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);
  free(file_name);
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
  struct dir *dir = get_dir (name, false, false);
  if (!strcmp(name, "/") || path_is_dir(name)) {
    return dir;
  }
  char *file_name = "";
  if (strcmp(name, ""))
    file_name = get_filename(name);
  struct inode *inode = NULL;
  if (dir != NULL)
    dir_lookup (dir, file_name, &inode);
  dir_close (dir);

  if (strcmp(name, ""))
    free(file_name);
  if (!inode) {
    return NULL;
  }
  return file_open(inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir;
  struct thread *current = thread_current();
  if (current->cwd) {
    dir = get_dir (name, false, false);
    if (inode_get_inumber(dir_get_inode(current->cwd)) == inode_get_inumber(dir_get_inode(dir))) {
      dir_close(dir);
      return false;
    }
    else if (inode_still_open(dir_get_inode(dir)))
      return false;
  }
  dir = get_dir (name, true, false);
  char *file_name = get_filename(name);
  bool success = dir != NULL && dir_remove (dir, file_name);
  if (path_is_dir(name))
    dir_close (dir);
  else
    file_close(dir);
  free(file_name);
  return success;
}

/* Changes the current working directory of the process to dir, which may be
    relative or absolute. Returns true if successful, false on failure. */
bool
filesys_chdir (const char *name)
{
  struct dir *dir = get_dir (name, false, true);
  if (!dir)
    return false;
  struct thread *current = thread_current();
  dir_close(current->cwd);
  current->cwd = dir;
  return true;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

struct dir*
get_dir (const char *path, bool to_remove, bool chdir)
{
  if (!strcmp(path, "/")) {
    return dir_open_root ();
  }

  struct inode *inode;
  struct dir *dir;
  struct thread *current = thread_current ();
  char path_copy[strlen(path) + 1];
  memcpy(path_copy, path, strlen(path) + 1);
  char *saveptr, *token, *next_token;
  if (*path_copy == '/' || !current->cwd) {
    dir = dir_open_root ();
  }
  else {
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
          if (to_remove && next_token == NULL) {
            inode_close(inode);
            break;
          }
          else if (inode_is_dir(inode))
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
        if (chdir)
          {
            dir_close(dir);
            return NULL;
          }
        break;
      }
    }
  return dir;
}

char*
get_filename (const char *path)
{
  char *path_copy = malloc(strlen(path) + 1);
  memcpy(path_copy, path, strlen(path) + 1);
  char *saveptr;
  char *token = strtok_r (path_copy, "/", &saveptr);
  char *next_token = strtok_r (NULL, "/", &saveptr);
  struct dir *dir;
  struct inode *inode;

  if (!strcmp(path, "/")) {
    return path_copy;
  }

  while (next_token != NULL) {
    token = next_token;
    next_token = strtok_r (NULL, "/", &saveptr);
  }

  if (!token)
    token = "";
  char *file_name = malloc(strlen(token) + 1);
  memcpy(file_name, token, strlen(token) + 1);
  free(path_copy);
  return file_name;
}

bool
path_is_dir (const char *path)
{
  if (!strcmp(path, ""))
    return false;
  if (!strcmp(path, "/"))
    return true;

  struct inode *inode;
  struct dir *dir;
  struct thread *current = thread_current ();
  char path_copy[strlen(path) + 1];
  memcpy(path_copy, path, strlen(path) + 1);
  char *saveptr, *token;

  if (*path_copy == '/' || !current->cwd) {
    dir = dir_open_root ();
  }
  else {
    dir = dir_reopen (current->cwd);
  }
  for (token = strtok_r (path_copy, "/", &saveptr); token != NULL; token = strtok_r (NULL, "/", &saveptr))
    {
      if (!dir) {
        return false;
      }
      if (dir_lookup (dir, token, &inode))
        {
          if (inode_is_dir(inode))
            {
              dir_close (dir);
              dir = dir_open (inode);
            }
          else {
            inode_close(inode);
            inode_close(dir);
            return false;
          }
        }
      else {
        dir_close(dir);
        inode_close(inode);
        return false;
      }
    }
  dir_close(dir);
  inode_close(inode);
  if (!token)
    return true;
  return false;
}
