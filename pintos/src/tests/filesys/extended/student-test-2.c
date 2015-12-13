#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#include <random.h>

static char buf[64000];

void
test_main (void) 
{
	const char *file_name = "testfile";
	int fd;
	CHECK (create (file_name, 64000), "create \"%s\"", file_name);
	CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
	random_bytes (buf, 64000);
	bool success = coalesce (fd, buf, 100);
	if (success) {
		msg ("true");
	}
  CHECK (success, "test coalesce");
  msg ("close \"%s\"", file_name);
  close (fd);
}
