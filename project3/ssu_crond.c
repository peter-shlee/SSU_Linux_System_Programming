#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_COMMAND_COUNT 100
#define NUMBER_OF_RUN_CYCLE_COMMANDS 5

const char *CRONTAB_FILE_NAME = "ssu_crontab_file";
const char *LOG_FILE_NAME = "ssu_crontab_log";

enum {MIN, HOUR, DAY, MON, YEAR};

int checkValidCommand(const char *input_command);
int checkRunCycle(char *run_cycle, int run_cycle_index, int current_time);
int checkCommaCommand(char *lexeme, int run_cycle_index, int current_time);
int checkSlashCommand(char *lexeme, int run_cycle_index, int current_time);
int checkMinusCommand(char *lexeme, int run_cycle_index, int current_time, int increase);
int checkNumberAndStarCommand(char *lexeme, int current_time);
char *commaStrtok(char *start);

int main(void)
{
	FILE *fp;
	char buf[BUFFER_SIZE];
	char run_cycle[BUFFER_SIZE];
	char command[BUFFER_SIZE];
	int i, j;

	if ((fp = fopen(CRONTAB_FILE_NAME, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	while (fgets(buf, BUFFER_SIZE, fp) != NULL) {
		if (feof(fp)) break;

		if (strlen(buf) - 1 >= 0) buf[strlen(buf) - 1] = '\0';

		for (i = 0, j = 0; i < NUMBER_OF_RUN_CYCLE_COMMANDS && j < strlen(buf); ++i) {
			while(buf[j] == ' ') ++j;
			while(buf[j] != ' ') ++j;
		}
		strncpy(run_cycle, buf, j);
		run_cycle[j] = '\0';
		while(buf[j] == ' ') ++j;
		strcpy(command, buf + j);

		printf("run_cycle = %s|\n", run_cycle);
		printf("command = %s|\n", command);

		if (checkValidCommand(run_cycle))
			printf("execute\n");
		else
			printf("no execute\n");
		printf("\n\n");

	}

	fclose(fp);

	exit(0);
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
			case YEAR: // 요일 (0~6) (일요일부터 시작)
				if (!checkRunCycle(next_lexeme, YEAR, current_tm->tm_wday)) return 0; // 실행 x 인 경우 바로 리턴
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
		if (lexeme[0] == ',') return 0; // 맨 앞 문자가 콤마이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == ',') return 0; // 맨 마지막 문자가 콤마이면 잘못 된 실행 주기

		ptr = commaStrtok(lexeme);
		do {
			if (!checkSlashCommand(ptr, run_cycle_index, current_time)) return 0;
		} while ((ptr = commaStrtok(NULL)) != NULL);

		return 1;
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
				case YEAR: // 요일 (0~6) (일요일부터 시작)
					strcpy(ptr1, "0-6");
					break;
				default:
					return 0;
			}
		}

		result = checkMinusCommand(lexeme, run_cycle_index, current_time, increase);
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
			case YEAR: // 요일 (0~6) (일요일부터 시작)
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
