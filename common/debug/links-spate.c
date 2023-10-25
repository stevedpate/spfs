#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int
main()
{	
    int fd1 = open("/mnt/lorem-ipsum", O_RDONLY);
    int fd2 = open("/mnt/hard-link", O_RDONLY);
    pause();
}

