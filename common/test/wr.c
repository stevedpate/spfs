#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

int
main()
{
    char *buf = "hello world\n";
    int  fd = open("/mnt/foo", O_CREAT|O_WRONLY, 0700);

    write(fd, buf, strlen(buf));
}
