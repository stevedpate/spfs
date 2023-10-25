#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int
main()
{	
	struct stat buf;
	size_t	sz;
	char *rlbuf[256];
	int error = lstat("/mnt/bar", &buf);

	if (error < 0) {
		printf("stat failed\n");
		return 0;
	} else {
		printf("mode = %o\n", buf.st_mode);
		printf("mode = %o (%o)\n", buf.st_mode, buf.st_mode & S_IFMT);
		printf("size = %lo\n", buf.st_size);
		printf("blocks = %lo\n", buf.st_blocks);
		printf("ino = %lo\n", buf.st_ino);
	}
	if (S_ISLNK(buf.st_mode)) {
		printf("it's a link\n");
	}

	sz = readlink("/mnt/bar", rlbuf, sizeof(buf));
	printf("sz from readlink = %ld, errno = %d\n", sz, errno);

	error = lstat("bar", &buf);
	printf("mode for local bar = %o (%o)\n", buf.st_mode, buf.st_mode & S_IFMT);
}
