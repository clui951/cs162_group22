#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#include <random.h>

static char buf[100];

void
test_main (void) 
{
	const char *file_name = "testfile";
	int fd;
	CHECK (create (file_name, 100), "create \"%s\"", file_name);
	CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
	random_bytes (buf, 100);
	unsigned long long count = hitrate (fd, buf, 100);
	if (count > 0) {
		msg ("true");
	}
  CHECK (count > 0, "test hitrate");
  msg ("close \"%s\"", file_name);
  close (fd);
}
