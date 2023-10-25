#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
   struct dirent **namelist;
   int nentries, i;

   nentries = scandir(".", &namelist, NULL, versionsort);
   for (i=0 ; i<nentries ; i++) {
	   printf("%s\n", namelist[i]->d_name);
	   free(namelist[i]);
   }
   free(namelist);
}
