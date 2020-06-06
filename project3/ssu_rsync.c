#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

const char *LOG_FILE_NAME = "ssu_rsync_log";
const char *TEMP_PATH_NAME = "tmp_for_sigint_20160548";
char *execute_path;
char *src_path;
char *dst_path;
char *src_file;
char *temp_dir_absolute_path;

void printUsage(const char *process_name);
void checkProcessArguments(int argc, char *argv[]);
void syncDirectory(const char *src_path_name, const char *dst_path_name, int sync_dir_flag);
void syncFile(const char *src_file_name, const char *dst_path_name);
void copy(const char *src, const char *dst);
void removeDirectory(const char *target);

int r_option;
int t_option;
int m_option;

static void sigint_during_sync_handler(int signo);

int main(int argc, char *argv[]) {
	struct sigaction sig_act;
	sigset_t sig_set;

	execute_path = getcwd(NULL, 0);
	umask(0);
	checkProcessArguments(argc, argv);
	
	// 파일 동기화 중 SIGINT 전달 될 때를 대비한 임시 파일 저장 디렉토리 초기화
	if (chdir(TEMP_PATH_NAME) == 0) { 
		chdir(execute_path);
		removeDirectory(TEMP_PATH_NAME);
		rmdir(TEMP_PATH_NAME);
	}
	mkdir(TEMP_PATH_NAME, 0777);
	temp_dir_absolute_path = realpath(TEMP_PATH_NAME, NULL);

	syncDirectory(dst_path,temp_dir_absolute_path, 1);
	// 여기서 SIGINT 핸들러 등록하기
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = 0;
	sig_act.sa_handler = sigint_during_sync_handler;
	if (sigaction(SIGINT, &sig_act, NULL) != 0) {
		fprintf(stderr, "sigaction error\n");
		exit(1);
	}


	// 동기화
	if (src_path) { // src 인자가 디렉토리인 경우
		syncDirectory(src_path, dst_path, r_option);
	} else { // src 인자가 일반 파일인 경우
		syncFile(src_file, dst_path);
	}


	// 이제 동기화 끝났으므로 기본 핸들러로 바꾼다
	sig_act.sa_handler = SIG_DFL;
	if (sigaction(SIGINT, &sig_act, NULL) != 0) {
		fprintf(stderr, "sigaction error\n");
		exit(1);
	}

	chdir(execute_path);
	removeDirectory(TEMP_PATH_NAME);
	rmdir(TEMP_PATH_NAME);


	if (src_path) free(src_path);
	free(dst_path);
	free(execute_path);
	exit(0);
}

static void sigint_during_sync_handler(int signo){
	chdir(execute_path);
	removeDirectory(dst_path);
	rmdir(dst_path);
	rename(temp_dir_absolute_path, dst_path);
	removeDirectory(temp_dir_absolute_path);
	rmdir(temp_dir_absolute_path);

	exit(0);
}

void syncDirectory(const char *src_path_name, const char *dst_path_name, int sync_dir_flag){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;
	char next_dst_path[PATH_MAX];

	if ((dp = opendir(src_path_name)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", src_path_name);
		exit(1);
	}

	current_path = getcwd(NULL, 0);
	chdir(src_path_name);

	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			// 옵션에 따라 달라져야 할 부분
			if (sync_dir_flag) {
				if (dst_path_name[strlen(dst_path_name) - 1] == '/') {
					sprintf(next_dst_path, "%s%s", dst_path_name, dirp->d_name);
				} else {
					sprintf(next_dst_path, "%s/%s", dst_path_name, dirp->d_name);
				}

				if (access(next_dst_path, F_OK) < 0) mkdir(next_dst_path, statbuf.st_mode);
				syncDirectory(dirp->d_name, next_dst_path, sync_dir_flag);
			} else continue;

		} else { // 디렉토리가 아닌 파일이라면
			// 하위 디렉토리도 동기화 하는 옵션일때는 제대로 수행되지 않을 것임
			syncFile(dirp->d_name, dst_path_name); //////////dst_path_name 바뀌도록 해야함
		}
	}

	chdir(current_path);
	free(current_path);
}

void syncFile(const char *src_file_name, const char *dst_path_name){
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
	if (dst_path_name[strlen(dst_path_name) - 1] == '/') {
		sprintf(dst_file_name, "%s%s", dst_path_name, src_relative_path);
	} else {
		sprintf(dst_file_name, "%s/%s", dst_path_name, src_relative_path);
	}
	
	if(access(dst_file_name, F_OK) < 0) {
		printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		copy(src_file_name, dst_file_name);
	} else { // dst 디렉토리에 이미 이름이 동일한 파일이 존재하는 경우
		if (stat(dst_file_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dst_file_name);
			exit(1);
		}
		if (statbuf.st_mtime == utimebuf.modtime) return;

		printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		remove(dst_file_name);
		copy(src_file_name, dst_file_name);
	}
}

void printUsage(const char *process_name){
	fprintf(stderr, "usage : %s <option> <src> <dst>\n", process_name);
}

void checkProcessArguments(int argc, char *argv[]) { // 옵션 주어졌을 경우 제대로 동작 안함
	struct stat statbuf;
	char *src;
	char *dst;

	// 전달인자 수 잘못 됐을 때
	if (argc < 3) {
		printUsage(argv[0]);
		exit(1);
	}

	if (argv[1][0] == '-') {
		switch(argv[1][1]) {
			case 'r':
				r_option = 1;
				break;
			case 't':
				t_option = 1;
				break;
			case 'm':
				m_option = 1;
				break;
			default:
				printUsage(argv[0]);
				exit(1);
		}
		src = argv[2];
		dst = argv[3];
	} else {
		src = argv[1];
		dst = argv[2];
	}

	// src 인자
	// 인자로 입력받은 파일 혹은 디렉토리를 찾을 수 없으면 usage 출력 후 프로그램 종료
	if (access(src, F_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
	// 인자로 입력받은 파일 혹은 디렉토리의 접근권한이 없는 경우 usage 출력 후 프로그램 종료
	if (access(src, R_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
	

	// dst 인자
	// 인자로 받은 디렉토리를 찾을 수 없으면 usage 출력 후 프로그램 종료
	if (access(dst, F_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}
	// 인자로 입력받은 디렉토리가 디렉토리 파일이 아니라면 usage 출력 후 프로그램 종료
	if (stat(dst, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", argv[2]);
		exit(1);
	}
	if (!S_ISDIR(statbuf.st_mode)) {
		printUsage(argv[0]);
		exit(1);
	}
	// 인자로 입력받은 디렉토리의 접근권한이 없는 경우 usage 출력 후 프로그램 종료
	if (access(dst, R_OK) < 0) {
		printUsage(argv[0]);
		exit(1);
	}


	if (stat(src, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src); 
		exit(1);
	}

	if (S_ISDIR(statbuf.st_mode)) { // src 인자가 디렉토리라면
		chdir(src);
		src_path = getcwd(NULL, 0); // 해당 디렉토리의 절대 경로 저장
		chdir(execute_path);
	} else {
		src_file = src;
	}

	chdir(dst); // dst 인자는 무조건 디렉토리
	dst_path = getcwd(NULL, 0);
	chdir(execute_path);
}

void copy(const char *src, const char *dst) {
	int src_fd;
	int dst_fd;
	char buf[BUFFER_SIZE];
	size_t length;
	struct stat statbuf;
	struct utimbuf utimebuf;

	if ((src_fd = open(src, O_RDONLY)) < 0) {
		fprintf(stderr, "open error for %s\n", src);
		exit(1);
	}
	
	if (stat(src, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src);
		exit(1);
	}

	utimebuf.actime = statbuf.st_atime;
	utimebuf.modtime = statbuf.st_mtime;

	if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode)) < 0) {
		fprintf(stderr, "open error for %s %s\n", dst, strerror(errno));
		exit(1);
	}

	while((length = read(src_fd, buf, BUFFER_SIZE)) > 0) {
		write(dst_fd, buf, length);
	}

	utime(dst, &utimebuf);
	
	close(src_fd);
	close(dst_fd);
}

void removeDirectory(const char *target){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;

	if ((dp = opendir(target)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", target);
		exit(1);
	}

	current_path = getcwd(NULL, 0);
	chdir(target);

	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			removeDirectory(dirp->d_name);
			rmdir(dirp->d_name);
		} else { // 디렉토리가 아닌 파일이라면
			unlink(dirp->d_name);
		}
	}

	chdir(current_path);
	free(current_path);
}
