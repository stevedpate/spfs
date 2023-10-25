#include <stdio.h>

#define SP_BSIZE 2048

int
main()
{
	printf("Calculating block numbers given offset\n\n");
	printf("0 / SP_BSIZE    = %d\n", 0 / SP_BSIZE);
	printf("1 / SP_BSIZE    = %d\n", 1 / SP_BSIZE);
	printf("2 / SP_BSIZE    = %d\n", 2 / SP_BSIZE);
	printf("2047 / SP_BSIZE = %d\n\n", 2047 / SP_BSIZE);

	printf("2048 / SP_BSIZE = %d\n", 2048 / SP_BSIZE);
	printf("4095 / SP_BSIZE = %d\n\n", 4095 / SP_BSIZE);

	printf("4096 / SP_BSIZE = %d\n", 4096 / SP_BSIZE);

	printf("\nCalculating block offset given file offset\n\n");
	printf("0 %% SP_BSIZE    = %d\n", 0 % SP_BSIZE);
	printf("1 %% SP_BSIZE    = %d\n", 1 % SP_BSIZE);
	printf("2 %% SP_BSIZE    = %d\n", 2 % SP_BSIZE);
	printf("2047 %% SP_BSIZE = %d\n\n", 2047 % SP_BSIZE);

	printf("2048 %% SP_BSIZE = %d\n", 2048 % SP_BSIZE);
	printf("2049 %% SP_BSIZE = %d\n", 2049 % SP_BSIZE);
	printf("4095 %% SP_BSIZE = %d\n\n", 4095 % SP_BSIZE);

	printf("4096 %% SP_BSIZE = %d\n", 4096 % SP_BSIZE);
}
