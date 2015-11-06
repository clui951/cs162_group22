#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

#define MIN_VALID_VADDR ((void *) 0x08048000)

static void syscall_handler (struct intr_frame *);
void check_valid_pointer (const void *vaddr);
struct file* get_file(int fd);

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
	check_valid_pointer((const void*) f->esp);
	uint32_t* args = ((uint32_t*) f->esp);
	// printf("current: %d, OPEN: %d, FS: %d, READ: %d, WRITE: %d, SEEK: %d, TELL: %d\n", args[0], SYS_OPEN, SYS_FILESIZE, SYS_WRITE, SYS_SEEK, SYS_TELL);
	switch (*(int *)f->esp)
		{
			case SYS_HALT: /* Halt the operating system. */
				{
					halt();
					break;
				}
			case SYS_EXIT: /* Terminate this process. */
				{
					exit(args[1]);
					break;
				}
			case SYS_EXEC: /* Start another process. */
				{
					check_valid_pointer(args[1]);
					args[1] = pagedir_get_page(thread_current()->pagedir, args[1]);
					f->eax = exec(args[1]);
					break;
				}
			case SYS_WAIT: /* Wait for a child process to die. */
				{
					f->eax = wait(args[1]);
					break;
				}
			case SYS_CREATE: /* Create a file. */
				{
					check_valid_pointer(args[1]);
					args[1] = pagedir_get_page(thread_current()->pagedir, args[1]);
					f->eax = create(args[1], args[2]);
					break;
				}
			case SYS_REMOVE: /* Delete a file. */
				{
					check_valid_pointer(args[1]);
					args[1] = pagedir_get_page(thread_current()->pagedir, args[1]);
					f->eax = remove(args[1]);
					break;
				}
			case SYS_OPEN: /* Open a file. */
				{
					check_valid_pointer(args[1]);
					args[1] = pagedir_get_page(thread_current()->pagedir, args[1]);
					f->eax = open(args[1]);
					break;
				}
			case SYS_FILESIZE: /* Obtain a file's size. */
				{
					f->eax = filesize(args[1]);
					break;
				}
			case SYS_READ: /* Read from a file. */
				{
					check_valid_pointer(args[2]);
					check_valid_pointer(args[2] + args[3]);
					args[2] = pagedir_get_page(thread_current()->pagedir, args[2]);
					f->eax = read(args[1], args[2], args[3]);
					break;
				}
			case SYS_WRITE: /* Write to a file. */
				{
					check_valid_pointer(args[2]);
					check_valid_pointer(args[2] + args[3]);
					args[2] = pagedir_get_page(thread_current()->pagedir, args[2]);
					f->eax = write(args[1], args[2], args[3]);
					break;
				}
			case SYS_SEEK: /* Change position in a file. */
				{
					break;
				}
			case SYS_TELL: /* Report current position in a file. */
				{
					break;
				}
			case SYS_CLOSE: /* Close a file. */
				{
					close(args[1]);
					break;
				}
			case SYS_PRACTICE: /* Returns arg incremented by 1 */
				{
					practice(args[1]);
					break;
				}
		}
}

void
check_valid_pointer (const void *vaddr)
{
	// printf("vaddr %d, MIN_VALID_VADDR %d, PHYS_BASE %d\n", vaddr, MIN_VALID_VADDR, PHYS_BASE);
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
	// printf("get to exit\n\n");
	struct thread *current = thread_current();
	if (current->aux->child)
		current->aux->child->exit_status = status; // my current aux is my parent's knowledge of me as a child
	printf("%s: exit(%d)\n", current->name, status);
	thread_exit();
}

pid_t
exec (const char *file)
{
	if (!file)
	{
		exit(-1);
	}
	printf("syscall.c exec: starting to execute\n");
	// printf("finishes executing in syscall exec\n");
	return process_execute(file);
}

int
wait (pid_t pid)
{
	if (pid < 0) {
		exit(-1);
	}
	return process_wait(pid);
}

bool
create (const char *file, unsigned initial_size)
{
	if (!file)
		exit(-1);
	lock_acquire(&file_lock);
	bool success = filesys_create(file, initial_size);
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
	for (i = 2; i < 128; i++) {
		if (!current->file_des[i]) {
			current->file_des[i] = new_file;
			lock_release(&file_lock);
			return i;
		}
	}
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
		exit(-1);
}

int
read (int fd, void *buffer, unsigned length)
{
	if (fd == STDIN_FILENO)
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

			int write = file_write(file, buffer, length);
			lock_release(&file_lock);
			return write;
		}
	else
		exit(-1);
}

void
seek (int fd, unsigned position)
{

}

unsigned
tell (int fd)
{

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
			return -1;
		}
	file_close(file);
	current->file_des[fd] = 0;
	lock_release(&file_lock);
}

int
practice (int i)
{
	return i++;
}
