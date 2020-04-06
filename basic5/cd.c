#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if(argc < 2) {
		fprintf(stderr, "usage : %s <pathname>\n", argv[0]);
		exit(1);
	}

	if (chdir(argv[1]) < 0) {
		fprintf(stderr, "chdir error\n");
		exit(1);
	}

	printf("chdir to %s succeded.\n", argv[1]);
	printf("%s\n", getcwd(NULL, 0));
	exit(0);
}
