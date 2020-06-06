#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

char *execute_path;
char *src_path;
char *dst_path;

void printUsage(const char *process_name);
void checkProcessArguments(int argc, char *argv[]);
void syncDirectory(const char *src_path_name);
void syncFile(const char *src_file_name);
void copy(const char *src, const char *dst);

int main(int argc, char *argv[]) {
	struct stat statbuf;

	checkProcessArguments(argc, argv);
	printf("1\n");//////////////////////////////////////////////////////////////////////////////

	execute_path = getcwd(NULL, 0);

	if (stat(argv[1], &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", argv[1]); 
		exit(1);
	}

	if (S_ISDIR(statbuf.st_mode)) { // src 인자가 디렉토리라면
		chdir(argv[1]);
		src_path = getcwd(NULL, 0); // 해당 디렉토리의 절대 경로 저장
		chdir(execute_path);
	}

	chdir(argv[2]); // dst 인자는 무조건 디렉토리
	dst_path = getcwd(NULL, 0);
	chdir(execute_path);

	printf("2\n");//////////////////////////////////////////////////////////////////////////////
	if (src_path) { // src 인자가 디렉토리인 경우
		printf("directory\n");//////////////////////////////////////////////////////////////////////////////
		syncDirectory(src_path);
	} else { // src 인자가 일반 파일인 경우
		printf("file\n");//////////////////////////////////////////////////////////////////////////////
		syncFile(argv[1]);
	}
	printf("3\n");//////////////////////////////////////////////////////////////////////////////

	if (src_path) free(src_path);
	free(dst_path);
	free(execute_path);
	exit(0);
}

void syncDirectory(const char *src_path_name){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;

	if ((dp = opendir(src_path_name)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", src_path_name);
		exit(1);
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			continue;
			// 옵션에 따라 달라져야 할 부분

		} else { // 디렉토리가 아닌 파일이라면
			// 하위 디렉토리도 동기화 하는 옵션일때는 제대로 수행되지 않을 것임
			syncFile(dirp->d_name);
		}
	}

}

void syncFile(const char *src_file_name){
	int i;
	const char *src_relative_path;
	struct stat statbuf;
	struct utimbuf utimebuf;
	char dst_file_name[PATH_MAX];

	if (stat(src_file_name, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src_file_name);
		exit(1);
	}

	utimebuf.actime = statbuf.st_atime;
	utimebuf.modtime = statbuf.st_mtime;

	// src 파일의 상대경로명 구함
	src_relative_path = src_file_name;
	for (i = strlen(src_file_name) - 1; i >= 0; --i) {
		if (src_file_name[i] == '/') {
			src_relative_path = src_file_name + i + 1;
			break;
		}
	}

	// dst 디렉토리로 옮겨질 파일의 절대경로 생성
	if (dst_path[strlen(dst_path) - 1] == '/') {
		sprintf(dst_file_name, "%s%s", dst_path, src_relative_path);
	} else {
		sprintf(dst_file_name, "%s/%s", dst_path, src_relative_path);
	}
	printf("dst path:%s\n", dst_file_name); //////////////////////////////////////////////////////////////////////////////////////////
	
	if(access(dst_file_name, F_OK) < 0) {
		printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		copy(src_file_name, dst_file_name);
		utime(dst_file_name, &utimebuf);
	} else { // dst 디렉토리에 이미 이름이 동일한 파일이 존재하는 경우
		if (stat(dst_file_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dst_file_name);
			exit(1);
		}
		if (statbuf.st_mtime == utimebuf.modtime) return;

		printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		remove(dst_file_name);
		copy(src_file_name, dst_file_name);
		utime(dst_file_name, &utimebuf);
	}
}

void printUsage(const char *process_name){
	fprintf(stderr, "usage : %s <src> <dst>\n", process_name);
}

void checkProcessArguments(int argc, char *argv[]) { // 옵션 주어졌을 경우 제대로 동작 안함
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

void copy(const char *src, const char *dst) {
	int src_fd;
	int dst_fd;
	char buf[BUFFER_SIZE];
	size_t length;
	struct stat statbuf;

	if ((src_fd = open(src, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", src);
		exit(1);
	}
	
	if (stat(src, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src);
		exit(1);
	}

	if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode)) < 0) {
		fprintf(stderr, "open error for %s\n", dst);
		exit(1);
	}

	while((length = read(src_fd, buf, BUFFER_SIZE)) > 0) {
		write(dst_fd, buf, length);
	}
	
	close(src_fd);
	close(dst_fd);
}
