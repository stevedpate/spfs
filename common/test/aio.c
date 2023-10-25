#include <fcntl.h>
#include <aio.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>

char *buf2 = "abcdefghij";
char buf1[32];
char buf3[32];

void
aio_hdlr(int signo)
{
    printf("buf1 = %.10s\n", buf1);
    printf("buf3 = %.10s\n", buf3);
}

int
main(int argc, char *argv[])
{
    struct sigaction act;
    struct sigevent  sevp;
    struct aiocb     *list_aio[3];
    struct aiocb     aio1, aio2, aio3;
    int              err, fd;

    fd = open("aio-data", O_RDWR);

    aio1.aio_fildes  = fd;   aio1.aio_lio_opcode = LIO_READ;
    aio1.aio_buf     = buf1; aio1.aio_offset     = 10; 
    aio1.aio_nbytes  = 10;   aio1.aio_reqprio    = 0;

    aio2.aio_fildes  = fd;   aio2.aio_lio_opcode = LIO_WRITE;
    aio2.aio_buf     = buf2; aio2.aio_offset     = 20; 
    aio2.aio_nbytes  = 10;   aio2.aio_reqprio    = 0;

    aio3.aio_fildes  = fd;   aio3.aio_lio_opcode = LIO_READ;
    aio3.aio_buf     = buf3; aio3.aio_offset     = 30; 
    aio3.aio_nbytes  = 10;   aio3.aio_reqprio    = 0;

    list_aio[0] = &aio1;
    list_aio[1] = &aio2;
    list_aio[2] = &aio3;

    memset(&act, 0, sizeof(act));
    act.sa_handler = aio_hdlr;
    sigaction(SIGUSR1, &act, NULL);
    act.sa_handler = SIG_IGN;
    sigaction(SIGHUP, &act, NULL);

    memset(&sevp, 0, sizeof(sevp));
    sevp.sigev_signo = SIGUSR1;
    sevp.sigev_notify = SIGEV_SIGNAL;
    sevp.sigev_value.sival_ptr = (void *)list_aio;

    err = lio_listio(LIO_NOWAIT, list_aio, 3, &sevp);
    pause();
}
