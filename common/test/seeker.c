#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int
main(int argc, char *argv[])
{
    char *buf = "hello world\n";
	int fd = open("/mnt/foo", O_CREAT|O_WRONLY, 0700);

	lseek(fd, 2030, SEEK_SET);
	write(fd, buf, strlen(buf));
}
