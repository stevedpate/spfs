#include <unistd.h>
#include <stdio.h>
#include <sys/resource.h>

int
main()
{
	struct rlimit  rlim;
    int            nentries;

    nentries = getdtablesize();
    getrlimit(RLIMIT_NOFILE, &rlim);
    printf("Current file table entries = %d\n", nentries);
    printf("rlimit current / maximum = %d / %d\n",
			(int)rlim.rlim_cur, (int)rlim.rlim_max);
}
