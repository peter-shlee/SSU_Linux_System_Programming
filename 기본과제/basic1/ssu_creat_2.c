#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){
	char *fname = "ssu_test.txt"; // creat할 파일 이름
	int fd; // 파일 디스크립터 저장할 변수

	if((fd = creat(fname, 0666) < 0)){ // creat 실패했다면
		// stderr에 에러 메세지 출력
		fprintf(stderr, "creat error for %s\n", fname);
		exit(1); // 프로그램 종료
	} else { // creat 성공했다면
		close(fd); // creat한 파일 close
		fd = open(fname, O_RDONLY); // read only mode로 다시 open
		// 성공 메세지 출력
		printf("Succeeded!\n<%s> is new readable and writable\n",fname);
	}


	exit(0);
}
