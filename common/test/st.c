#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int
main()
{	
	struct stat buf;
	int error = stat("../test/lorem-ipsum", &buf);

	printf("mode = %o (%o)\n", buf.st_mode, buf.st_mode & S_IFMT);
	if (S_ISREG(buf.st_mode)) {
		printf("The file is a regular file\n");
	}
	if ((buf.st_mode & S_IFMT) == S_IFREG) {
		printf("... and again!\n");
	}
}
