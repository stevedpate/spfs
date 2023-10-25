#include <stdio.h>
#include <string.h>

#define BUFSZ 64

int
main()
{
    char buf[BUFSZ];

	fprintf(stderr, "I'm mycat\n");
    fgets(buf, BUFSZ, stdin);
    printf("%s", buf);
}
