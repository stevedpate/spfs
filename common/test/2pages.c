#include <unistd.h>
#include <fcntl.h>

int
main()
{
    char buf[2];
    int	 fd;
   
    fd  = open("big-lorem-ipsum", O_RDONLY);
    read(fd, buf, 1);
    lseek(fd, 4096, SEEK_SET);
    read(fd, buf, 1);
    pause();
}
