#include <stdio.h>
#include <fcntl.h>

int
main()
{
		int	fd = open("lorem-ipsum", O_RDONLY);
		printf("fd = %d\n", fd);
}
