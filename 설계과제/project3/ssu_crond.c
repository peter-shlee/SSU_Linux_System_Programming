#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define MAX_COMMAND_COUNT 100
#define NUMBER_OF_RUN_CYCLE_COMMANDS 5

const char *CRONTAB_FILE_NAME = "ssu_crontab_file"; // crontab 명령어 저장할 파일 명
const char *LOG_FILE_NAME = "ssu_crontab_log"; // log 저장할 파일 명

enum {MIN, HOUR, DAY, MON, WDAY};

void startCrond(); // ssu_crond 실행시키는 함수
int ssu_daemon_init(const char *path); // 디몬 프로세스 생성하는 함수

void checkRunCommand(); // 실행시킬 명령어 있는지 확인하는 함수
int checkValidCommand(const char *input_command); // 실행 주기를 확인해 실행시킬 시간인지 확인하는 함수
int checkRunCycle(char *run_cycle, int run_cycle_index, int current_time); // 실행주기 확인하는 함수
int checkCommaCommand(char *lexeme, int run_cycle_index, int current_time); //',' 이용한 실행주기를 확인해 실행해야 할 명령어인지 확인하는 함수
int checkSlashCommand(char *lexeme, int run_cycle_index, int current_time); //'/' 이용한 실행주기를 확인해 실행해야 할 명령어인지 확인하는 함수
int checkMinusCommand(char *lexeme, int run_cycle_index, int current_time, int increase); //'-' 이용한 실행주기를 확인해 실행해야 할 명령어인지 확인하는 함수
int checkNumberAndStarCommand(char *lexeme, int current_time); //'*', 숫자를 이용한 실행주기를 확인해 실행해야 할 명령어인지 확인하는 함수
char *commaStrtok(char *start); // ',' 이용한 실행주기에서 각각의 실행주기 분리해낼 때 사용하는 함수

int main(void)
{
	//checkRunCommand(); // 디버깅용
	startCrond(); // crond 프로세스 시작

	exit(0);
}

void startCrond(){
	pid_t pid;
	if ((pid = fork()) == 0) { // 자식 프로세스 생성, 자식 프로세스라면
		char path[PATH_MAX];
		getcwd(path, PATH_MAX); // 현재 경로 구한다
		ssu_daemon_init(path); // 디몬 프로세스 실행
	} else if (pid < 0) { // fork 에러라면
		fprintf(stderr, "mntr starting error\n");
		exit(1);
	} else { // 부모 프로세스라면
		return;
	}
}

int ssu_daemon_init(const char *path) { // 디몬 프로세스 시작하는 함수
	pid_t pid;
	int fd, maxfd;
	time_t current_time;
	struct tm *current_tm;
	int prev_minute;

	if ((pid = fork()) < 0) { // 백그라운드 프로세스가 된다
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid != 0)
		exit(0);

	setsid(); // 새로운 프로세스 그룹 생성
	signal(SIGTTIN, SIG_IGN); // 터미널 입출력 시그널 무시
	signal(SIGTTOU, SIG_IGN); // 터미널 입출력 시그널 무시
	signal(SIGTSTP, SIG_IGN); // 터미널 입출력 시그널 무시
	maxfd = getdtablesize(); // 최대 디스크립터 개수 구함

	for (fd = 0; fd < maxfd; fd++) // 모든 디스크립터 close
		close(fd);

	umask(0); // 파일 모드 생성 마스크 해제
	chdir(path); // 디몬 프로세스 실행할 경로로 이동
	fd = open("/dev/null", O_RDWR); // 표준 입출력, 에러 재지정
	dup(0); // 표준 입출력, 에러 재지정
	dup(0); // 표준 입출력, 에러 재지정

	if (access(CRONTAB_FILE_NAME, F_OK) < 0) { // crontab 명령어 저장된 파일 없다면 생성
		close(open(CRONTAB_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}

	if (access(LOG_FILE_NAME, F_OK) < 0) { // crontab 로그 저장된 파일 없다면 생성
		close(open(LOG_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}

	prev_minute = -1;
	while(1) {
		// 현재 시간 구한다
		current_time = time(NULL);
		current_tm = localtime(&current_time);

		if (prev_minute != current_tm->tm_min) { // 1분마다 실행할 명령어 있는지 확인
			prev_minute = current_tm->tm_min;
			checkRunCommand();
		}

		sleep(10); // 10초씩 sleep하면서 시간 확인
	}

	return 0;
}

void checkRunCommand() {
	FILE *command_fp;
	FILE *log_fp;
	char buf[BUFFER_SIZE];
	char run_cycle[BUFFER_SIZE];
	char command[BUFFER_SIZE];
	int i, j;
	time_t current_time;
	char *time_str;
	struct flock fl;

	if ((command_fp = fopen(CRONTAB_FILE_NAME, "r")) == NULL) { // 실행할 명령어 저장된 파일 오픈
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	if ((log_fp = fopen(LOG_FILE_NAME , "a")) == NULL) { // 로그 저장할 파일 오픈
		fprintf(stderr, "fopen error for %s\n", LOG_FILE_NAME );
		exit(1);
	}

	// ssu_crontab 프로세스에서 파일을 수정할 수 있으므로 명령어 저장된 파일에 읽기 락을 건다
	fl.l_type = F_RDLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fileno(command_fp), F_SETLKW, &fl) < 0) {
		fprintf(stderr, "fcntl error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}
	
	while (fgets(buf, BUFFER_SIZE, command_fp) != NULL) { // 파일에서 명령어 하나씩 읽어온다
		if (feof(command_fp)) break;

		if (strlen(buf) - 1 >= 0) buf[strlen(buf) - 1] = '\0';

		// 실행 주기 명령어들 pass
		for (i = 0, j = 0; i < NUMBER_OF_RUN_CYCLE_COMMANDS && j < strlen(buf); ++i) {
			while(buf[j] == ' ') ++j;
			while(buf[j] != ' ') ++j;
		}
		strncpy(run_cycle, buf, j); // 실행주기 명령어들 따로 저장
		run_cycle[j] = '\0';
		while(buf[j] == ' ') ++j; // 실행할 명령어로 인덱스 이동
		strcpy(command, buf + j); // 실행할 명령어 따로 저장

		if (checkValidCommand(run_cycle)) { // 현재 실행해야 할 실행주기인지 확인
			system(command); // 현재 실행할 실행주기라면 명령어 실행

			current_time = time(NULL);
			time_str = ctime(&current_time);
			time_str[strlen(time_str) -1] = '\0';
			fprintf(log_fp, "[%s] run %s\n", time_str, command); // 실행 했으면 로그 출력
		}
	}

	// 명령어 확인 끝났으면 락 해제
	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fileno(command_fp), F_SETLKW, &fl) < 0) {
		fprintf(stderr, "fcntl error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	fclose(command_fp);
	fclose(log_fp);
}

int checkValidCommand(const char *input_command) {
	char *next_lexeme;
	char copied_input_command[BUFFER_SIZE];
	int lexeme_count = 0;
	time_t current_time;
	struct tm *current_tm;

	current_time = time(NULL); // 현재 시간 가져옴
	current_tm = localtime(&current_time);

	//printf("min:%d, hour:%d, mday:%d, mon:%d, wday:%d\n", current_tm->tm_min, current_tm->tm_hour, current_tm->tm_mday, current_tm->tm_mon, current_tm->tm_wday);

	strcpy(copied_input_command, input_command);
	
	next_lexeme = strtok(copied_input_command, " "); // 첫번째 실행 주기 가져옴
	do {
		if (lexeme_count == 5) break; // 실행주기 5개 모두 확인했다면 반복 종료

		switch(lexeme_count) { // 실행 주기별 확인
			case MIN: // 분 (0~59)
				if (!checkRunCycle(next_lexeme, MIN, current_tm->tm_min)) return 0; // 실행 x 인 경우 바로 리턴
				break;
			case HOUR: // 시 (0~23)
				if (!checkRunCycle(next_lexeme, HOUR, current_tm->tm_hour)) return 0; // 실행 x 인 경우 바로 리턴
				break;
			case DAY: // 일 (0~31)
				if (!checkRunCycle(next_lexeme, DAY, current_tm->tm_mday)) return 0; // 실행 x 인 경우 바로 리턴
				break;
			case MON: // 월 (1~12)
				if (!checkRunCycle(next_lexeme, MON, current_tm->tm_mon + 1)) return 0; // 실행 x 인 경우 바로 리턴
				break;
			case WDAY: // 요일 (0~6) (일요일부터 시작)
				if (!checkRunCycle(next_lexeme, WDAY, current_tm->tm_wday)) return 0; // 실행 x 인 경우 바로 리턴
				break;
			default:
				return 0;
		}

		++lexeme_count; // 확인한 실행 주기 개수 ++
	} while ((next_lexeme = strtok(NULL, " ")) != NULL); // 다음 실행 주기 가져옴

	return 1;
}

int checkRunCycle(char *run_cycle, int run_cycle_index, int current_time) { // 없어도 되는 함수...
	return checkCommaCommand(run_cycle, run_cycle_index, current_time);
}

int checkCommaCommand(char *lexeme, int run_cycle_index, int current_time){
	char *ptr;

	//printf("check comma command %s\n", lexeme); /////////////////////////

	if (strstr(lexeme, ",") == NULL) { // 실행 주기에 ','가 없다면
		return checkSlashCommand(lexeme, run_cycle_index, current_time); // '/' 확인하러 간다
	} else {
		ptr = commaStrtok(lexeme); // ',' 기준으로 실행 주기 분리
		do {
			if (checkSlashCommand(ptr, run_cycle_index, current_time)) return 1; // ','은 or 이므로 실행주기 중 하나라도 유효하면 바로 1 리턴
		} while ((ptr = commaStrtok(NULL)) != NULL); // 다음 실행주기 확인

		return 0; // 여기까지 왔다면 유효하지 않은 실행주기임
	}
}

int checkSlashCommand(char *lexeme, int run_cycle_index, int current_time){
	char *ptr1;
	char *ptr2;
	int increase;
	int i;
	int malloc_flag = 0;
	int result;

	//printf("check slash command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "/")) == NULL) { // 실행 주기에 '/'가 없다면 
		return checkMinusCommand(lexeme, run_cycle_index, current_time, 1); // '-' 확인하러 간다
	} else {
		ptr1 = lexeme; // '/' 앞쪽의 실행 주기는 ptr1에 
		*ptr2 = '\0';
		++ptr2; // '/' 뒤의 실행 주기는 ptr2에 
		increase = atoi(ptr2); // '/' 뒤의 실행주기(무조건 숫자)를 정수형으로 변환하여 저장, 이 숫자를 이용해 '-'를 확인하면서 유효한 실행 주기인지 확인한다

		if (!strcmp(ptr1, "*")) { // '/'앞의 실행주기가 '*' 이라면
			malloc_flag = 1;
			ptr1 = (char *) malloc(10);
			// '*'을 각 실행주기에 맞는 범위로 변환한다
			switch(run_cycle_index) {
				case MIN: // 분 (0~59)
					strcpy(ptr1, "0-59");
					break;
				case HOUR: // 시 (0~23)
					strcpy(ptr1, "0-23");
					break;
				case DAY: // 일 (0~31)
					strcpy(ptr1, "0-31");
					break;
				case MON: // 월 (1~12)
					strcpy(ptr1, "1-12");
					break;
				case WDAY: // 요일 (0~6) (일요일부터 시작)
					strcpy(ptr1, "0-6");
					break;
				default:
					return 0;
			}
		}

		result = checkMinusCommand(ptr1, run_cycle_index, current_time, increase); // '-' 확인한다
		if (malloc_flag) free(ptr1);

		if (result) return 1; // 유효하면 1 리턴
		else return 0; // 유효하지 않으면 0 리턴
	}
}

int checkMinusCommand(char *lexeme, int run_cycle_index, int current_time, int increase){
	char *ptr1;
	char *ptr2;
	int start;
	int end;
	int max_value;
	int i;
	int count;

	//printf("check minue command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "-")) == NULL) { // '-'가 실행주기에 들어있지 않다면
		return checkNumberAndStarCommand(lexeme, current_time); // 다음 단계로 이동
	} else {
		ptr1 = lexeme; // '-' 앞쪽의 실행 주기 ptr1에
		*ptr2 = '\0';
		++ptr2; // '-' 뒤의 실행 주기 ptr2에

		start = atoi(ptr1); // 앞쪽 실행 주기 정수형으로 변환
		end = atoi(ptr2); // 뒤쪽 실행 주기 정수형으로 변환

		switch(run_cycle_index) { // 실행 주기 확인을 위해 각 실행 주기 별 최대값 저장
			case MIN: // 분 (0~59)
				max_value = 59;
				break;
			case HOUR: // 시 (0~23)
				max_value = 23;
				break;
			case DAY: // 일 (0~31)
				max_value = 31;
				break;
			case MON: // 월 (1~12)
				max_value = 12;
				break;
			case WDAY: // 요일 (0~6) (일요일부터 시작)
				max_value = 6;
				break;
			default:
				return 0;
		}

		i = start - 1;
		count = 0;
		do {
			++i; // '-'앞쪽의 시간부터 뒤쪽의 시간까지 이동하며 확인
			++count; // '/' 뒤쪽의 실행주기를 이용해 명령어 실행할 순서인지 확인하는데 사용
			if (i > max_value) { // i 가 실행주기 최댓값을 넘어섰다면
				// 최솟값으로 바꿔준다
				if (run_cycle_index == MON) i = 1;
				else i = 0;
			}

			if (count % increase == 0 && i == current_time) return 1;
		} while (i != end);

		return 0;
	}
}

int checkNumberAndStarCommand(char *lexeme, int current_time){
	int time;

	//printf("check number and star command %s\n", lexeme); ///////////////////////
	if (!strcmp(lexeme, "*")) return 1; // '*'이면 항상 유효하므로 1 리턴

	time = atoi(lexeme);
	if (time == current_time) return 1; // 숫자가 현재 시간과 일치하면 유효하므로 1 리턴

	//printf("check number end\n");//////////////////////////
	
	return 0;
}

char *commaStrtok(char *start) { // ssu_crontab의 commaStrtok함수와 동일
	static char *next_start;
	char *prev_start;
	int i;
	int length;

	//printf("commastrtok start\n");///////////////////

	if (start != NULL) {
		next_start = start;
		prev_start = start;
	} else {
		prev_start = next_start;
	}

	if (next_start == NULL) return NULL;

	length = strlen(next_start);
	for(i = 0; i < length; ++i) {
		if (next_start[i] == ',') break;
	}

	if (i < length) {
		next_start[i] = '\0';
		if (i + 1 < length) 
			next_start = next_start + i + 1;
		else next_start = NULL;
	} else {
		next_start = NULL;
	}

	//printf("commastrtok end\n"); ////////////////////

	return prev_start;
}
