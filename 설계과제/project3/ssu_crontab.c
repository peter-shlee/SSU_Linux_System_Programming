#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>

#define SECOND_TO_MICRO 1000000
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t); // 수행시간 계산 함수

enum {MIN, HOUR, DAY, MON, WDAY};

#define BUFFER_SIZE 1024
#define MAX_COMMAND_COUNT 100

const char *PROMPT_STR = "20160548>"; // 프롬프트에 출력할 문자열
const char *CRONTAB_FILE_NAME = "ssu_crontab_file"; // crontab 명령어 저장할 파일 명
const char *LOG_FILE_NAME = "ssu_crontab_log"; // log 저장할 파일 명

char crontab_commands[MAX_COMMAND_COUNT][BUFFER_SIZE]; // crontab 명령어를 불러와 저장할 버퍼
int crontab_commands_count; // crontab 명령어 개수를 저장할 변수

void printCrontabCommands(void); // 저장된 명령어들을 읽어와 프롬프트에 출력하는 함수
void getCrontabCommands(void); // 파일에 저장된 명령어들을 읽어오는 함수
void doAddCommand(char *input_command); // 명령어를 추가하는 함수
void doRemoveCommand(void); // 명령어를 삭제하는 함수
int checkValidCommand(const char *input_command); // 유효한 명령어인지 확인하는 함수
int checkIncludeValidCharactersOnly(const char *lexeme); // 사용할 수 없는 문자가 명령어에 포함되어있는지 확인하는 함수
int checkValidRunCycle(char *lexeme, int run_cycle_index); // 실행 주기가 유효한 명령어인지 확인하는 함수
int checkCommaCommand(char *lexeme, int run_cycle_index); // 실행 주기 문자 ','가 제대로 쓰였는지 확인하는 함수
int checkSlashCommand(char *lexeme, int run_cycle_index);// 실행 주기 문자 '/'가 제대로 쓰였는지 확인하는 함수
int checkMinusCommand(char *lexeme, int run_cycle_index);// 실행 주기 문자 '-'가 제대로 쓰였는지 확인하는 함수
int checkNumberAndStarCommand(char *lexeme, int run_cycle_index);// 실행 주기 문자 '*'와 숫자가 제대로 쓰였는지 확인하는 함수
char *commaStrtok(char *start); // ',' 문자를 검사할 때 사용하기 위해 만든 수행되는 strtok 함수, ','만 토큰으로 인식한다는 것 외에는 strtok함수와 동일하게 작동함

int main(void)
{
	char input_str[BUFFER_SIZE]; // 사용자가 프롬프트에 입력한 명령어를 저장할 배열
	char *input_command; // 사용자가 입력한 명령어를 strtok으로 나누어 확인 할 때 사용하는 포인터
	struct timeval begin_t, end_t; // 프로세스 수행시간 측정을 위해 사용
	gettimeofday(&begin_t, NULL); // 시작 시간 기록

	if (access(CRONTAB_FILE_NAME, F_OK) < 0) { // ssu_crontab_file이 존재하지 않으면 새로 생성
		close(open(CRONTAB_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}
	if (access(LOG_FILE_NAME, F_OK) < 0) {  // ssu_crontab_log 파일이 존재하지 않으면 새로 생성 
		close(open(LOG_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666));
	}

	getCrontabCommands(); // 파일에서 기존에 저장되어 있던 명령어들을 읽어옴
	printCrontabCommands(); // 저장되어 있던 명령어들을 프롬프트에 출력함
	while (1) { // 사용자가 exit 입력해 프로세스 종료시킬 때 까지 반복

		printf("%s", PROMPT_STR); // 프롬프트 문자열 출력
		fgets(input_str, sizeof(input_str), stdin); // 사용자에게 명령어 입력받음
		if (input_str[0] == '\n') continue; // 사용자가 엔터 입력했을 시에는 다시 프롬프트 문자열 출력
		if (strlen(input_str) > 0) input_str[strlen(input_str) - 1] = '\0'; // fgets로 문자열 읽어오면 맨 끝에 개행문자가 남아있으므로 개행을 없애고 널문자로 바꿈
		else continue;

		input_command = strtok(input_str, " "); // 사용자가 어떤 명령어를 입력했는지 확인하기 위해 strtok 사용
		if (!strcmp(input_command, "add") || !strcmp(input_command, "ADD")) { // 사용자가 add 명령어를 사용했다면
			doAddCommand(input_command); // 명령어 추가
		} else if (!strcmp(input_command, "remove") || !strcmp(input_command, "REMOVE")) { // 사용자가 remove 명령어를 사용했다면
			doRemoveCommand(); // 명령어 삭제
		} else if (!strcmp(input_command, "exit") || !strcmp(input_command, "EXIT")) { // 사용자가 exit 명령어를 사용했다면
			break; // 프로세스 종료를 위해 반복문 빠져나감
		} else {
			continue; // 인식할 수 없는 명령어라면 다시 프롬프트 문자열 출력
		}

		getCrontabCommands(); // 파일에 저장된 명령어들 읽어옴
		printCrontabCommands(); // 파일에 저장된 명령어들을 프롬프트에 출력함
	}

	gettimeofday(&end_t, NULL); // 종료 시간 기록
	ssu_runtime(&begin_t, &end_t); // 프로그램 실행 시간 계산, 출력

	exit(0); // 프로세스 종료
}

void printCrontabCommands(void) {
	int i;

	for (i = 0; i < crontab_commands_count; ++i) { // 명령어 개수만큼 반복
		printf("%d. %s\n", i, crontab_commands[i]); // 명령어들 저장해 놓은 배열을 이용해 명령어를 화면에 출력한다
	}

	printf("\n");
}

void getCrontabCommands(void) {
	FILE *fp;
	int i;

	if ((fp = fopen(CRONTAB_FILE_NAME, "r")) == NULL){ // 명령어 저장된 파일 오픈
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	for (i = 0; i < MAX_COMMAND_COUNT && !feof(fp); ++i) { // 명령어 읽어올 수 있는 최대 개수만큼 읽는다
		if (fgets(crontab_commands[i], sizeof(crontab_commands[i]), fp) == NULL) { // 파일에서 한줄(명령어 하나) 읽어옴
			break; // 아무것도 읽어온게 없다면 명령어를 다 읽어왔다는 뜻이므로 반복 종료
		} else {
			if (strlen(crontab_commands[i]) > 0)
				crontab_commands[i][strlen(crontab_commands[i]) - 1] = '\0'; // 명령어 맨 끝의 개행문자 없앤다
		}
	}
	crontab_commands_count = i; // 읽어온 명령어 개수 저장

	fclose(fp);

	return;
}

void doAddCommand(char *input_command){
	FILE *log_fp;
	FILE *command_fp;
	char new_command[BUFFER_SIZE];
	time_t current_time;
	char *time_str;
	struct flock fl;

	// 입력된 명령어의 실행 주기가 유효한지 검사
	while(*input_command != '\0') ++input_command; // 명령어 앞쪽의 불필요한 문자, 널문자 pass
	while(*input_command == '\0') ++input_command;
	strcpy(new_command, input_command);

	if (!checkValidCommand(new_command)) return; // 실행 주기의 입력이 잘못 된 경우 에러 처리 후 프롬프트 제어가 넘어감

	// 유효한 명령어인 경우
	if ((command_fp = fopen(CRONTAB_FILE_NAME, "a")) == NULL) { // 명령어 저장하는 파일 오픈
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	// 해당 파일은 ssu_crond와 함께 쓰기 때문에 오류 발생하지 않기 위해 쓰기 락을 걸어준다
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fileno(command_fp), F_SETLKW, &fl) < 0) {
		fprintf(stderr, "fcntl error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	fprintf(command_fp, "%s\n", new_command); // 파일에 새로운 명령어를 추가한다

	// 쓰기 작업 끝났으므로 락 해제한다
	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fileno(command_fp), F_SETLKW, &fl) < 0) {
		fprintf(stderr, "fcntl error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	fclose(command_fp);

	// 명령어 추가 됐으면 로그 남긴다
	if ((log_fp = fopen(LOG_FILE_NAME, "a")) == NULL) { // 로그 파일 오픈
		fprintf(stderr, "fopen error for %s\n", LOG_FILE_NAME);
		exit(1);
	}

	current_time = time(NULL);
	time_str = ctime(&current_time);
	time_str[strlen(time_str) -1] = '\0'; // ctime으로 리턴된 시간 문자열 맨 끝의 개행문제 지운다
	fprintf(log_fp, "[%s] add %s\n", time_str, new_command); // 로그 메시지 출력
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
	struct flock fl;

	next_lexeme = strtok(NULL, " "); // 입력된 문자열에서 삭제할 명령어 번호 추출
	if (next_lexeme == NULL) return; // 입력된 번호가 없다면 프롬프트로 제어가 넘어감
	for (i = 0; i < strlen(next_lexeme); ++i) // 숫자가 아닌 문자가 입력됐다면 프롬프트로 제어가 넘어감
		if (!isdigit(next_lexeme[i])) return;

	selected_index = atoi(next_lexeme); // 입력된 번호를 정수형으로 변환
	if (selected_index < 0 || crontab_commands_count <= selected_index) return; // (인덱스 번호를 벗어난) 잘못된 번호가 입력되었다면 프롬프트로 제어가 넘어감

	if ((command_fp = fopen(CRONTAB_FILE_NAME, "w")) == NULL) { // 명령어 저장된 파일 오픈
		fprintf(stderr, "fopen error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	// 쓰기 락을 건다
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fileno(command_fp), F_SETLKW, &fl) < 0) {
		fprintf(stderr, "fcntl error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	for (i = 0; i < crontab_commands_count; ++i) { // 삭제할 명령어를 뺀 모든 명령어들을 파일에 다시 쓴다
		if (i == selected_index) continue; // 삭제할 명령어 차례이면 skip 한다

		fprintf(command_fp, "%s\n", crontab_commands[i]); // 파일에 명령어 출력
	}

	// 락 해제
	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	if (fcntl(fileno(command_fp), F_SETLKW, &fl) < 0) {
		fprintf(stderr, "fcntl error for %s\n", CRONTAB_FILE_NAME);
		exit(1);
	}

	fclose(command_fp);

	// 삭제 됐으면 로그 남긴다
	if ((log_fp = fopen(LOG_FILE_NAME, "a")) == NULL) {
		fprintf(stderr, "open error for %s\n", LOG_FILE_NAME);
		exit(1);
	}

	current_time = time(NULL);
	time_str = ctime(&current_time);
	time_str[strlen(time_str) -1] = '\0';
	fprintf(log_fp, "[%s] remove %s\n", time_str, crontab_commands[selected_index]); // 로그 메시지 출력
	fclose(log_fp);
}

int checkValidCommand(const char *input_command) {
	char *next_lexeme;
	char copied_input_command[BUFFER_SIZE];
	int lexeme_count = 0;
	strcpy(copied_input_command, input_command);
	
	next_lexeme = strtok(copied_input_command, " "); // 맨 앞의 실행 주기 가져온다
	do {
		if (lexeme_count == 5) break; // 실행 주기 5개 모두 검사했으면 반복 종료
		if (!checkIncludeValidCharactersOnly(next_lexeme)) return 0; // 실행 주기에 유효한 문자만 포함되어 있는지 검사, 유효하지 않은 명령어이면 0 리턴
		if (!checkValidRunCycle(next_lexeme, lexeme_count)) return 0; // 유효한 실행 주기인지 검사, 유효하지 않은 명령어이면 0 리턴

		++lexeme_count; // 검사한 실행 주기 개수 ++
		//printf("lexeme complete\n");
	} while ((next_lexeme = strtok(NULL, " ")) != NULL); // 다음 실행주기 가져온다

	return 1; // 유효한 명령어이면 1 리턴
}

int checkIncludeValidCharactersOnly(const char *lexeme) {
	int i;
	for (i = 0; i < strlen(lexeme); ++i) { // 인자로 전달된 문자열의 모든 문자를 확인
		if (lexeme[i] == '*' || lexeme[i] == '-'|| lexeme[i] == ','|| lexeme[i] == '/' || (0 <= lexeme[i] && lexeme[i] <= '9')) continue; // 유효한 문자라면 다음 문자 확인
		else return 0; // 유효하지 않은 문자가 포함되어있다면 0 리턴
	}

	return 1; // 유효한 문자만 들어있다면 1 리턴
}

int checkValidRunCycle(char *lexeme, int run_cycle_index) { // 없어도 되는 함수...
	//printf("check valid runcycle %s\n", lexeme); ///////////////////////
	return checkCommaCommand(lexeme, run_cycle_index);
}

int checkCommaCommand(char *lexeme, int run_cycle_index){
	char *ptr;

	//printf("check comma command %s\n", lexeme); /////////////////////////

	if (strstr(lexeme, ",") == NULL) { // 실행 주기에 ','가 들어있는지 확인
		return checkSlashCommand(lexeme, run_cycle_index); // 들어있지 않다면 바로 '/' 문자가 유효한지 검사하러 간다
	} else {
		if (lexeme[0] == ',') return 0; // 맨 앞 문자가 콤마이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == ',') return 0; // 맨 마지막 문자가 콤마이면 잘못 된 실행 주기

		ptr = commaStrtok(lexeme); // ','를 토큰으로 해서 실행주기를 분리한다
		do {
			if (!checkSlashCommand(ptr, run_cycle_index)) return 0; // '/'문자가 유효한지 확인한다, 유효하지 않으면 0을 리턴한다
		} while ((ptr = commaStrtok(NULL)) != NULL); // ','를 토큰으로 해서 실행주기를 분리한다

		return 1; // 유효하면 1을 리턴한다
	}
}

int checkSlashCommand(char *lexeme, int run_cycle_index){
	char *ptr1;
	char *ptr2;
	int checkMinusCommandResult1;
	int checkMinusCommandResult2;

	//printf("check slash command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "/")) == NULL) { // 실행 주기에 '/'문자가 들어있지 않다면
		return checkMinusCommand(lexeme, run_cycle_index); // '-' 바로 문자가 유효한지 확인하러 간다
	} else {
		if (lexeme[0] == '/') return 0; // 맨 앞 문자가 슬래쉬이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == '/') return 0; // 맨 마지막 문자가 슬래쉬이면 잘못 된 실행 주기

		ptr1 = lexeme; // '/' 앞의 실행 주기는 ptr1에
		*ptr2 = '\0';
		++ptr2; // '/' 뒤의 실행 주기는 ptr2에
		if ((strstr(ptr2, "/")) != NULL) return 0; // '/'문자가 여러개 있다면 잘못 된 실행 주기
		
		// '/' 뒤에는 숫자만 올 수 있다. checkMinusCommandResult2가 1이 아니라면 숫자가 아니란 뜻이므로 유효하지 않은 실행 주기이다.
		checkMinusCommandResult1 = checkMinusCommand(ptr1, run_cycle_index); // '/' 앞의 실행 주기가 유효한지 확인해 그 결과를 저장
		checkMinusCommandResult2 = checkMinusCommand(ptr2, run_cycle_index); // '/' 뒤의 실행 주기가 유효한지 확인해 그 결과를 저장
		if (checkMinusCommandResult1 == 1) return 0; // '/'문자 앞에는 '*' 또는 '/'를 이용한 범위가 와야 한다.
		if (checkMinusCommandResult2 != 1) return 0; // '/' 뒤에는 반드시 숫자가 와야 한다

		if (checkMinusCommandResult1 * checkMinusCommandResult2) return 1;  // '/' 앞뒤 실행주기가 모두 유효하면 1 리턴
		else return 0;
	}
}

int checkMinusCommand(char *lexeme, int run_cycle_index){
	char *ptr1;
	char *ptr2;
	int checkNumAndStarResult;

	//printf("check minue command %s\n", lexeme); ///////////////////////

	if ((ptr2 = strstr(lexeme, "-")) == NULL) { // 실행주기에 '-'가 들어있지 않다면 바로 다음 단계 확인하러 감
		return checkNumberAndStarCommand(lexeme, run_cycle_index);
	} else {
		if (lexeme[0] == '-') return 0; // 맨 앞 문자가 '-'이면 잘못 된 실행 주기
		if (lexeme[strlen(lexeme) - 1] == '-') return 0; // 맨 마지막 문자가 '-'이면 잘못 된 실행 주기

		ptr1 = lexeme; // '-'앞의 실행주기는 ptr1에
		*ptr2 = '\0';
		++ptr2; // '-'뒤의 실행주기는 ptr1에
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
		if (!isdigit(lexeme[i])) return 0; // '*'도 아니고, 숫자도 아니면 잘못된 실행주기이므로 0리턴
	}

	num = atoi(lexeme);
	switch(run_cycle_index) { // 각 실행 주기 별 유효한 범위 내에 있는지 검사
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
	return 1; // 유효하다면 1 리턴
}

char *commaStrtok(char *start) {
	static char *next_start;
	char *prev_start;
	int i;
	int length;

	//printf("commastrtok start\n");///////////////////

	if (start != NULL) { // 전달인자가 null이 아니라면 새로운 문자열에 대한 함수 호출
		next_start = start;
		prev_start = start;
	} else { // 기존의 문자열에 대한 함수 호출
		prev_start = next_start;
	}

	if (next_start == NULL) return NULL; // 문자열 끝까지 확인 마쳤다면 NULL 리턴

	length = strlen(next_start);
	for(i = 0; i < length; ++i) {
		if (next_start[i] == ',') break; // ',' 토큰 찾는다
	}

	if (i < length) {
		next_start[i] = '\0'; // 토큰을 널문자로 바꾼다
		if (i + 1 < length) 
			next_start = next_start + i + 1; // 다음 시작위치 저장한다
		else next_start = NULL; // 문자열 끝까지 확인했으면 NULL 저장한다
	} else {
		next_start = NULL;
	}

	//printf("commastrtok end\n"); ////////////////////

	return prev_start;
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
