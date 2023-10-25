#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int
main()
{
	struct stat	st;
	char		*addr;
	int			pgsz, fd, i;

	pgsz = getpagesize();
	printf("PAGESIZE = %d\n", pgsz);
	fd = open("/mnt/big-lorem-ipsum", O_RDONLY);
	fstat(fd, &st);
	addr = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	for (i=0 ; i < st.st_size ; i++) {
		putchar(*addr);
		addr++;
	}
}
