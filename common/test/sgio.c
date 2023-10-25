#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/uio.h>

int
main(int argc, char *argv[])
{
	int  fd;
    char buf2[32];
    char buf3[32];
    char buf1[32];
    struct iovec bufs[] = {
    	{ .iov_base = (void *)buf1, .iov_len = 10 },
    	{ .iov_base = (void *)buf2, .iov_len = 10 },
    	{ .iov_base = (void *)buf3, .iov_len = 10 },
    };

	fd = open("vector-file", O_RDONLY);
	lseek(fd, 10, SEEK_SET);
    readv(fd, bufs, sizeof(bufs) / sizeof(bufs[0]));
    printf("buf1 = %.10s\n", buf1);
    printf("buf2 = %.10s\n", buf2);
    printf("buf3 = %.10s\n", buf3);

    preadv(fd, bufs, sizeof(bufs) / sizeof(bufs[0]), 20);
    printf("buf1 = %.10s\n", buf1);
    printf("buf2 = %.10s\n", buf2);
    printf("buf3 = %.10s\n", buf3);
}
