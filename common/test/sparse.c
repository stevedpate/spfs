#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

char *buf1 = "hello";
char *buf2 = "world";

int
main(int argc, char *argv[])
{
	int fd = open("/mnt/foo", O_CREAT|O_WRONLY, 0700);
	write(fd, buf1, strlen(buf1));
	lseek(fd, 16384, SEEK_SET);
	write(fd, buf2, strlen(buf2));
}
