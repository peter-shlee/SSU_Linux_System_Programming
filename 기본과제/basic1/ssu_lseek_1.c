#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){
	char *fname = "ssu_test.txt"; // 파일명
	off_t fsize; // 파일의 크기 저장할 변수
	int fd; // 파일 디스크립터 저장할 변수

	if((fd = open(fname, O_RDONLY)) < 0){ // open 실패했다면
		// stderr에 에러 메세지 출력
		fprintf(stderr, "open error for %s\n", fname);
		exit(1); // 프로그램 종료
	}

	if((fsize = lseek(fd, (off_t)0, SEEK_END)) < 0){ // lseek 실패했다면
		fprintf(stderr, "lseek error\n"); // stderr에 에러 메세지 출력
		exit(1); // 프로그램 종료
	}

	// 파일 크기 출력
	printf("The size of <%s> is %ld bytes.\n", fname, fsize);

	exit(0);
}

