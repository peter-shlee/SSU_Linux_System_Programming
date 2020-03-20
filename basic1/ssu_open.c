#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){
	char *fname = "ssu_test.txt";
	int fd;

	if((fd = open(fname, O_RDONLY)) < 0){
		fprintf(stderr,"open error for %s\n", fname);
		exit(1);
	} else 
		printf("Success!\nFilename : %s\nDescriptor : %d\n", fname, fd);

	exit(0);
}
