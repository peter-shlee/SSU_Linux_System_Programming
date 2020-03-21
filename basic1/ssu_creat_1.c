#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(){
	char *fname = "ssu_test.txt"; // creat할 파일 이름
	int fd; // 파일 디스크립터 저장할 변수

	if((fd = creat(fname, 0666)) < 0){ // creat가 실패했다면
		// stderr에 에러메세지 출력
		fprintf(stderr, "creat error for %s\n", fname);
		exit(1); // 프로그램 종료
	} else { // creat 성공했다면
		// 성공 메세지 출력
		printf("Success!\nFilename : %s\nDescriptor : %d\n", fname, fd);
		close(fd); // 파일 close
	}

	exit(0);
}
