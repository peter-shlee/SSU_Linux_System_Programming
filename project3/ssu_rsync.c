#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SECOND_TO_MICRO 1000000
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);
struct timeval begin_t, end_t;

#define BUFFER_SIZE 1024
#define COMMAND_BUFFER_SIZE 4096

const char *LOG_FILE_NAME = "ssu_rsync_log";
const char *TEMP_PATH_NAME = "tmp_for_sigint_20160548";
const char *TEMP_LOG_FILE = "tmp_for_delete_log_20160548";
const char *TAR_FILE_NAME = "tmptar20160548.tar";
char *execute_path;
char *src_path;
char *dst_path;
char *src;
char *dst;
char *temp_dir_absolute_path;

void do_tOption(const char *src_path_name, const char *dst_path_name);
void do_mOption(FILE *tmp_log_fp, const char *path_name, const char *src_path_name, const char *dst_path_name, int sync_dir_flag);
void printUsage(const char *process_name);
void checkProcessArguments(int argc, char *argv[]);
void syncDirectory(const char *src_path_name, const char *dst_path_name, int sync_dir_flag);
void syncFile(const char *src_file_name, const char *dst_path_name);
void copy(const char *src, const char *dst);
void removeDirectory(FILE *tmp_log_fp, const char *path_name, const char *target);
void printLog(FILE *tmp_log_fp, const char *src_path_name, int sync_dir_flag);
void printFileNameAndSizeAtLogFile(FILE * fp, const char *src_path_name, const char *path_name, int sync_dir_flag);
int checkSyncTarget(const char *src_file_name, const char *dst_path_name);

int r_option;
int t_option;
int m_option;

static void sigint_during_sync_handler(int signo);

int main(int argc, char *argv[]) { ///////// exit(1)들 sigint로 바꾸기
	FILE *tmp_log_fp;
	struct sigaction sig_act;
	sigset_t sig_set;
	gettimeofday(&begin_t, NULL); // 시작 시간 기록

	execute_path = getcwd(NULL, 0);
	umask(0);
	checkProcessArguments(argc, argv);
	
	// 파일 동기화 중 SIGINT 전달 될 때를 대비한 임시 파일 저장 디렉토리 초기화
	if (chdir(TEMP_PATH_NAME) == 0) { 
		chdir(execute_path);
		removeDirectory(NULL, NULL, TEMP_PATH_NAME);
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

	if ((tmp_log_fp = fopen(TEMP_LOG_FILE, "w+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", TEMP_LOG_FILE);
		raise(SIGINT);
	}

	
	// 동기화
	if (t_option){
		if (src_path) { // src 인자가 디렉토리인 경우
			do_tOption(src_path, dst_path);
		} else { // src 인자가 일반 파일인 경우
			do_tOption(src, dst_path);
		}
	} else {
		if (src_path) { // src 인자가 디렉토리인 경우
			syncDirectory(src_path, dst_path, r_option);
			if (m_option) do_mOption(tmp_log_fp, "", src_path, dst_path, r_option);
		} else { // src 인자가 일반 파일인 경우
			syncFile(src, dst_path);
			if (m_option) do_mOption(tmp_log_fp, "", src, dst_path, r_option);
		}
	}

	// 이제 동기화 끝났으므로 기본 핸들러로 바꾼다
	sig_act.sa_handler = SIG_DFL;
	if (sigaction(SIGINT, &sig_act, NULL) != 0) {
		fprintf(stderr, "sigaction error\n");
		raise(SIGINT);
	}

	chdir(execute_path);
	if (src_path) { // src 인자가 디렉토리인 경우
		printLog(tmp_log_fp, src_path, r_option);
	} else { // src 인자가 일반 파일인 경우
		printLog(tmp_log_fp, src, r_option);
	}

	chdir(execute_path);
	fclose(tmp_log_fp);
	unlink(TEMP_LOG_FILE);
	removeDirectory(NULL, NULL, TEMP_PATH_NAME);
	rmdir(TEMP_PATH_NAME);


	if (src_path) free(src_path);
	free(dst_path);
	free(execute_path);

	gettimeofday(&end_t, NULL); // 종료 시간 기록
	ssu_runtime(&begin_t, &end_t); // 프로그램 실행 시간 계산, 출력

	exit(0);
}

void do_tOption(const char *src_path_name, const char *dst_path_name){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;
	char tar_command[COMMAND_BUFFER_SIZE];
	const char *src_relative_path;
	int i;
	int do_sync_flag = 0;

	if (access(TAR_FILE_NAME, F_OK) == 0) unlink(TAR_FILE_NAME);

	sprintf(tar_command, "tar cvf %s ", TAR_FILE_NAME);

	if (stat(src_path_name, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src_path_name);
		raise(SIGINT);
	}

	// src가 일반 파일인 경우
	if (!S_ISDIR(statbuf.st_mode)) {
		src_relative_path = src_path_name;
		for (i = strlen(src_path_name) - 1; i >= 0; --i) {
			if (src_path_name[i] == '/') {
				src_relative_path = src_path_name + i + 1;
				break;
			}
		}

		if(checkSyncTarget(src_relative_path, dst_path_name)){
			strcat(tar_command, src_relative_path);
			system(tar_command);
			sprintf(tar_command, "tar xvf %s -C %s", TAR_FILE_NAME, dst_path_name);
			system(tar_command);
			unlink(TAR_FILE_NAME);
			return;
		} else {
			return;
		}

	}

	// src가 디렉토리인 경우
	if ((dp = opendir(src_path_name)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", src_path_name);
		raise(SIGINT);
	}

	current_path = getcwd(NULL, 0);
	chdir(src_path_name);

	if (access(TAR_FILE_NAME, F_OK) == 0) unlink(TAR_FILE_NAME);

	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			;
		} else { // 디렉토리가 아닌 파일이라면
			if(checkSyncTarget(dirp->d_name, dst_path_name)) {// 동기화 해야 하는 파일이라면
				do_sync_flag = 1;
				strcat(tar_command, dirp->d_name);
				strcat(tar_command, " ");
			}
		}
	}

	if (do_sync_flag) {
		system(tar_command);
		sprintf(tar_command, "tar xvf %s -C %s", TAR_FILE_NAME, dst_path_name);
		system(tar_command);
		unlink(TAR_FILE_NAME);
	}

	chdir(current_path);
	free(current_path);
}

void printLog(FILE *tmp_log_fp, const char *src_path_name, int sync_dir_flag){
	FILE *log_fp;
	time_t current_time;
	char *time_str;
	char *option;
	char tmp_log_buffer[BUFFER_SIZE];

	if ((log_fp = fopen(LOG_FILE_NAME, "a")) == NULL) {
	       fprintf(stderr, "fopen error for %s\n", LOG_FILE_NAME);
	       exit(1);
	}

	if (m_option) option = "-m";
	else if (r_option) option = "-r";
	else if (t_option) option = "-t";
	else option = "\0";

	current_time = time(NULL);
	time_str = ctime(&current_time);
	time_str[strlen(time_str) -1] = '\0';

	fprintf(log_fp, "[%s] ssu_rsync %s %s %s\n", time_str, option, src, dst);
	chdir(src_path_name);
	printFileNameAndSizeAtLogFile(log_fp, src_path_name, "", sync_dir_flag);
	chdir(execute_path);

	fseek(tmp_log_fp, 0, SEEK_SET);
	while (fgets(tmp_log_buffer, sizeof(tmp_log_buffer), tmp_log_fp) != NULL) {
		printf("%s", tmp_log_buffer);
		fprintf(log_fp, "%s", tmp_log_buffer);
	}

	fclose(log_fp);
}

void printFileNameAndSizeAtLogFile(FILE * fp, const char *src_path_name, const char *path_name, int sync_dir_flag){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char cur_path_buf[PATH_MAX];
	char next_src_path[PATH_MAX];
	const char *src_relative_path;
	int i;

	// src가 디렉토리가 아닌 파일이었다면 해당 파일만 출력
	if (stat(src_path_name, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src_path_name);
		exit(1);
	}

	if (!S_ISDIR(statbuf.st_mode)) {
		src_relative_path = src_path_name;
		for (i = strlen(src_path_name) - 1; i >= 0; --i) {
			if (src_path_name[i] == '/') {
				src_relative_path = src_path_name + i + 1;
				break;
			}
		}

		fprintf(fp, "\t%s %ldbytes\n", src_relative_path, statbuf.st_size);

		return;
	}


	// 디렉토리였다면 하위 모든 파일들 출력
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

		if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name);
		else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name);

		if (S_ISDIR(statbuf.st_mode) && sync_dir_flag) { // -r 옵션이 지정되었고, 디렉토리라면
			if (src_path_name[strlen(src_path_name) - 1] == '/') {
				sprintf(next_src_path, "%s%s", src_path_name, dirp->d_name);
			} else {
				sprintf(next_src_path, "%s/%s", src_path_name, dirp->d_name);
			}

			chdir(dirp->d_name);
			printFileNameAndSizeAtLogFile(fp, next_src_path, cur_path_buf, sync_dir_flag);
			chdir("..");
		} else { // 일반 파일이라면
			fprintf(fp, "\t%s %ldbytes\n", cur_path_buf, statbuf.st_size);
		}
	}
}

void do_mOption(FILE *tmp_log_fp, const char *path_name,  const char *src_path_name, const char *dst_path_name, int sync_dir_flag){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;
	const char *src_relative_path;
	char next_src_path[PATH_MAX];
	char next_dst_path[PATH_MAX];
	char *absolute_path_name_of_target_file;
	int i;
	char cur_path_buf[BUFFER_SIZE];

	if ((dp = opendir(dst_path_name)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", dst_path_name);
		raise(SIGINT);
	}

	current_path = getcwd(NULL, 0);
	chdir(dst_path_name);

	// src가 디렉토리가 아닌 파일이었다면 나머지 모든 파일 삭제해야함
	if (stat(src_path_name, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src_path_name);
		raise(SIGINT);
	}

	if (!S_ISDIR(statbuf.st_mode)) {
		// src 파일의 상대경로명 구함
		src_relative_path = src_path_name;
		for (i = strlen(src_path_name) - 1; i >= 0; --i) {
			if (src_path_name[i] == '/') {
				src_relative_path = src_path_name + i + 1;
				break;
			}
		}

		while ((dirp = readdir(dp)) != NULL) {
			if (dirp->d_ino == 0) continue;
			if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

			if (stat(dirp->d_name, &statbuf) < 0) {
				fprintf(stderr, "stat error for %s\n", dirp->d_name);
				continue;
			}

			if (tmp_log_fp) {
				if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name);
				else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name);
			}

			if (strcmp(src_relative_path, dirp->d_name)) { // 파일명과 일치하지 않는다면
				if (S_ISDIR(statbuf.st_mode)) { // 디렉토리라면
					absolute_path_name_of_target_file = realpath(dirp->d_name, NULL);
					removeDirectory(tmp_log_fp, cur_path_buf, absolute_path_name_of_target_file);
					rmdir(absolute_path_name_of_target_file);
					free(absolute_path_name_of_target_file);
				} else { // 디렉토리가 아닌 일반 파일이라면
					unlink(dirp->d_name);
					fprintf(tmp_log_fp, "\t%s delete\n", cur_path_buf);
				}
			}
		}


		return;
	}


	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (src_path_name[strlen(src_path_name) - 1] == '/') {
			sprintf(next_src_path, "%s%s", src_path_name, dirp->d_name);
		} else {
			sprintf(next_src_path, "%s/%s", src_path_name, dirp->d_name);
		}

		if (tmp_log_fp) {
			if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name);
			else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name);
		}

		if (access(next_src_path, F_OK) < 0) { // src에 존재하지 않는 파일이었다면 (동기화로 생성된 것이 아닌 파일)
		  	if (S_ISDIR(statbuf.st_mode)) { // 디렉토리라면
				absolute_path_name_of_target_file = realpath(dirp->d_name, NULL);
				removeDirectory(tmp_log_fp, cur_path_buf, absolute_path_name_of_target_file);
				rmdir(absolute_path_name_of_target_file);
				free(absolute_path_name_of_target_file);

			} else { // 일반 파일이라면
				unlink(dirp->d_name);
				fprintf(tmp_log_fp, "\t%s delete\n", cur_path_buf);
			}

			continue;
		}


		if (S_ISDIR(statbuf.st_mode)) { // 동기화 된 해당 파일이 디렉토리라면 내부의 파일들을 검사해봐야 함
			// 옵션에 따라 달라져야 할 부분
			if (sync_dir_flag) { // 하위 디렉토리도 동기화 한 경우엔 해당 디렉토리 내부의 파일들도 확인해 봐야 함
				do_mOption(tmp_log_fp, cur_path_buf, next_src_path, dirp->d_name, sync_dir_flag); // next_src_path는 위에서 생성함

			} else { // 하위 디렉토리는 동기화 하지 않은 경우 - 하위 디렉토리는 동기화 되지 않기 때문에 무조건 지워야 하는 파일임
				absolute_path_name_of_target_file = realpath(dirp->d_name, NULL);
				removeDirectory(tmp_log_fp, cur_path_buf, absolute_path_name_of_target_file);
				rmdir(absolute_path_name_of_target_file);
				free(absolute_path_name_of_target_file);

				continue;
			}
		} else { // 디렉토리가 아닌 파일이라면
			continue;
		}
	}

	chdir(current_path);
	free(current_path);

}

static void sigint_during_sync_handler(int signo){
	printf("SIGINT raised during sync\n");
	chdir(execute_path);
	removeDirectory(NULL, NULL, dst_path);
	rmdir(dst_path);
	rename(temp_dir_absolute_path, dst_path);
	removeDirectory(NULL, NULL, temp_dir_absolute_path);
	rmdir(temp_dir_absolute_path);

	gettimeofday(&end_t, NULL); // 종료 시간 기록
	ssu_runtime(&begin_t, &end_t); // 프로그램 실행 시간 계산, 출력
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
		raise(SIGINT);
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

int checkSyncTarget(const char *src_file_name, const char *dst_path_name){
	int i;
	const char *src_relative_path;
	struct stat statbuf;
	struct utimbuf utimebuf;
	char dst_file_name[PATH_MAX];
	off_t src_file_size;

	if (stat(src_file_name, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src_file_name);
		exit(1);
	}

	src_file_size = statbuf.st_size;
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
		return 1;
	} else { // dst 디렉토리에 이미 이름이 동일한 파일이 존재하는 경우
		if (stat(dst_file_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dst_file_name);
			exit(1);
		}
		if (statbuf.st_mtime == utimebuf.modtime && statbuf.st_size == src_file_size) return 0;
	}

	return 1;
}


void syncFile(const char *src_file_name, const char *dst_path_name){
	int i;
	const char *src_relative_path;
	struct stat statbuf;
	struct utimbuf utimebuf;
	char dst_file_name[PATH_MAX];
	off_t src_file_size;

	if (stat(src_file_name, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src_file_name);
		raise(SIGINT);
	}

	src_file_size = statbuf.st_size;
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
		//printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		copy(src_file_name, dst_file_name);
	} else { // dst 디렉토리에 이미 이름이 동일한 파일이 존재하는 경우
		if (stat(dst_file_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dst_file_name);
			raise(SIGINT);
		}
		if (statbuf.st_mtime == utimebuf.modtime && statbuf.st_size == src_file_size) return;

		//printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		remove(dst_file_name);
		copy(src_file_name, dst_file_name);
	}
}

void printUsage(const char *process_name){
	fprintf(stderr, "usage : %s <option> <src> <dst>\n", process_name);
}

void checkProcessArguments(int argc, char *argv[]) { // 옵션 주어졌을 경우 제대로 동작 안함
	struct stat statbuf;
	
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
		raise(SIGINT);
	}
	
	if (stat(src, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src);
		raise(SIGINT);
	}

	utimebuf.actime = statbuf.st_atime;
	utimebuf.modtime = statbuf.st_mtime;

	if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode)) < 0) {
		fprintf(stderr, "open error for %s %s\n", dst, strerror(errno));
		raise(SIGINT);
	}

	while((length = read(src_fd, buf, BUFFER_SIZE)) > 0) {
		write(dst_fd, buf, length);
	}

	utime(dst, &utimebuf);
	
	close(src_fd);
	close(dst_fd);
}

void removeDirectory(FILE *tmp_log_fp, const char *path_name, const char *target){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;
	char cur_path_buf[PATH_MAX];

	if ((dp = opendir(target)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", target);
		raise(SIGINT);
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

		if (tmp_log_fp) {
			if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name);
			else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name);
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			removeDirectory(tmp_log_fp, cur_path_buf, dirp->d_name);
			rmdir(dirp->d_name);
		} else { // 디렉토리가 아닌 파일이라면
			unlink(dirp->d_name);
			if (tmp_log_fp) fprintf(tmp_log_fp, "\t%s delete\n", cur_path_buf);
		}
	}

	chdir(current_path);
	free(current_path);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	// 시작시간과 종료시간의 차이 계산
	end_t->tv_sec -= begin_t->tv_sec;

	if(end_t->tv_usec < begin_t->tv_usec){
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}

	end_t->tv_usec -= begin_t->tv_usec;
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec); // 프로그램 실행에 걸린 시간 출력
}
