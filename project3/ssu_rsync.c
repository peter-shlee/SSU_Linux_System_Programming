#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

char *execute_path;

void printUsage(const char *process_name);
void checkProcessArguments(int argc, char *argv[]);

int main(int argc, char *argv[]) {

	checkProcessArguments(argc, argv);

	execute_path = getcwd(NULL, 0);




	free(execute_path);
	exit(0);
}

void printUsage(const char *process_name){
	fprintf(stderr, "usage : %s <src> <dst>\n", process_name);
}

void checkProcessArguments(int argc, char *argv[]) {
	struct stat statbuf;

	// 전달인자 수 잘못 됐을 때
	if (argc != 3) {
		printUsage(argv[0]);
		exit(1);
	}

	// src 인자
	// 인자로 입력받은 파일 혹은 디렉토리를 찾을 수 없으면 usage 출력 후 프로그램 종료
	if (access(argv[1], F_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
	// 인자로 입력받은 파일 혹은 디렉토리의 접근권한이 없는 경우 usage 출력 후 프로그램 종료
	if (access(argv[1], R_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
	

	// dst 인자
	// 인자로 받은 디렉토리를 찾을 수 없으면 usage 출력 후 프로그램 종료
	if (access(argv[2], F_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
	// 인자로 입력받은 디렉토리가 디렉토리 파일이 아니라면 usage 출력 후 프로그램 종료
	if (stat(argv[2], &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", argv[2]);
		exit(1);
	}
	if (!S_ISDIR(statbuf.st_mode)) {
		printUsage(argv[0]);
		exit(1);
	}
	// 인자로 입력받은 디렉토리의 접근권한이 없는 경우 usage 출력 후 프로그램 종료
	if (access(argv[2], R_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
}
