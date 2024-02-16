#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int
main()
{
    char buf[8192];
    int	 fd1, fd2, res;

    fd1  = open("/home/spate/spfs/common/test/big-lorem-ipsum", O_RDONLY);
    fd2  = open("/mnt/myfile", O_WRONLY|O_CREAT);
    res = read(fd1, buf, 6);
    printf("Read %d bytes\n", res);
    write(fd2, buf, res);
    printf("Written %d bytes\n", res);
    pause();
}
