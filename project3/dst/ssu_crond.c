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

const char *CRONTAB_FILE_NAME = "ssu_crontab_file";
const char *LOG_FILE_NAME = "ssu_crontab_log";

enum {MIN, HOUR, DAY, MON, WDAY};

void startCrond();
int ssu_daemon_init(const char *path);

void checkRunCommand();
int checkValidCommand(const char *input_command);
int checkRunCycle(char *run_cycle, int run_cycle_index, int current_time);
int checkCommaCommand(char *lexeme, int run_cycle_index, int current_time);
int checkSlashCommand(char *lexeme, int run_cycle_index, int current_time);
int checkMinusCommand(char *lexeme, int run_cycle_index, int current_time, int increase);
int checkNumberAndStarCommand(char *lexeme, int current_time);
char *commaStrtok(char *start);

int main(void)
{
	//checkRunCommand(); // 디버깅용
	startCrond();

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

	struct FileData *prevFileData;
	struct FileData *curFileData;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid != 0)
		exit(0);

	pid = getpid();
	//printf("process %d running as daemon\n", pid);
	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for (fd = 0; fd < maxfd; fd++)
		close(fd);

	umask(0);
	chdir(path);
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	if (access(CRONTAB_FILE_NAME, F_OK) < 0) {
		close(open(CRONTAB_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}

	if (access(LOG_FILE_NAME, F_OK) < 0) {
		close(open(LOG_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}

	prev_minute = -1;
	while(1) {
		current_time = time(NULL);
		current_tm = localtime(&current_time);

		if (prev_minute != current_tm->tm_min) {
			prev_minute = current_tm->tm_min;
			checkRunCommand();
		}

		sleep(10);
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

	if ((command_fp = fopen(CRONTAB_FILE_NAME, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	if ((log_fp = fopen(LOG_FILE_NAME , "a")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", LOG_FILE_NAME );
		exit(1);
	}
	
	while (fgets(buf, BUFFER_SIZE, command_fp) != NULL) {
		if (feof(command_fp)) break;

		if (strlen(buf) - 1 >= 0) buf[strlen(buf) - 1] = '\0';

		for (i = 0, j = 0; i < NUMBER_OF_RUN_CYCLE_COMMANDS && j < strlen(buf); ++i) {
			while(buf[j] == ' ') ++j;
			while(buf[j] != ' ') ++j;
		}
		strncpy(run_cycle, buf, j);
		run_cycle[j] = '\0';
		while(buf[j] == ' ') ++j;
		strcpy(command, buf + j);

		if (checkValidCommand(run_cycle)) {
			system(command);

			current_time = time(NULL);
			time_str = ctime(&current_time);
			time_str[strlen(time_str) -1] = '\0';
			fprintf(log_fp, "[%s] run %s\n", time_str, command);
		}
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

	current_time = time(NULL);
	current_tm = localtime(&current_time);

	printf("min:%d, hour:%d, mday:%d, mon:%d, wday:%d\n", current_tm->tm_min, current_tm->tm_hour, current_tm->tm_mday, current_tm->tm_mon, current_tm->tm_wday);

	strcpy(copied_input_command, input_command);
	
	next_lexeme = strtok(copied_input_command, " ");
	do {
		if (lexeme_count == 5) break;

		switch(lexeme_count) {
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

		++lexeme_count;
	} while ((next_lexeme = strtok(NULL, " ")) != NULL);

	return 1;
}

int checkRunCycle(char *run_cycle, int run_cycle_index, int current_time) {
	return checkCommaCommand(run_cycle, run_cycle_index, current_time);
}

int checkCommaCommand(char *lexeme, int run_cycle_index, int current_time){
	char *ptr;

	//printf("check comma command %s\n", lexeme); /////////////////////////

	if (strstr(lexeme, ",") == NULL) {
		return checkSlashCommand(lexeme, run_cycle_index, current_time);
	} else {
		ptr = commaStrtok(lexeme);
		do {
			if (checkSlashCommand(ptr, run_cycle_index, current_time)) return 1;
		} while ((ptr = commaStrtok(NULL)) != NULL);

		return 0;
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

	if ((ptr2 = strstr(lexeme, "/")) == NULL) {
		return checkMinusCommand(lexeme, run_cycle_index, current_time, 1);
	} else {
		ptr1 = lexeme;
		*ptr2 = '\0';
		++ptr2;
		increase = atoi(ptr2);

		if (!strcmp(ptr1, "*")) {
			malloc_flag = 1;
			ptr1 = (char *) malloc(10);
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

		result = checkMinusCommand(ptr1, run_cycle_index, current_time, increase);
		if (malloc_flag) free(ptr1);

		if (result) return 1;
		else return 0;
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

	if ((ptr2 = strstr(lexeme, "-")) == NULL) {
		return checkNumberAndStarCommand(lexeme, current_time);
	} else {
		ptr1 = lexeme;
		*ptr2 = '\0';
		++ptr2;

		start = atoi(ptr1);
		end = atoi(ptr2);

		switch(run_cycle_index) {
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
			++i;
			++count;
			if (i > max_value) {
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
	if (!strcmp(lexeme, "*")) return 1;

	time = atoi(lexeme);
	if (time == current_time) return 1;

	//printf("check number end\n");//////////////////////////
	
	return 0;
}

char *commaStrtok(char *start) {
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
