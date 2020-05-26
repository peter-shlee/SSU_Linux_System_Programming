#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>


int main(void)
{
	char randNum;
	int i;
	int fd;

	fd = open("ssu_test1.txt", O_WRONLY | O_TRUNC | O_CREAT);

	srand(time(NULL));

	for(i = 0; i < 500000; ++i){
		randNum = rand() % 10 + '0';
		write(fd, &randNum, 1);
	}

	return 0;
}

