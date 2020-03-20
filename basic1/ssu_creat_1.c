#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(){
	char *fname = "ssu_test.txt";
	int fd;

	if((fd = creat(fname, 0666)) < 0){
		fprintf(stderr, "creat error for %s\n", fname);
		exit(1);
	} else {
		printf("Success!\nFilename : %s\nDescriptor : %d\n", fname, fd);
		close(fd);
	}

	exit(0);
}
