#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define MAX_COMMAND_COUNT 100
#define NUMBER_OF_RUN_CYCLE_COMMANDS 5

const char *CRONTAB_FILE_NAME = "ssu_crontab_file";
const char *LOG_FILE_NAME = "ssu_crontab_log";

int checkRunCommand(char *run_cycle);

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
	}

	fclose(fp);

	exit(0);
}

int checkRunCommand(char *run_cycle) {


}

int checkValidRunCycle(char *lexeme) {
	printf("check valid runcycle %s\n", lexeme); ///////////////////////
	checkCommaCommand(lexeme);
}

int checkCommaCommand(char *lexeme){
	char *ptr;

	printf("check comma command %s\n", lexeme); /////////////////////////

	if (strstr(lexeme, ",") == NULL) {
		return checkSlashCommand(lexeme);
	} else {
		if (lexeme[0] == ',') return 0; // 맨 앞 문자가 콤마이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == ',') return 0; // 맨 마지막 문자가 콤마이면 잘못 된 실행 주기

		ptr = commaStrtok(lexeme);
		do {
			if (!checkSlashCommand(ptr)) return 0;
		} while ((ptr = commaStrtok(NULL)) != NULL);

		return 1;
	}
}

int checkSlashCommand(char *lexeme){
	char *ptr1;
	char *ptr2;

	printf("check slash command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "/")) == NULL) {
		return checkMinusCommand(lexeme);
	} else {
		if (lexeme[0] == '/') return 0; // 맨 앞 문자가 슬래쉬이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == '/') return 0; // 맨 마지막 문자가 슬래쉬이면 잘못 된 실행 주기

		ptr1 = lexeme;
		*ptr2 = '\0';
		++ptr2;
		if ((strstr(ptr2, "/")) != NULL) return 0; // '/'문자가 여러개 있다면 잘못 된 실행 주기

		return checkMinusCommand(ptr1) * checkMinusCommand(ptr2);
	}
}

int checkMinusCommand(char *lexeme){
	char *ptr1;
	char *ptr2;

	printf("check minue command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "-")) == NULL) {
		return checkNumberAndStarCommand(lexeme);
	} else {
		if (lexeme[0] == '-') return 0; // 맨 앞 문자가 '-'이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == '-') return 0; // 맨 마지막 문자가 '-'이면 잘못 된 실행 주기

		ptr1 = lexeme;
		*ptr2 = '\0';
		++ptr2;
		if ((strstr(ptr2, "-")) != NULL) return 0; // '-'문자가 여러개 있다면 잘못 된 실행 주기

		return checkNumberAndStarCommand(ptr1) * checkNumberAndStarCommand(ptr2);
	}
}

int checkNumberAndStarCommand(char *lexeme){
	int i;

	printf("check number and star command %s\n", lexeme); ///////////////////////

	if (!strcmp(lexeme, "*")) return 1;

	printf("%s\n", lexeme);
	for (i = 0; i < strlen(lexeme); ++i) {
		if (!isdigit(lexeme[i])) return 0;
	}
	printf("check number end\n");//////////////////////////
	return 1;
}

char *commaStrtok(char *start) {
	static char *next_start;
	char *prev_start;
	int i;

	printf("commastrtok start\n");///////////////////

	if (start != NULL) {
		next_start = start;
		prev_start = start;
	} else {
		prev_start = next_start;
	}

	if (next_start == NULL) return NULL;

	for(i = 0; i < strlen(next_start); ++i) {
		if (next_start[i] == ',') break;
	}

	if (i < strlen(next_start)) {
		start[i] = '\0';
		if (i + 1 < strlen(next_start)) 
			next_start = next_start + i + 1;
		else next_start = NULL;
	} else {
		next_start = NULL;
	}

	printf("commastrtok end\n"); ////////////////////

	return prev_start;
}
