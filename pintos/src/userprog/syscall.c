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

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
	check_valid_pointer((const void*) f->esp);
	uint32_t* args = ((uint32_t*) f->esp);
	switch (*(int *)f->esp)
		{
			case SYS_HALT: /* Halt the operating system. */
				{
					// printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_EXIT: /* Terminate this process. */
				{
					exit(args[1]);
					break;
				}
			case SYS_EXEC: /* Start another process. */
				{
					exec(args[1]);
					break;
				}
			case SYS_WAIT: /* Wait for a child process to die. */
				{
					// printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_CREATE: /* Create a file. */
				{
					create(args[1], args[2]);
					break;
				}
			case SYS_REMOVE: /* Delete a file. */
				{
					remove(args[1]);
					break;
				}
			case SYS_OPEN: /* Open a file. */
				{
					open(args[1]);
					break;
				}
			case SYS_FILESIZE: /* Obtain a file's size. */
				{
					filesize(args[1]);
					break;
				}
			case SYS_READ: /* Read from a file. */
				{
					read(args[1], args[2], args[3]);
					break;
				}
			case SYS_WRITE: /* Write to a file. */
				{
					write(args[1], args[2], args[3]);
					break;
				}
			case SYS_SEEK: /* Change position in a file. */
				{
					// printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_TELL: /* Report current position in a file. */
				{
					// printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_CLOSE: /* Close a file. */
				{
					close(args[1]);
					break;
				}
			case SYS_PRACTICE: /* Returns arg incremented by 1 */
				{
					// printf("System call number: %d\n", args[0]);
					break;
				}
		}
}

void
check_valid_pointer (const void *vaddr)
{
	if (!is_user_vaddr(vaddr) || vaddr < MIN_VALID_VADDR)
		exit(-1);
}

void
exit (int status)
{
	struct thread *current = thread_current();
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
}

// int wait (pid_t pid);

bool
create (const char *file, unsigned initial_size)
{
	if (!file || !initial_size) {
		exit(-1);
	}
	// if (initial_size == 0) {

	// }
}

bool
remove (const char *file)
{
	if (!file) {
		exit(-1);
	}
}

int
open (const char *file)
{
	if (!file) {
		exit(-1);
	}
}

int
filesize (int fd)
{
	if (fd < 128)
		{
			struct file *file = thread_current()->file_des[fd];
			return file_length(file);
		}
	else
		exit(-1);
}

int
read (int fd, void *buffer, unsigned length)
{
	if (fd == STDIN_FILENO)
		{
			return length;
		}
	else if (fd > STDOUT_FILENO && fd < 128)
		{
			struct thread *current = thread_current();
			struct file *file = current->file_des[fd];
			return file_read(file, length);
		}
	else
		exit(-1);
}

int
write (int fd, const void *buffer, unsigned length)
{
	if (fd == STDOUT_FILENO)
		{
			putbuf(buffer, length);
			return length;
		}
	else if (fd > STDOUT_FILENO && fd < 128)
		{
			struct thread *current = thread_current();
			struct file *file = current->file_des[fd];
			return file_write(file, buffer, length);
		}
	else
		exit(-1);
}

// void seek (int fd, unsigned position);
// unsigned tell (int fd);

void
close (int fd)
{
	if (fd < 2 || fd >= 128)
		exit(-1);
}

// int practice (int i);
