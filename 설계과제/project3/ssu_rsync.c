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
time_t exctime; // 프로세스 시작 시간 저장할 변수 (log 출력에 이용)

#define BUFFER_SIZE 1024
#define COMMAND_BUFFER_SIZE 4096 // tar 명령어 저장할 버퍼 사이즈

const char *LOG_FILE_NAME = "ssu_rsync_log";
const char *TEMP_PATH_NAME = "tmp_for_sigint_20160548"; // SIGINT 발생 시 원상 복구를 위해 기존의 파일들을 백업해 둘 디렉토리 이름
const char *TEMP_LOG_FILE = "tmp_for_delete_log_20160548"; // -m 옵션 사용 시 삭제된 파일들의 로그들을 임시로 기록해 둘 임시 파일 이름
const char *TAR_FILE_NAME = "tmptar20160548.tar"; // -t 옵션 사용 시 생성될 tar 파일의 이름
char *execute_path; // 프로세스 실행 위치의 절대경로 저장할 포인터
char *src_path; // src 디렉토리 절대 경로 저장할 포인터
char *dst_path; // dst 디렉토리 절대 경로 저장할 포인터
char *src; // 프로세스 실행 시 전달된 src 디렉토리 혹은 파일 이름 저장할 포인터
char *dst; // 프로세스 실행 시 전달된 dst 디렉토리 혹은 파일 이름 저장할 포인터
char *temp_dir_absolute_path; // 임시로 사용할 디렉토리의 절대경로 저장할 포인터

void do_tOption(const char *src_path_name, const char *dst_path_name); // -t 옵션 수행하는 함수
void do_mOption(FILE *tmp_log_fp, const char *path_name, const char *src_path_name, const char *dst_path_name, int sync_dir_flag); // -m 옵션 수행하는 함수
void printUsage(const char *process_name); // 프로세스 사용법 출력하는 함수
void checkProcessArguments(int argc, char *argv[]); // 프로세스에 전달된 인자들이 유효한지 확인하는 함수
void syncDirectory(const char *src_path_name, const char *dst_path_name, int sync_dir_flag); // 디렉토리를 동기화하는 함수
void syncFile(const char *src_file_name, const char *dst_path_name); // 파일을 동기화하는 함수
void copy(const char *src, const char *dst); // 파일을 복사하는 함수
void removeDirectory(FILE *tmp_log_fp, const char *path_name, const char *target); // 디렉토리와 그 디렉토리 하위의 모든 파일을 삭제하는 함수
void printLog(FILE *tmp_log_fp, const char *src_path_name, int sync_dir_flag); // 로그를 출력하는 함수
void printFileNameAndSizeAtLogFile(FILE * fp, const char *src_path_name, const char *path_name, int sync_dir_flag); // 동기화된 파일의 이름과 사이즈를 로그파일에 출력하는 함수
int checkSyncTarget(const char *src_file_name, const char *dst_path_name); // 동기화 대상 파일인지 확인하는 함수

int r_option;
int t_option;
int m_option;

static void sigint_during_sync_handler(int signo); // 동기화 중 발생한 SIGINT의 핸들러, 동기화를 취소하고, 기존의 내용으로 복원한다

int main(int argc, char *argv[]) { ///////// exit(1)들 sigint로 바꾸기
	FILE *tmp_log_fp; // 임시 로그 파일의 디스크립터
	struct sigaction sig_act;
	sigset_t sig_set;
	gettimeofday(&begin_t, NULL); // 시작 시간 기록
	exctime = time(NULL);

	execute_path = getcwd(NULL, 0); // 프로세스 실행 위치의 절대경로 구해서 저장
	umask(0); 
	checkProcessArguments(argc, argv); // 프로세스에 전달된 인자들이 유효한지 확인
	
	// 파일 동기화 중 SIGINT 전달 될 때를 대비한 임시 파일 저장 디렉토리 초기화
	if (chdir(TEMP_PATH_NAME) == 0) {  // 임시 파일 저장 디렉토리가 이미 존재한다면 삭제하고 다시 생성한다
		chdir(execute_path);
		removeDirectory(NULL, NULL, TEMP_PATH_NAME);
		rmdir(TEMP_PATH_NAME);
	}
	mkdir(TEMP_PATH_NAME, 0777);
	temp_dir_absolute_path = realpath(TEMP_PATH_NAME, NULL); // 임시 파일 저장 디렉토리의 절대 경로를 구해놓는다

	syncDirectory(dst_path,temp_dir_absolute_path, 1); // 기존의 동기화 디렉토리의 모든 파일들을 임시 파일 저장 디렉토리로 백업한다
	// SIGINT 핸들러 등록한다
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = 0;
	sig_act.sa_handler = sigint_during_sync_handler;
	if (sigaction(SIGINT, &sig_act, NULL) != 0) {
		fprintf(stderr, "sigaction error\n");
		exit(1);
	}

	if ((tmp_log_fp = fopen(TEMP_LOG_FILE, "w+")) == NULL) { // 임시 로그 파일 생성한다
		fprintf(stderr, "fopen error for %s\n", TEMP_LOG_FILE);
		raise(SIGINT);
	}

	
	// 동기화
	if (t_option){ // -t 옵션이 지정된 경우
		if (src_path) { // src 인자가 디렉토리인 경우
			do_tOption(src_path, dst_path); // 디렉토리에 대해 toption 수행
		} else { // src 인자가 일반 파일인 경우
			do_tOption(src, dst_path); // 파일에 대해 toption 수행
		}
	} else {
		if (src_path) { // src 인자가 디렉토리인 경우
			syncDirectory(src_path, dst_path, r_option); // 디렉토리에 대해 동기화 수행
			if (m_option) do_mOption(tmp_log_fp, "", src_path, dst_path, r_option); // -m 옵션 수행 (동기화 되지 않은 파일들 삭제)
		} else { // src 인자가 일반 파일인 경우
			syncFile(src, dst_path); // 파일에 대해 동기화 수행
			if (m_option) do_mOption(tmp_log_fp, "", src, dst_path, r_option); // -m 옵션 수행 (동기화 되지 않은 파일들 삭제)
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
		printLog(tmp_log_fp, src_path, r_option); // 로그 출력
	} else { // src 인자가 일반 파일인 경우
		printLog(tmp_log_fp, src, r_option); // 로그 출력
	}

	chdir(execute_path);
	fclose(tmp_log_fp); // 임시 로그 파일 닫는다
	unlink(TEMP_LOG_FILE); // 임시 로그 파일 삭제한다
	removeDirectory(NULL, NULL, TEMP_PATH_NAME); // 임시 백업 디렉토리 내부 모든 파일들을 삭제한다
	rmdir(TEMP_PATH_NAME); // 임시 백업 디렉토리 삭제한다

	// 기타 동적할당 했던 문자열들 해제한다
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

	if (access(TAR_FILE_NAME, F_OK) == 0) unlink(TAR_FILE_NAME); // 이전에 생성했던 tar파일이 남아있다면 삭제한다

	sprintf(tar_command, "tar cvf %s ", TAR_FILE_NAME); // tar 명령어를 만든다

	if (stat(src_path_name, &statbuf) < 0) { // 파일의 종류를 알아내기 위해 stat함수 사용
		fprintf(stderr, "stat error for %s\n", src_path_name);
		raise(SIGINT);
	}

	// src가 일반 파일인 경우
	if (!S_ISDIR(statbuf.st_mode)) {
		// src파일의 상대경로 명 구한다
		src_relative_path = src_path_name;
		for (i = strlen(src_path_name) - 1; i >= 0; --i) {
			if (src_path_name[i] == '/') {
				src_relative_path = src_path_name + i + 1;
				break;
			}
		}

		if(checkSyncTarget(src_relative_path, dst_path_name)){ // 해당 파일이 동기화 대상인지 (동일한 파일이 동기화 디렉토리 내에 없는지) 확인한다
			// tar 명령어 생성
			strcat(tar_command, src_relative_path);
			strcat(tar_command, "> /dev/null"); /////////////////////////////////////////////////
			system(tar_command);
			// 묶음 해제 명령어 생성
			sprintf(tar_command, "tar xvf %s -C %s", TAR_FILE_NAME, dst_path_name);
			strcat(tar_command, "> /dev/null"); //////////////////////////////////////////////////
			system(tar_command);
			unlink(TAR_FILE_NAME); // 사용한 tar 파일은 삭제한다
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

	if (access(TAR_FILE_NAME, F_OK) == 0) unlink(TAR_FILE_NAME); // 해당 디렉토리 내부에 이전에 생성했던 tar파일이 남아있다면 삭제한다

	while ((dirp = readdir(dp)) != NULL) { // 디렉토리 내의 모든 파일들을 확인한다
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			; // 스킵
		} else { // 디렉토리가 아닌 파일이라면
			if(checkSyncTarget(dirp->d_name, dst_path_name)) {// 동기화 해야 하는 파일이라면
				do_sync_flag = 1;
				// 명령어 뒤에 해당 파일 이름 덧붙임
				strcat(tar_command, dirp->d_name);
				strcat(tar_command, " ");
			}
		}
	}

	if (do_sync_flag) { // 동기화 해야 하는 파일이 존재한다면
		// tar 명령어 생성
		strcat(tar_command, "> /dev/null"); /////////////////////////////////////////////////
		system(tar_command);
		// 묶음 해제 명령어 생성
		sprintf(tar_command, "tar xvf %s -C %s", TAR_FILE_NAME, dst_path_name);
		strcat(tar_command, "> /dev/null"); /////////////////////////////////////////////////
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

	if ((log_fp = fopen(LOG_FILE_NAME, "a")) == NULL) { // 로그파일 오픈
	       fprintf(stderr, "fopen error for %s\n", LOG_FILE_NAME);
	       exit(1);
	}

	// 어떤 옵션이 지정되었는지 저장
	if (m_option) option = "-m";
	else if (r_option) option = "-r";
	else if (t_option) option = "-t";
	else option = "\0";

	// 현재 시간 구한다
	current_time = time(NULL);
	time_str = ctime(&current_time);
	time_str[strlen(time_str) -1] = '\0';

	fprintf(log_fp, "[%s] ssu_rsync %s %s %s\n", time_str, option, src, dst); // 로그 첫 부분에 출력할 문자열 생성
	chdir(src_path_name);
	printFileNameAndSizeAtLogFile(log_fp, src_path_name, "", sync_dir_flag); // 동기화 된 파일들의 이름과 사이즈를 출력한다
	chdir(execute_path);

	fseek(tmp_log_fp, 0, SEEK_SET); // 임시 로그파일을 확인한다
	while (fgets(tmp_log_buffer, sizeof(tmp_log_buffer), tmp_log_fp) != NULL) {
		fprintf(log_fp, "%s", tmp_log_buffer); // 임시 로그파일에 기록되어있던 삭제 로그들을 로그파일에 옮겨 출력한다
	}

	fclose(log_fp);
}

void printFileNameAndSizeAtLogFile(FILE * fp, const char *src_path_name, const char *path_name, int sync_dir_flag){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	struct stat sync_statbuf;
	char cur_path_buf[PATH_MAX];
	char next_src_path[PATH_MAX];
	char dst_path_name[PATH_MAX];
	const char *src_relative_path;
	int i;

	// src가 디렉토리가 아닌 파일이었다면 해당 파일만 출력
	if (stat(src_path_name, &statbuf) < 0) { // 파일의 종류를 알아내기 위해 stat() 호출
		fprintf(stderr, "stat error for %s\n", src_path_name);
		exit(1);
	}

	if (!S_ISDIR(statbuf.st_mode)) { // 일반 파일이라면
		// src파일의 파일명을 구한다
		src_relative_path = src_path_name;
		for (i = strlen(src_path_name) - 1; i >= 0; --i) {
			if (src_path_name[i] == '/') {
				src_relative_path = src_path_name + i + 1;
				break;
			}
		}
		
		// 동기화 된 경우에만 출력///////////////////////////////////////////////////////////
		//  path_name 을 이용하여 dst 내의 동기화된 파일의 경로를 구하고, 해당 파일의 st_atime을 확인
		// 동기화 된 파일의 경로 만든다
		if (dst_path[strlen(dst_path) - 1] == '/') {
			sprintf(dst_path_name, "%s%s", dst_path, src_relative_path);
		} else {
			sprintf(dst_path_name, "%s/%s", dst_path, src_relative_path);
		}

		if (stat(dst_path_name, &sync_statbuf) < 0) { // 동기화된 파일의 정보를 가져오기 위해 stat() 호출
			fprintf(stderr, "stat error for %s\n", dst_path_name);
			exit(1);
		}

		// 동기화 디렉토리에 파일을 동기화 할 때 동기화 했다는 사실을 표시하기 위해 atime을 프로세스 실행시 구했던 exctime으로 수정했음
		if (sync_statbuf.st_atime == exctime) { // 동기화된 파일과 exctime이 일치하면 해당 파일은 이번 실행에서 동기화된 파일임
			fprintf(fp, "\t%s %ldbytes\n", src_relative_path, statbuf.st_size); // 로그 출력
		}

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

		// 로그에 상대경로명을 출력하기 위해 상대경로 만드는 부분
		if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name); // src 디렉토리 바로 아래의 파일이라면
		else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name); // 하위 디렉토리 내의 파일이라면

		if (S_ISDIR(statbuf.st_mode) && sync_dir_flag) { // -r 옵션이 지정되었고, 디렉토리라면
			// 다음 탐색할 디렉토리의 절대경로 생성
			if (src_path_name[strlen(src_path_name) - 1] == '/') {
				sprintf(next_src_path, "%s%s", src_path_name, dirp->d_name);
			} else {
				sprintf(next_src_path, "%s/%s", src_path_name, dirp->d_name);
			}

			chdir(dirp->d_name);
			printFileNameAndSizeAtLogFile(fp, next_src_path, cur_path_buf, sync_dir_flag); // 다음 디렉토리에 대해서 재귀호출
			chdir("..");
		} else if (!S_ISDIR(statbuf.st_mode)) { // 일반 파일이라면
			// 동기화 된 경우에만 출력///////////////////////////////////////////////////////////
			// 동기화 된 파일의 절대 경로 생성
			if (dst_path[strlen(dst_path) - 1] == '/') {
				sprintf(dst_path_name, "%s%s", dst_path, cur_path_buf);
			} else {
				sprintf(dst_path_name, "%s/%s", dst_path, cur_path_buf);
			}

			if (stat(dst_path_name, &sync_statbuf) < 0) { // 동기화된 파일의 정보를 얻기 위해 stat() 호출
				fprintf(stderr, "stat error for %s\n", dst_path_name);
				exit(1);
			}

			if (sync_statbuf.st_ctime == exctime) { // 이번 프로세스 실행에서 동기화된 파일이라면
				fprintf(fp, "\t%s %ldbytes\n", cur_path_buf, statbuf.st_size); // 로그 메시지 출력
			}

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

	if ((dp = opendir(dst_path_name)) == NULL) { // dst 디렉터리 오픈
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

	// src가 아닌 일반 파일인 경우
	if (!S_ISDIR(statbuf.st_mode)) {
		// src 파일의 상대경로명 구함
		src_relative_path = src_path_name;
		for (i = strlen(src_path_name) - 1; i >= 0; --i) {
			if (src_path_name[i] == '/') {
				src_relative_path = src_path_name + i + 1;
				break;
			}
		}

		while ((dirp = readdir(dp)) != NULL) { // dst 디렉토리 내의 모든 파일에 대해서 
			if (dirp->d_ino == 0) continue;
			if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

			if (stat(dirp->d_name, &statbuf) < 0) {
				fprintf(stderr, "stat error for %s\n", dirp->d_name);
				continue;
			}

			if (tmp_log_fp) { // 삭제 로그를 출력해야 한다면(삭제 로그를 출력해야 할 때 이외에는 tmp_log_fp에 NULL 넣어서 호출)
				// 로그에 상대경로명을 출력하기 위해 상대경로 만드는 부분
				if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name); // src 디렉토리 바로 아래의 파일이라면
				else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name); // 하위 디렉토리 내의 파일이라면
			}

			if (strcmp(src_relative_path, dirp->d_name)) { // 파일명과 일치하지 않는다면
				if (S_ISDIR(statbuf.st_mode)) { // 디렉토리라면
					absolute_path_name_of_target_file = realpath(dirp->d_name, NULL); // 삭제할 디렉토리의 절대경로 구함
					removeDirectory(tmp_log_fp, cur_path_buf, absolute_path_name_of_target_file); // 해당 디렉토리 내의 모든 파일 삭제
					rmdir(absolute_path_name_of_target_file); // 해당 디렉토리 삭제
					free(absolute_path_name_of_target_file);
				} else { // 디렉토리가 아닌 일반 파일이라면
					unlink(dirp->d_name); // 파일 unlink
					fprintf(tmp_log_fp, "\t%s delete\n", cur_path_buf); // 파일 삭제 로그 출력
				}
			}
		}


		return;
	}


	// src가 디렉토리인 경우
	while ((dirp = readdir(dp)) != NULL) { // dst 디렉토리 내의 모든 파일에 대해서 
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		// src 내부의 파일의 절대경로 생성
		if (src_path_name[strlen(src_path_name) - 1] == '/') {
			sprintf(next_src_path, "%s%s", src_path_name, dirp->d_name);
		} else {
			sprintf(next_src_path, "%s/%s", src_path_name, dirp->d_name);
		}

		// 삭제 로그를 찍어야 한다면
		if (tmp_log_fp) {
			// 로그에 출력할 상대경로 생성
			if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name);
			else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name);
		}

		if (access(next_src_path, F_OK) < 0) { // src에 존재하지 않는 파일이었다면 (동기화로 생성된 것이 아닌 파일)
		  	if (S_ISDIR(statbuf.st_mode)) { // 디렉토리라면
				absolute_path_name_of_target_file = realpath(dirp->d_name, NULL); // 해당 디렉토리의 절대경로 생성
				removeDirectory(tmp_log_fp, cur_path_buf, absolute_path_name_of_target_file); // 해당 디렉토리 내의 모든 파일 삭제
				rmdir(absolute_path_name_of_target_file); // 해당 디렉토리 삭제 
				free(absolute_path_name_of_target_file);

			} else { // 일반 파일이라면
				unlink(dirp->d_name); // 해당 파일 unlink
				fprintf(tmp_log_fp, "\t%s delete\n", cur_path_buf); // 로그 출력
			}

			continue;
		}


		if (S_ISDIR(statbuf.st_mode)) { // 동기화 된 해당 파일이 디렉토리라면 내부의 파일들을 검사해봐야 함
			// 옵션에 따라 달라져야 할 부분
			if (sync_dir_flag) { // 하위 디렉토리도 동기화 한 경우엔 해당 디렉토리 내부의 파일들도 확인해 봐야 함
				do_mOption(tmp_log_fp, cur_path_buf, next_src_path, dirp->d_name, sync_dir_flag); // next_src_path는 위에서 생성함

			} else { // 하위 디렉토리는 동기화 하지 않은 경우 - 하위 디렉토리는 동기화 되지 않기 때문에 무조건 지워야 하는 파일임
				absolute_path_name_of_target_file = realpath(dirp->d_name, NULL); // 해당 디렉토리의 절대경로 생성
				removeDirectory(tmp_log_fp, cur_path_buf, absolute_path_name_of_target_file); // 해당 디렉토리 내의 모든 파일 삭제
				rmdir(absolute_path_name_of_target_file); // 해당 디렉토리 삭제
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
	printf("SIGINT raised during sync\n"); // SIGINT 전달됐다는 메시지 출력
	chdir(execute_path); // 프로세스 실행된 위치로 chdir
	removeDirectory(NULL, NULL, dst_path); // 원래 상태로 복원하기 위해 동기화 하고 있던 디렉토리 내의 모든 파일들을 삭제함
	rmdir(dst_path); // 동기화 디렉토리 삭제
	rename(temp_dir_absolute_path, dst_path); // 백업해뒀던 기존 동기화 디렉토리를 이용하여 복원함
	//removeDirectory(NULL, NULL, temp_dir_absolute_path);
	//rmdir(temp_dir_absolute_path);

	gettimeofday(&end_t, NULL); // 종료 시간 기록
	ssu_runtime(&begin_t, &end_t); // 프로그램 실행 시간 계산, 출력
	exit(0); // 프로세스 종료
}

void syncDirectory(const char *src_path_name, const char *dst_path_name, int sync_dir_flag){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;
	char next_dst_path[PATH_MAX];

	if ((dp = opendir(src_path_name)) == NULL) { // 동기화 해야 할 src 디렉토리 오픈
		fprintf(stderr, "opendir error for %s\n", src_path_name);
		raise(SIGINT);
	}

	current_path = getcwd(NULL, 0);
	chdir(src_path_name);

	while ((dirp = readdir(dp)) != NULL) { // src 디렉토리 내의 모든 파일에 대해서
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			// 옵션에 따라 달라져야 할 부분
			if (sync_dir_flag) { // -r 옵션이 지정되어 있다면
				// 동기화할 디렉토리의 절대경로명을 생성한다
				if (dst_path_name[strlen(dst_path_name) - 1] == '/') {
					sprintf(next_dst_path, "%s%s", dst_path_name, dirp->d_name);
				} else {
					sprintf(next_dst_path, "%s/%s", dst_path_name, dirp->d_name);
				}

				if (access(next_dst_path, F_OK) < 0) mkdir(next_dst_path, statbuf.st_mode); // 해당 디렉토리가 동기화 디렉토리 내에 존재하지 않는다면 새로 생성
				syncDirectory(dirp->d_name, next_dst_path, sync_dir_flag); // 해당 디렉토리에 대해 재귀호출
			} else continue;

		} else { // 디렉토리가 아닌 파일이라면
			syncFile(dirp->d_name, dst_path_name); // 해당 파일에 대해 동기화 수행
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

	// 동기화 할 파일의 정보들을 저장해둔다
	src_file_size = statbuf.st_size;
	utimebuf.actime = statbuf.st_atime;
	utimebuf.modtime = statbuf.st_mtime;

	// src 파일의 파일명 구함
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
	
	if(access(dst_file_name, F_OK) < 0) { // 동기화 디렉토리에 해당 파일이 존재하지 않는다면
		return 1;
	} else { // dst 디렉토리에 이미 이름이 동일한 파일이 존재하는 경우
		if (stat(dst_file_name, &statbuf) < 0) { // 해당 파일의 정보를 가져온다
			fprintf(stderr, "stat error for %s\n", dst_file_name);
			exit(1);
		}
		if (statbuf.st_mtime == utimebuf.modtime && statbuf.st_size == src_file_size) return 0; // 동기화 디렉토리 내의 파일과 동기화할 파일의 최종 수정시간, 파일 크기가 동일하면 같은 파일이므로 새롭게 동기화 할 필요가 없다. 따라서 0리턴
	}

	return 1; // 여기까지 왔으면 동기화 해야 하는 파일이다. 1 리턴
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

	// 해당 파일에 대한 정보들을 저장해 둔다
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
	
	if(access(dst_file_name, F_OK) < 0) { // 동기화 디렉토리에 해당 파일이 존재하지 않는다면
		//printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		copy(src_file_name, dst_file_name); // 파일 복사하여 동기화 수행
	} else { // dst 디렉토리에 이미 이름이 동일한 파일이 존재하는 경우
		if (stat(dst_file_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dst_file_name);
			raise(SIGINT);
		}
		if (statbuf.st_mtime == utimebuf.modtime && statbuf.st_size == src_file_size) return; // 동일한 파일이 이미 존재한다면 동기화가 필요 없으므로 리턴

		// 이름만 같고 서로 다른 파일이라면
		//printf("copy %s to %s\n", src_file_name, dst_file_name); ////////////////////////////////////////////////////
		remove(dst_file_name); // 동일한 이름의 기존 파일 삭제
		copy(src_file_name, dst_file_name); // 복사하여 동기화 수행
	}
}

void printUsage(const char *process_name){ 
	fprintf(stderr, "usage : %s <option> <src> <dst>\n", process_name); // 사용법 출력
}

void checkProcessArguments(int argc, char *argv[]) { // 옵션 주어졌을 경우 제대로 동작 안함
	struct stat statbuf;
	
	// 전달인자 수 잘못 됐을 때
	if (argc < 3) {
		printUsage(argv[0]);
		exit(1);
	}

	if (argv[1][0] == '-') { // 옵션이 전달됐을 때
		switch(argv[1][1]) { // 어떤 옵션인지 판별
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
				printUsage(argv[0]);// 잘못된 옵션 전달됐으면 사용법 출력하고
				exit(1); // 프로세스 종료
		}
	       	// 전달인자로 전달된 src와 dst 문자열 저장
		src = argv[2];
		dst = argv[3];
	} else {
	       	// 전달인자로 전달된 src와 dst 문자열 저장
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
	dst_path = getcwd(NULL, 0); // dst 디렉토리의 절대경로 저장
	chdir(execute_path);
}

void copy(const char *src, const char *dst) {
	int src_fd;
	int dst_fd;
	char buf[BUFFER_SIZE];
	size_t length;
	struct stat statbuf;
	struct utimbuf utimebuf;

	if ((src_fd = open(src, O_RDONLY)) < 0) { // src파일 오픈
		fprintf(stderr, "open error for %s\n", src);
		raise(SIGINT);
	}
	
	if (stat(src, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", src);
		raise(SIGINT);
	}

	// 동기화 파일에 저장할 actime 지정, 동기화 됐음을 표시하기 위하여 프로세스 시작 부분에서 저장했던 exctime을 동기화된 파일의 actime으로 지정한다
	utimebuf.actime = exctime;
	// src 파일의 최종 수정시간 가져와 저장
	utimebuf.modtime = statbuf.st_mtime;

	if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode)) < 0) { // 동기화 파일 생성
		fprintf(stderr, "open error for %s %s\n", dst, strerror(errno));
		raise(SIGINT);
	}

	while((length = read(src_fd, buf, BUFFER_SIZE)) > 0) { // 기존의 파일을 새로 생성한 동기화 파일에 복사한다
		write(dst_fd, buf, length);
	}

	utime(dst, &utimebuf); // 동기화 파일의 시간 정보를 수정한다
	
	close(src_fd);
	close(dst_fd);
}

void removeDirectory(FILE *tmp_log_fp, const char *path_name, const char *target){
	DIR *dp;
	struct dirent *dirp;
	struct stat statbuf;
	char *current_path;
	char cur_path_buf[PATH_MAX];

	if ((dp = opendir(target)) == NULL) { // 삭제할 디렉토리 open
		fprintf(stderr, "opendir error for %s\n", target);
		raise(SIGINT);
	}

	current_path = getcwd(NULL, 0);
	chdir(target);

	while ((dirp = readdir(dp)) != NULL) { // 해당 디렉토리 내의 모든 파일에 대해서 
		if (dirp->d_ino == 0) continue;
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		if (stat(dirp->d_name, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", dirp->d_name);
			continue;
		}

		if (tmp_log_fp) { // 삭제 로그를 찍어야 한다면
			// 삭제 로그에 출력할 상대 경로를 생성한다
			if (!strcmp(path_name, "")) sprintf(cur_path_buf, "%s", dirp->d_name);
			else sprintf(cur_path_buf, "%s/%s", path_name, dirp->d_name);
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리라면
			removeDirectory(tmp_log_fp, cur_path_buf, dirp->d_name); // 재귀호출하여 해당 디렉토리 내의 모든 파일을 삭제
			rmdir(dirp->d_name); // 해당 디렉토리 삭제
		} else { // 디렉토리가 아닌 파일이라면
			unlink(dirp->d_name); // 해당 파일 unlink
			if (tmp_log_fp) fprintf(tmp_log_fp, "\t%s delete\n", cur_path_buf); // 삭제 로그를 출력해야 한다면, 임시 로그 파일에 삭제 로그를 출력
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
