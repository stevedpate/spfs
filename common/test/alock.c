#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

void
mysig_hdlr(int signo)
{
    printf("alock - got signal\n");
    return;
}

int
main()
{
    struct flock     lock;
    int              fd;
    struct sigaction action;

    action.sa_handler = mysig_hdlr;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, NULL);

    fd = open("hello", O_RDWR);

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    fcntl(fd, F_SETLK, &lock);
    printf("alock - file now locked\n");
    pause();
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    printf("alock - file now unlocked\n");
}
