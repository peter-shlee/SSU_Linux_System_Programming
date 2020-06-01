#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define MAX_COMMAND_COUNT 100

const char *PROMPT_STR = "20160548>";
const char *CRONTAB_FILE_NAME = "ssu_crontab_file";
const char *LOG_FILE_NAME = "ssu_crontab_log";

char crontab_commands[MAX_COMMAND_COUNT][BUFFER_SIZE];
int crontab_commands_count;

void printCrontabCommands(void);
void getCrontabCommands(void);
void doAddCommand(char *input_command);
void doRemoveCommand(void);
int checkValidCommand(const char *input_command);

int main(void)
{
	char input_str[BUFFER_SIZE];
	char *input_command;

	if (access(CRONTAB_FILE_NAME, F_OK) < 0) {
		fclose(fopen(CRONTAB_FILE_NAME, "w"));
	}

	getCrontabCommands();
	printCrontabCommands();
	while (1) {

		printf("%s", PROMPT_STR);
		fgets(input_str, sizeof(input_str), stdin);
		if (input_str[0] == '\n') continue;
		if (strlen(input_str) > 0) input_str[strlen(input_str) - 1] = '\0';
		else continue;

		input_command = strtok(input_str, " ");
		if (!strcmp(input_command, "add") || !strcmp(input_command, "ADD")) {
			doAddCommand(input_command);
		} else if (!strcmp(input_command, "remove") || !strcmp(input_command, "REMOVE")) {
			doRemoveCommand();
		} else if (!strcmp(input_command, "exit") || !strcmp(input_command, "EXIT")) {
			break;
		} else {
			continue;
		}

		getCrontabCommands();
		printCrontabCommands();
	}

}

void printCrontabCommands(void) {
	int i;

	for (i = 0; i < crontab_commands_count; ++i) {
		printf("%d. %s\n", i, crontab_commands[i]);
	}

	printf("\n");
}

void getCrontabCommands(void) {
	FILE *fp;
	int i;

	if ((fp = fopen(CRONTAB_FILE_NAME, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	for (i = 0; i < MAX_COMMAND_COUNT && !feof(fp); ++i) {
		if (fgets(crontab_commands[i], sizeof(crontab_commands[i]), fp) == NULL) {
			break;
		} else {
			if (strlen(crontab_commands[i]) > 0)
				crontab_commands[i][strlen(crontab_commands[i]) - 1] = '\0';
		}
	}
	crontab_commands_count = i;

	fclose(fp);

	return;
}

void doAddCommand(char *input_command){
	FILE *log_fp;
	FILE *command_fp;
	char new_command[BUFFER_SIZE];
	time_t current_time;
	char *time_str;

	// 입력된 명령어의 실행 주기가 유효한지 검사
	while(*input_command != '\0') ++input_command;
	while(*input_command == '\0') ++input_command;
	strcpy(new_command, input_command);

	if (!checkValidCommand(new_command)) return; // 실행 주기의 입력이 잘못 된 경우 에러 처리 후 프롬프트 제어가 넘어감

	if ((command_fp = fopen(CRONTAB_FILE_NAME, "a")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	fprintf(command_fp, "%s\n", new_command);
	fclose(command_fp);

	// 저장 됐으면 로그 남겨야함
	if ((log_fp = fopen(LOG_FILE_NAME, "a")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", LOG_FILE_NAME);
		exit(1);
	}

	current_time = time(NULL);
	time_str = ctime(&current_time);
	time_str[strlen(time_str) -1] = '\0';
	fprintf(log_fp, "[%s] add %s\n", time_str, new_command);
	fclose(log_fp);
}

void doRemoveCommand(void){
	int i;
	int selected_index;
	char *next_lexeme;
	FILE *command_fp;
	FILE *log_fp;
	time_t current_time;
	char *time_str;

	next_lexeme = strtok(NULL, " "); // 입력된 문자열에서 번호 추출
	if (next_lexeme == NULL) return; // 입력된 번호가 없다면 프롬프트로 제어가 넘어감
	for (i = 0; i < strlen(next_lexeme); ++i) // 숫자가 아닌 문자가 입력됐다면 프롬프트로 제어가 넘어감
		if (!isdigit(next_lexeme[i])) return;

	selected_index = atoi(next_lexeme); // 입력된 번호를 정수형으로 변환
	if (selected_index < 0 || crontab_commands_count <= selected_index) return; // 잘못된 번호가 입력되었다면 프롴프트로 제어가 넘어감

	if ((command_fp = fopen(CRONTAB_FILE_NAME, "w")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	for (i = 0; i < crontab_commands_count; ++i) {
		if (i == selected_index) continue;

		fprintf(command_fp, "%s\n", crontab_commands[i]);
	}
	fclose(command_fp);

	// 삭제 됐으면 로그 남겨야함
	if ((log_fp = fopen(LOG_FILE_NAME, "a")) == NULL) {
		fprintf(stderr, "open error for %s\n", LOG_FILE_NAME);
		exit(1);
	}

	current_time = time(NULL);
	time_str = ctime(&current_time);
	time_str[strlen(time_str) -1] = '\0';
	fprintf(log_fp, "[%s] remove %s\n", time_str, crontab_commands[selected_index]);
	fclose(log_fp);
}

int checkValidCommand(const char *input_command) {
	return 1;
}
