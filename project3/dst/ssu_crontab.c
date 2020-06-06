#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>


/////////////////////////////////////// 숫자 범위 벗어나는 경우 입력 안되도록
enum {MIN, HOUR, DAY, MON, WDAY};

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
int checkIncludeValidCharactersOnly(const char *lexeme);
int checkValidRunCycle(char *lexeme, int run_cycle_index);
int checkCommaCommand(char *lexeme, int run_cycle_index);
int checkSlashCommand(char *lexeme, int run_cycle_index);
int checkMinusCommand(char *lexeme, int run_cycle_index);
int checkNumberAndStarCommand(char *lexeme, int run_cycle_index);
char *commaStrtok(char *start);

int main(void)
{
	char input_str[BUFFER_SIZE];
	char *input_command;

	if (access(CRONTAB_FILE_NAME, F_OK) < 0) {
		close(open(CRONTAB_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}
	if (access(LOG_FILE_NAME, F_OK) < 0) {
		close(open(LOG_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
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
	char *next_lexeme;
	char copied_input_command[BUFFER_SIZE];
	int lexeme_count = 0;
	strcpy(copied_input_command, input_command);
	
	next_lexeme = strtok(copied_input_command, " ");
	do {
		if (lexeme_count == 5) break;
		if (!checkIncludeValidCharactersOnly(next_lexeme)) return 0;
		if (!checkValidRunCycle(next_lexeme, lexeme_count)) return 0;

		++lexeme_count;
		//printf("lexeme complete\n");
	} while ((next_lexeme = strtok(NULL, " ")) != NULL);

	return 1;
}

int checkIncludeValidCharactersOnly(const char *lexeme) {
	int i;
	for (i = 0; i < strlen(lexeme); ++i) {
		if (lexeme[i] == '*' || lexeme[i] == '-'|| lexeme[i] == ','|| lexeme[i] == '/' || (0 <= lexeme[i] && lexeme[i] <= '9')) continue;
		else return 0;
	}

	return 1;
}

int checkValidRunCycle(char *lexeme, int run_cycle_index) {
	//printf("check valid runcycle %s\n", lexeme); ///////////////////////
	checkCommaCommand(lexeme, run_cycle_index);
}

int checkCommaCommand(char *lexeme, int run_cycle_index){
	char *ptr;

	//printf("check comma command %s\n", lexeme); /////////////////////////

	if (strstr(lexeme, ",") == NULL) {
		return checkSlashCommand(lexeme, run_cycle_index);
	} else {
		if (lexeme[0] == ',') return 0; // 맨 앞 문자가 콤마이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == ',') return 0; // 맨 마지막 문자가 콤마이면 잘못 된 실행 주기

		ptr = commaStrtok(lexeme);
		do {
			if (!checkSlashCommand(ptr, run_cycle_index)) return 0;
		} while ((ptr = commaStrtok(NULL)) != NULL);

		return 1;
	}
}

int checkSlashCommand(char *lexeme, int run_cycle_index){
	char *ptr1;
	char *ptr2;
	int checkMinusCommandResult1;
	int checkMinusCommandResult2;

	//printf("check slash command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "/")) == NULL) {
		return checkMinusCommand(lexeme, run_cycle_index);
	} else {
		if (lexeme[0] == '/') return 0; // 맨 앞 문자가 슬래쉬이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == '/') return 0; // 맨 마지막 문자가 슬래쉬이면 잘못 된 실행 주기

		ptr1 = lexeme;
		*ptr2 = '\0';
		++ptr2;
		if ((strstr(ptr2, "/")) != NULL) return 0; // '/'문자가 여러개 있다면 잘못 된 실행 주기
		
		// '/' 뒤에는 숫자만 올 수 있다. checkMinusCommandResult2가 1이 아니라면 숫자가 아니란 뜻이므로 유효하지 않은 실행 주기이다.
		checkMinusCommandResult1 = checkMinusCommand(ptr1, run_cycle_index); 
		checkMinusCommandResult2 = checkMinusCommand(ptr2, run_cycle_index);
		if (checkMinusCommandResult1 == 1) return 0; // '/'문자 앞에는 '*' 또는 '/'를 이용한 범위가 와야 한다.
		if (checkMinusCommandResult2 != 1) return 0;

		if (checkMinusCommandResult1 * checkMinusCommandResult2) return 1; 
		else return 0;
	}
}

int checkMinusCommand(char *lexeme, int run_cycle_index){
	char *ptr1;
	char *ptr2;
	int checkNumAndStarResult;

	//printf("check minue command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "-")) == NULL) {
		return checkNumberAndStarCommand(lexeme, run_cycle_index);
	} else {
		if (lexeme[0] == '-') return 0; // 맨 앞 문자가 '-'이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == '-') return 0; // 맨 마지막 문자가 '-'이면 잘못 된 실행 주기

		ptr1 = lexeme;
		*ptr2 = '\0';
		++ptr2;
		if ((strstr(ptr2, "-")) != NULL) return 0; // '-'문자가 여러개 있다면 잘못 된 실행 주기

		// '-' 앞 뒤가 모두 숫자여야 유효한 실행 주기이다. 둘중 하나라도 '*'이면 안됨
		checkNumAndStarResult = checkNumberAndStarCommand(ptr1, run_cycle_index) * checkNumberAndStarCommand(ptr2, run_cycle_index); 
		if (checkNumAndStarResult == 1) return 3; // '/' 뒤에는 숫자만 올 수 있다. 이를 구분하기 위해 3을 리턴한다
		else return 0;
	}
}

int checkNumberAndStarCommand(char *lexeme, int run_cycle_index){
	int i;
	int num;

	//printf("check number and star command %s\n", lexeme); ///////////////////////

	if (!strcmp(lexeme, "*")) return 2; // '-' 앞뒤로 '*'가 오면 안되기 때문에, '-'에서 '*'가 있는지 판단하기 위해 '*'인 경우에는 2를 리턴한다.

	//printf("%s\n", lexeme);
	for (i = 0; i < strlen(lexeme); ++i) {
		if (!isdigit(lexeme[i])) return 0;
	}

	num = atoi(lexeme);
	switch(run_cycle_index) {
		case MIN: // 분 (0~59)
			if (num < 0 || num > 59) return 0;
			break;
		case HOUR: // 시 (0~23)
			if (num < 0 || num > 23) return 0;
			break;
		case DAY: // 일 (0~31)
			if (num < 0 || num > 31) return 0;
			break;
		case MON: // 월 (1~12)
			if (num < 1 || num > 12) return 0;
			break;
		case WDAY: // 요일 (0~6) (일요일부터 시작)
			if (num < 0 || num > 6) return 0;
			break;
		default:
			return 0;
	}
	//printf("check number end\n");//////////////////////////
	return 1;
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
