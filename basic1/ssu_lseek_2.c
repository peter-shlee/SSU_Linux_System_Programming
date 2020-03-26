#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

// creat 할때 사용할 mode
#define CREAT_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

char buf1[] = "1234567890";
char buf2[] = "ABCDEFGHIJ";

int main(){
	char *fname = "ssu_hole.txt"; // 파일명
	int fd; // 파일 디스크립터 저장할 변수

	if((fd = creat(fname, CREAT_MODE)) < 0){ // creat 실패했다면
		// stderr에 에러메세지 출력
		fprintf(stderr, "creat error for %s\n", fname);
		exit(1); // 프로그램 종료
	}

	if(write(fd, buf1, 12) != 12){ // 파일에 buf1내용 write
		// write 실패했다면
		// stderr에 에러메세지 출력
		fprintf(stderr, "buf1 write error\n"); 
		exit(1); // 프로그램 종료
	}

	// 파일 맨 앞에서 15000번째 바이트로 오프셋 이동
	if(lseek(fd, (off_t)15000, SEEK_SET) < 0){ 
		// lseek 실패했다면
		fprintf(stderr, "lseek error\n"); // stderr에 에러메세지 출력
		exit(1); // 프로그램 종료
	}

	if(write(fd, buf2, 12) != 12){ // 파일에 buf2 내용 write
		// write 실패했다면
		// stderr에 에러메세지 출력
		fprintf(stderr, "buf2 write error\n"); 
		exit(1); // 프로그램 종료
	}


	exit(0);
}
