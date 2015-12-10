#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "filesys/inode.h"

#define MIN_VALID_VADDR ((void *) 0x08048000)

static void syscall_handler (struct intr_frame *);
void check_valid_pointer (const void *vaddr);

/* Lock for files. */
struct lock file_lock;

void
syscall_init (void)
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
	check_valid_pointer((const void *)f->esp);
	uint32_t* args = ((uint32_t*) f->esp);
	switch (*(int *)f->esp)
		{
			case SYS_HALT:
				{
					halt();
					break;
				}
			case SYS_EXIT:
				{
					exit(args[1]);
					break;
				}
			case SYS_EXEC:
				{
					check_valid_pointer((const void *)args[1]);
					args[1] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[1]);
					f->eax = exec((const char *)args[1]);
					break;
				}
			case SYS_WAIT:
				{
					f->eax = wait(args[1]);
					break;
				}
			case SYS_CREATE:
				{
					check_valid_pointer((const void *)args[1]);
					args[1] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[1]);
					f->eax = create((const char *)args[1], args[2]);
					break;
				}
			case SYS_REMOVE:
				{
					check_valid_pointer((const void *)args[1]);
					args[1] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[1]);
					f->eax = remove((const char *)args[1]);
					break;
				}
			case SYS_OPEN:
				{
					check_valid_pointer((const void *)args[1]);
					args[1] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[1]);
					f->eax = open((const char *)args[1]);
					break;
				}
			case SYS_FILESIZE:
				{
					f->eax = filesize(args[1]);
					break;
				}
			case SYS_READ:
				{
					check_valid_pointer((const void *)args[2]);
					check_valid_pointer((const void *)args[2] + args[3]);
					args[2] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[2]);
					f->eax = read(args[1], (void *)args[2], args[3]);
					break;
				}
			case SYS_WRITE:
				{
					check_valid_pointer((const void *)args[2]);
					check_valid_pointer((const void *)args[2] + args[3]);
					args[2] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[2]);
					f->eax = write(args[1], (void *)args[2], args[3]);
					break;
				}
			case SYS_SEEK:
				{
					seek(args[1], args[2]);
					break;
				}
			case SYS_TELL:
				{
					f->eax = tell(args[1]);
					break;
				}
			case SYS_CLOSE:
				{
					close(args[1]);
					break;
				}
			case SYS_PRACTICE:
				{
					f->eax = practice(args[1]);
					break;
				}
			case SYS_CHDIR: // Change the current directory.
				{
					check_valid_pointer((const void *)args[1]);
					args[1] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[1]);
					f->eax = chdir((const char *)args[1]);
					break;
				}
			case SYS_MKDIR: // Create a directory.
				{
					check_valid_pointer((const void *)args[1]);
					args[1] = (int)pagedir_get_page(thread_current()->pagedir,
													(const void *)args[1]);
					f->eax = mkdir((const char *)args[1]);
					break;
				}
			case SYS_READDIR: // Reads a directory entry.
				{

				}
			case SYS_ISDIR: // Tests if a fd represents a directory.
				{
					f->eax = isdir(args[1]);
					break;
				}
			case SYS_INUMBER: // Returns the inode number for a fd.
				{

				}
		}
}

/* Checks whether the pointer is in valid userspace. */
void
check_valid_pointer (const void *vaddr)
{
	if (vaddr < MIN_VALID_VADDR || !is_user_vaddr(vaddr))
		exit(-1);
}

void
halt (void)
{
	shutdown_power_off();
}

void
exit (int status)
{
	if (status < -1)
		status = -1;
	struct thread *current = thread_current();
	lock_acquire(&file_lock);
	if (current->child)
		current->child->exit_status = status;
	lock_release(&file_lock);
	printf("%s: exit(%d)\n", current->name, status);
	thread_exit();
}

pid_t
exec (const char *file)
{
	if (!file)
		return -1;
	lock_acquire(&file_lock);
	char *state;
	char *fn_copy;
	fn_copy = palloc_get_page (0);
	if (fn_copy == NULL)
	  return TID_ERROR;
	strlcpy (fn_copy, file, PGSIZE);
	fn_copy = strtok_r(fn_copy, " ", &state);
	struct file *open_file = filesys_open(fn_copy);
	if (!open_file)
		{
			palloc_free_page (fn_copy);
			file_close(open_file);
			lock_release(&file_lock);
		  return -1;
		}
	palloc_free_page (fn_copy);
	file_close(open_file);
	int pid = process_execute(file);
	lock_release(&file_lock);
	return pid;
}

int
wait (pid_t pid)
{
	if (pid < 0)
		return -1;
	return process_wait(pid);
}

bool
create (const char *file, unsigned initial_size)
{
	if (!file)
		exit(-1);
	lock_acquire(&file_lock);
	bool success = filesys_create(file, initial_size, false);
	lock_release(&file_lock);
	return success;
}

bool
remove (const char *file)
{
	if (!file)
		exit(-1);
	lock_acquire(&file_lock);
	bool success = filesys_remove(file);
	lock_release(&file_lock);
	return success;
}

int
open (const char *file)
{
	if (!file)
		exit(-1);
	lock_acquire(&file_lock);
	struct file *new_file = filesys_open(file);
	if (!new_file)
		{
			lock_release(&file_lock);
			return -1;
		}
	struct thread *current = thread_current();
	int i;
	for (i = 2; i < 128; i++)
		{
			if (!current->file_des[i])
				{
					current->file_des[i] = new_file;
					lock_release(&file_lock);
					return i;
				}
		}
	lock_release(&file_lock);
	return -1;
}

int
filesize (int fd)
{
	if (fd < 128 && fd >1)
		{
			lock_acquire(&file_lock);
			struct file *file = thread_current()->file_des[fd];
			if (!file)
				{
					lock_release(&file_lock);
					return -1;
				}
			int filesize = file_length(file);
			lock_release(&file_lock);
			return filesize;
		}
	else
		return -1;
}

int
read (int fd, void *buffer, unsigned length)
{
	if (!buffer)
		return -1;
	else if (fd == STDIN_FILENO)
		return length;
	else if (fd > STDOUT_FILENO && fd < 128)
		{
			lock_acquire(&file_lock);
			struct thread *current = thread_current();
			struct file *file = current->file_des[fd];
			if (!file)
				{
					lock_release(&file_lock);
					return -1;
				}
			struct inode *inode = file_get_inode (file);
			if (!inode)
				{
					lock_release(&file_lock);
					return -1;
				}
			bool is_dir = inode_is_dir(inode);
			if (is_dir)
				{
					lock_release(&file_lock);
					return -1;
				}
			int read = file_read(file, buffer, length);
			lock_release(&file_lock);
			return read;
		}
	else
		return -1;
}

int
write (int fd, const void *buffer, unsigned length)
{
	if (!buffer)
		exit(-1);
	else if (fd == STDOUT_FILENO)
		{
			putbuf(buffer, length);
			return length;
		}
	else if (fd > STDOUT_FILENO && fd < 128)
		{
			lock_acquire(&file_lock);
			struct thread *current = thread_current();
			struct file *file = current->file_des[fd];
			if (!file)
				{
					lock_release(&file_lock);
					return -1;
				}
			struct inode *inode = file_get_inode (file);
			if (!inode)
				{
					lock_release(&file_lock);
					return -1;
				}
			bool is_dir = inode_is_dir(inode);
			if (is_dir)
				{
					lock_release(&file_lock);
					return -1;
				}
			int write = file_write(file, buffer, length);
			lock_release(&file_lock);
			return write;
		}
	else
		return -1;
}

void
seek (int fd, unsigned position)
{
	if (fd < 128 && fd > 1)
		{
			lock_acquire(&file_lock);
			struct file *file = thread_current()->file_des[fd];
			if (!file)
				{
					lock_release(&file_lock);
					exit(-1);
				}
			file_seek(file, position);
			lock_release(&file_lock);
		}
	else
		exit(-1);
}

unsigned
tell (int fd)
{
	if (fd < 128 && fd > 1)
		{
			lock_acquire(&file_lock);
			struct file *file = thread_current()->file_des[fd];
			if (!file)
				{
					lock_release(&file_lock);
					return -1;
				}
			int file_pos = file_tell(file);
			lock_release(&file_lock);
			return file_pos;
		}
	else
		return -1;
}

void
close (int fd)
{
	if (fd < 2 || fd > 127)
		exit(-1);
	lock_acquire(&file_lock);
	struct thread *current = thread_current();
	struct file *file = current->file_des[fd];
	if (!file)
		{
			lock_release(&file_lock);
			exit(-1);
		}
	struct inode *inode = file_get_inode (file);
	if (!inode)
		{
			lock_release(&file_lock);
			return -1;
		}
	bool is_dir = inode_is_dir(inode);
	if (is_dir)
		dir_close((struct dir *) inode);
	else
		file_close(file);
	current->file_des[fd] = 0;
	lock_release(&file_lock);
}

int
practice (int i)
{
	return i + 1;
}

bool
chdir (const char *dir)
{
	return filesys_chdir(dir);
}

bool
mkdir (const char *dir)
{
	return filesys_create(dir, 0, true);
}

bool
readdir (int fd, char name[READDIR_MAX_LEN + 1])
{
	// uses dir_readdir
	return false;
}

bool
isdir (int fd)
{
	if (fd < 2 || fd > 127)
		exit(-1);
	lock_acquire(&file_lock);
	struct thread *current = thread_current();
	struct file *file = current->file_des[fd];
	if (!file)
		{
			lock_release(&file_lock);
			exit(-1);
		}
	struct inode *inode = file_get_inode (file);
	if (!inode)
		{
			lock_release(&file_lock);
			exit(-1);
		}
	bool is_dir = inode_is_dir(inode);
	lock_release(&file_lock);
	return is_dir;
}

int
inumber (int fd)
{
	return 0;
}
