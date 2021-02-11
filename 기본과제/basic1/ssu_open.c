#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){
	char *fname = "ssu_test.txt"; // open할 파일 이름
	int fd; // 파일 디스크립터 저장할 변수

	if((fd = open(fname, O_RDONLY)) < 0){ // open이 제대로 수행되지 않으면
		// stderr에 에러메세지 출력
		fprintf(stderr,"open error for %s\n", fname);
		exit(1); // 프로그램 종료
	} else // open이 제대로 수행됐다면
		// 성공했다는 메세지 출력
		printf("Success!\nFilename : %s\nDescriptor : %d\n", fname, fd);

	exit(0);
}
