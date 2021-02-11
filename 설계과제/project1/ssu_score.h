#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef STDOUT
	#define STDOUT 1
#endif
#ifndef STDERR
	#define STDERR 2
#endif
#ifndef TEXTFILE
	#define TEXTFILE 3
#endif
#ifndef CFILE
	#define CFILE 4
#endif
#ifndef OVER
	#define OVER 5
#endif
#ifndef WARNING
	#define WARNING -0.1
#endif
#ifndef ERROR
	#define ERROR 0
#endif

#define FILELEN 64
#define BUFLEN 1024
#define SNUM 100
#define QNUM 100
#define ARGNUM 5 // 가변인자를 받는 옵션의 경우, 최대로 받을 수 있는 가변인자의 개수는 5개로 제한

struct ssu_scoreTable{ // 문제별 정보를 담아놓는 구조체
	char qname[FILELEN]; // 문제 번호
	double score; // 배점
};

void ssu_score(int argc, char *argv[]);
int check_option(int argc, char *argv[]); // 프로그램 실행 시 전달된 옵션을 체크하는 함수
void print_usage(); // 프로그램 사용법 출력 (-h 옵션)

void score_students();// 채점하는 함수
double score_student(int fd, char *id);// 한 학생에 대하여 채점을 하는 함수, 리턴값은 해당 학생의 총점
void write_first_row(int fd);// score.csv의 첫번째 열을 write하는 함수

char *get_answer(int fd, char *result);// 답안 파일에서 내용을 읽어와 result에 저장하는 함수
int score_blank(char *id, char *filename);// 빈칸 문제를 채점하는 함수, 리턴값은 정답 여부 또는 감점된 점수
double score_program(char *id, char *filename);// 프로그램 문제를 채점하는 함수, 리턴값은 정답 여부 또는 감점된 점수
double compile_program(char *id, char *filename);// 프로그램 문제를 컴파일하는 함수
int execute_program(char *id, char *filname);// 프로그램 문제를 실행하는 함수
pid_t inBackground(char *name);// 인자로 전달된 프로세스가 실행중인지 확인하는 함수
double check_error_warning(char *filename);// 컴파일 에러 내용이 저장된 파일을 이용해 error인지 warning인지 확인해서 결과 점수를 리턴하는 함수
int compare_resultfile(char *file1, char *file2);// 프로그램을 실행한 결과로 나온 파일을 비교하는 함수

void do_cOption(char (*ids)[FILELEN]);// 선택한 학번의 점수를 출력하는 옵션 -> 필요 없음
int is_exist(char (*src)[FILELEN], char *target);// src 문장열 배열 안에 target 문자열이 들어있는지 확인하는 함수

int is_thread(char *qname);// -t(lpthread 사용) 옵션으로 지정된 문제인지 확인하는 함수
void redirection(char *command, int newfd, int oldfd);// new 파일디스크립터를 old에 복사한 뒤 command를 실행한다. 실행한 후에는 다시 원래 old에 있던 값으로 복구
int get_file_type(char *filename);// 파일의 확장자를 확인하는 함수
void rmdirs(const char *path);// 디렉터리 삭제하는 함수
void to_lower_case(char *c);// 대문자 알파벳을 소문자로 변환하는 함수

void set_scoreTable(char *ansDir);// 각 문제별 점수를 저장해 놓는 score_table 구조체 배열을 setting하는 함수
void read_scoreTable(char *path);// 파일에서 문제의 정보를 읽어와 score_table 구조체 배열에 저장하는 함수
void make_scoreTable(char *ansDir);// score_table 구조체 배열에 문제 번호와 점수를 저장해 점수 테이블을 만드는 함수
void write_scoreTable(char *filename);// score_table 구조체 배열의 내용을 csv 형식의 파일에 출력하는 함수
void set_idTable(char *stuDir);// 학생들의 학번을 저장해 놓는 id_table 배열을 setting 하는 함수
int get_create_type();// 사용자가 문제 번호와 점수를 어떤 식으로 저장할지 선택하도록 하는 함수

void sort_idTable(int size);// 학생들의 학번이 저장되어 있는 id_table을 정렬하는 함수
void sort_scoreTable(int size);// 각 문제들의 점수가 저장되어있는 score_table 구조체 배열을 정렬하는 함수
void get_qname_number(char *qname, int *num1, int *num2);// 문자열로 되어있는 문제번호를 인자로 받아 int형으로 변환하는 함수, num1은 상위 문제번호, num2는 하위 문제번호를 뜻함

void do_mOption();// m옵션 수행하는 함수
void do_iOption(char (*ids)[FILELEN]);// i옵션 수행하는 함수

#endif
