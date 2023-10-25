#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
    int   pipefd[2];
    pid_t child;
 
    pipe(pipefd);
    child = fork();

    if (child == 0) { 
        /* child process */
        close(STDOUT_FILENO);
        dup(pipefd[1]);
        execlp("myecho", "myecho", "Hello world!\n",
    		   (char *)NULL);
    } else { 
        /* parent process */
        close(STDIN_FILENO);
        dup(pipefd[0]);
        execlp("mycat", "mycat", (char *)NULL);
    }
}
