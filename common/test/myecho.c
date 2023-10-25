#include <stdio.h>

int
main(int argc, char *argv[])
{
    fprintf(stderr, "I'm myecho\n");
    fprintf(stdout, "%s", argv[1]);
}
