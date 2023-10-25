#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int
main()
{	
    int fd = open("lorem-ipsum", O_RDONLY);
    pause();
}

