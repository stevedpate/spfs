#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

pid_t
file_is_locked(int fd)
{
    struct flock    lock;

    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = 0;

    fcntl(fd, F_GETLK, &lock);
    return (lock.l_type == F_UNLCK) ? 0 : lock.l_pid;
}

int main()
{
    pid_t  pid;
    int    i, fd;

    fd = open("hello", O_RDONLY);

    for (i = 0 ; i < 3 ; i++) {
        if ((pid = file_is_locked(fd)) != 0) {
            printf("lcat - waiting for lock\n");
            sleep(1);
        } else {
            system("cat hello"); /* shouldn't get here */
        }
    }
    printf("pid = %d\n", pid);
    kill(pid, SIGUSR1);
    sleep(1);
    if ((pid = file_is_locked(fd)) == 0) {
        system("cat hello");
    } else {
        /* shouldn't get here */
        printf("lcat - lock still not released\n");
    }
}
