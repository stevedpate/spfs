#include <stdio.h>
#include <unistd.h>

int
main()
{	
	char	buf[256];
	size_t		sz;
	pid_t	pid;

	pid = getpid();
	printf("pid = %d\n", pid);
	sleep(20);

	sz = readlink("/mnt/bar", buf, sizeof(buf));
	printf("sz from readlink = %ld\n", sz);
}

