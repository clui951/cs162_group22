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
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_EXIT: /* Terminate this process. */
				{
					// f->eax = args[1]; what's this for again?
					exit(args[1]);
					break;
				}
			case SYS_EXEC: /* Start another process. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_WAIT: /* Wait for a child process to die. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_CREATE: /* Create a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_REMOVE: /* Delete a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_OPEN: /* Open a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_FILESIZE: /* Obtain a file's size. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_READ: /* Read from a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_WRITE: /* Write to a file. */
				{
					// printf("syscall number: %d %d %d %d\n", args[0], args[1], args[2], args[3]);
					write(args[1], args[2], args[3]);
					// printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_SEEK: /* Change position in a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_TELL: /* Report current position in a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_CLOSE: /* Close a file. */
				{
					printf("System call number: %d\n", args[0]);
					break;
				}
			case SYS_PRACTICE: /* Returns arg incremented by 1 */
				{
					printf("System call number: %d\n", args[0]);
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

// struct file*
// get_file(int fd)
// {
// 	struct thread *current = thread_current();

// }

// void halt (void);

void
exit (int status)
{
	struct thread *current = thread_current();
	printf("%s: exit(%d)\n", current->name, status);
	thread_exit();
}

// pid_t exec (const char *file);
// int wait (pid_t pid);
// bool create (const char *file, unsigned initial_size);
// bool remove (const char *file);
// int open (const char *file);
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned length);
int
write (int fd, const void *buffer, unsigned length)
{
	if (fd == STDOUT_FILENO)
		{
			putbuf(buffer, length);
			return length;
		}
	// struct
	// return file_write(thefile, buffer, length);
	return length;
}
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);
// int practice (int i);
