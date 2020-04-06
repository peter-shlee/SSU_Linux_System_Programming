#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ssu_score.h"
#include "blank.h"

extern struct ssu_scoreTable score_table[QNUM];
extern char id_table[SNUM][10];

struct ssu_scoreTable score_table[QNUM];
char id_table[SNUM][10];

char stuDir[BUFLEN]; // 학생들이 제출한 답안들이 들어있는 디렉토리
char ansDir[BUFLEN]; // 정답 파일들이 들어있는 디렉토리
char errorDir[BUFLEN];
char threadFiles[ARGNUM][FILELEN];
char cIDs[ARGNUM][FILELEN];

// option flags
int eOption = false;
int tOption = false;
int pOption = false;
int cOption = false;

void ssu_score(int argc, char *argv[])
{
	char saved_path[BUFLEN];
	int i; // for문에 사용할 인덱스 변수

	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-h")){ // -h 옵션이 적용되어 실행되면
			print_usage(); // 사용법 출력
			return; // ssu_score 종료
		}
	}

	memset(saved_path, 0, BUFLEN); // saved_path을 0으로 초기화
	if(argc >= 3 && strcmp(argv[1], "-c") != 0){ // ********** -c옵션이 무엇인지?
		strcpy(stuDir, argv[1]); // <STUDENTDIR> 저장
		strcpy(ansDir, argv[2]); // <TRUESETDIR> 저장
	}

	if(!check_option(argc, argv))
		exit(1);

	if(!eOption && !tOption && !pOption && cOption){ // -c 옵션만 적용됐으면
		do_cOption(cIDs);
		return; // ssu_score 종료
	}

	getcwd(saved_path, BUFLEN); // 프로세스의 현재 위치 절대경로 저장 

	if(chdir(stuDir) < 0){ // cd <STUDENTDIR>, cd 실패했다면
		fprintf(stderr, "%s doesn't exist\n", stuDir); //에레메세지 출력
		return; // ssu_score 종료
	}
	getcwd(stuDir, BUFLEN); // stuDir에 <STUDENTDIR>의 절대경로 저장

	chdir(saved_path); // 다시 프로세스가 실행된 디렉토리로 이동
	if(chdir(ansDir) < 0){ // cd <TRUESETDIR>, cd 실패했다면
		fprintf(stderr, "%s doesn't exist\n", ansDir); //에러메세지 출력
		return; // ssu_score 종료
	}
	getcwd(ansDir, BUFLEN); // ansDir에 <TRUESETDIR>의 절대경로 저장

	chdir(saved_path); // 다시 프로세스가 실행된 디렉토리로 이동

	set_scoreTable(ansDir); // 문제별 점수들이 저장될 score_table 구조체 배열을 setting
	set_idTable(stuDir); // 학생들의 학번이 저장될 id_table 배열을 setting

	printf("grading student's test papers..\n");
	score_students();

	if(cOption)
		do_cOption(cIDs);

	return;
}

// 프로그램 실행 시 전달된 옵션을 체크하는 함수
int check_option(int argc, char *argv[])
{
	int i, j; // 반복문에서 사용하는 인덱스
	int c; // 옵션으로 전달된 알파벳

	while((c = getopt(argc, argv, "e:thpc")) != -1)
	{
		switch(c){
			case 'e': // 옵션 e
				eOption = true;
				strcpy(errorDir, optarg); // 옵션에 전달된 인자(에러 메세지가 출력될 디렉토리)를 errorDir에 복사해 놓는다

				if(access(errorDir, F_OK) < 0) // 디렉터리에 접근이 불가능하면
					mkdir(errorDir, 0755);// 디렉터리 생성
				else{
					rmdirs(errorDir); // 기존 디렉터리 제거
					mkdir(errorDir, 0755); // 새 디렉터리 생성
				}
				break;
			case 't': // 옵션 t
				tOption = true;
				i = optind; // 프로그램 전달인자 인덱스
				j = 0; // 옵션 가변인자 인덱스

				while(i < argc && argv[i][0] != '-'){ // t 옵션에 전달된 가변인자들 확인을 위한 반복문

					if(j >= ARGNUM) // 가변인자를 받는 옵션이므로 가변인자의 개수가 최대 개수를 넘지 않았는지 확인한다
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else // 옵션에 전달된 인자를 threadFiles에 복사해 놓는다
						strcpy(threadFiles[j], argv[i]);
					i++; 
					j++;
				}
				break;
			case 'p': // 옵션 p - 항상 수행되어야 한다
				pOption = true;
				break;
			case 'c': // 옵션 c - 필요없는 옵션
				cOption = true;
				i = optind; // 프로그램 전달인자 인덱스
				j = 0; // 옵션에 전달된 가변인자 인덱스

				while(i < argc && argv[i][0] != '-'){ // c 옵션에 전달된 가변인자들 확인을 위한 반복문

					if(j >= ARGNUM) // 가변인자를 받는 옵션이므로 가변인자의 개수가 최대 개수를 넘지 않았는지 확인
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else
						strcpy(cIDs[j], argv[i]); // 옵션에 전달된 인자를 cIDs에 복사해 놓는다
					i++; 
					j++;
				}
				break;
			case '?': // unkown option
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}


void do_cOption(char (*ids)[FILELEN]) // 선택한 학번의 점수를 출력하는 옵션 -> 필요 없음
{
	FILE *fp;
	char tmp[BUFLEN];
	int i = 0;
	char *p, *saved;

	if((fp = fopen("score.csv", "r")) == NULL){
		fprintf(stderr, "file open error for score.csv\n");
		return;
	}

	fscanf(fp, "%s\n", tmp);

	while(fscanf(fp, "%s\n", tmp) != EOF)
	{
		p = strtok(tmp, ",");

		if(!is_exist(ids, tmp))
			continue;

		printf("%s's score : ", tmp);

		while((p = strtok(NULL, ",")) != NULL)
			saved = p;

		printf("%s\n", saved);
	}
	fclose(fp);
}

int is_exist(char (*src)[FILELEN], char *target) // - C 옵션에서 사용하는 함수 -> 필요 없음
{
	int i = 0;

	while(1)
	{
		if(i >= ARGNUM)
			return false;
		else if(!strcmp(src[i], ""))
			return false;
		else if(!strcmp(src[i++], target))
			return true;
	}
	return false;
}

void set_scoreTable(char *ansDir) // 각 문제별 점수를 저장해 놓는 score_table 구조체 배열을 setting하는 함수
{
	char filename[FILELEN];

	sprintf(filename, "%s/%s", ansDir, "score_table.csv"); // 점수 테이블 파일이 생성될 경로를 생성해 filename에 저장

	if(access(filename, F_OK) == 0) // 이미 점수 테이블 파일이 존재한다면
		read_scoreTable(filename); // 기존의 파일에서 문제 번호와 점수들을 불러온다
	else{ // 점수 테이블 파일이 존재하지 않는다면
		make_scoreTable(ansDir);
		write_scoreTable(filename);
	}
}

void read_scoreTable(char *path)
{
	FILE *fp;
	char qname[FILELEN]; // 문제 번호를 임시 저장할 배열
	char score[BUFLEN]; // 점수를 임시 저장할 배열
	int idx = 0; // score_table 구조체 배열의 인덱스

	if((fp = fopen(path, "r")) == NULL){ // 점수 테이블 오픈
		fprintf(stderr, "file open error for %s\n", path);
		return ;
	}

	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){ // 파일에서 문제 번호와 점수 읽어 들임
		strcpy(score_table[idx].qname, qname); // 문제 번호를 score_table 구조체 배열에 저장
		score_table[idx++].score = atof(score); // 해당 문제의 점수를 score_table 구조체 배열에 저장
	}

	fclose(fp);
}

void make_scoreTable(char *ansDir) // score_table 구조체 배열에 문제 번호와 점수를 저장해 점수 테이블을 만드는 함수
{
	int type, num; // type - 파일의 확장자 type을 저장해 놓을 변수
	double score, bscore, pscore;
	struct dirent *dirp, *c_dirp;
	DIR *dp, *c_dp; 
	char tmp[BUFLEN];
	int idx = 0; // 문제 총 개수 저장할 변수
	int i;

	num = get_create_type(); // 사용자에게 점수를 어떤식으로 입력받을 것인지 선택하도록 한다

	if(num == 1) // 점수 일괄 입력 선택 시
	{
		printf("Input value of blank question : ");
		scanf("%lf", &bscore); // 빈칸 문제의 점수 입력받음
		printf("Input value of program question : ");
		scanf("%lf", &pscore); // 프로그램 문제의 점수 입력받음
	}

	if((dp = opendir(ansDir)) == NULL){ // 디렉터리 open
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}	

	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) // 디렉터리 이름이 . 과 .. 이라면 pass
			continue;

		sprintf(tmp, "%s/%s", ansDir, dirp->d_name); // 다음에 확인할 디렉터리 경로 tmp에 저장

		if((c_dp = opendir(tmp)) == NULL){ // 확인 할 디렉터리 open
			fprintf(stderr, "open dir error for %s\n", tmp);
			return;
		}

		while((c_dirp = readdir(c_dp)) != NULL)
		{
			if(!strcmp(c_dirp->d_name, ".") || !strcmp(c_dirp->d_name, ".."))
				continue;

			if((type = get_file_type(c_dirp->d_name)) < 0) // 파일의 확장자가 .txt or .c 가 아니라면 pass
				continue;

			strcpy(score_table[idx++].qname, c_dirp->d_name);
		}

		closedir(c_dp);
	}

	closedir(dp);
	sort_scoreTable(idx);

	for(i = 0; i < idx; i++) // 모든 문제에 대하여
	{
		type = get_file_type(score_table[i].qname); // 파일의 확장자가 무엇인지 확인해 type에 저장

		if(num == 1) // 점수 일괄 입력 선택 시
		{
			if(type == TEXTFILE) // .txt 파일이라면 (빈칸문제)
				score = bscore;
			else if(type == CFILE) // .c 파일이라면 (프로그램 문제)
				score = pscore;
		}
		else if(num == 2) // 점수 각각 입력 선택 시
		{
			printf("Input of %s: ", score_table[i].qname); // 점수 입력받을 문제 번호 출력
			scanf("%lf", &score); // 해당 문제의 점수 입력받음
		}

		score_table[i].score = score; // score_table 구조체 배열에 문제 점수 저장
	}
}

void write_scoreTable(char *filename) // score_table 구조체 배열의 내용을 csv 형식의 파일에 출력하는 함수
{
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]); // 문제 총 개수

	if((fd = creat(filename, 0666)) < 0){ // 새로운 csv파일 생성
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++) // 모든 문제에 대하여
	{
		if(score_table[i].score == 0) // score_table에 저장된 점수가 0점이라면 끝 //////////////////////////////////////////////////////////// score_table 끝에 0 어디서 넣었는지?
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score); // 문제 번호와 점수를 csv형식의 문자열로 만들어 tmp에 저장
		write(fd, tmp, strlen(tmp)); // tmp의 내용을 파일에 write
	}

	close(fd);
}


void set_idTable(char *stuDir) // 학생들의 학번을 저장해 놓는 id_table 배열을 setting 하는 함수
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	if((dp = opendir(stuDir)) == NULL){
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) // 디렉터리 이름이 . 과 .. 이라면 pass
			continue;

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name); // 확인할 디렉터리의 경로를 tmp에 저장
		stat(tmp, &statbuf);

		if(S_ISDIR(statbuf.st_mode)) // 디렉터리 파일이라면
			strcpy(id_table[num++], dirp->d_name); // id_table에 디렉터리 이름(학번) 복사
		else
			continue;
	}

	sort_idTable(num); // id_table 정렬
}

void sort_idTable(int size) // 학생들의 학번이 저장되어 있는 id_table을 정렬하는 함수
{
	int i, j;
	char tmp[10]; // swap에 사용하는 임시 배열

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

void sort_scoreTable(int size) // 각 문제들의 점수가 저장되어있는 score_table 구조체 배열을 정렬하는 함수
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2; // 비교할 첫번째 문제의 상위 문제 번호와 하위 문제 번호
	int num2_1, num2_2; // 비교할 두번째 문제의 상위 문제 번호와 하위 문제 번호

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){

			get_qname_number(score_table[j].qname, &num1_1, &num1_2); // 비교할 첫번째 문제 번호를 int형으로 변환
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2); // 비교할 두번째 문제 번호를 int형으로 변환


			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){ // 첫번째 문제 번호가 두번째 문제 번호보다 큰 경우 SWAP

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

void get_qname_number(char *qname, int *num1, int *num2) // 문자열로 되어있는 문제번호를 인자로 받아 int형으로 변환하는 함수, num1은 상위 문제번호, num2는 하위 문제번호를 뜻함
{
	char *p;
	char dup[FILELEN];

	strncpy(dup, qname, strlen(qname)); // 인자로 받은 문제 번호를 dup에 복사한다
	*num1 = atoi(strtok(dup, "-.")); // '-', '.'을 기준으로 문자열을 분할, 분할한 문자열을 int형으로 변환
	
	p = strtok(NULL, "-."); // '-', '.'을 기준으로 위의 문자열을 이어서 분할
	if(p == NULL) // 여기서 p가 NULL이면 하위 문제가 없는 문제
		*num2 = 0;
	else // 하위 문제가 있는 문제일 경우
		*num2 = atoi(p); // 하위 문제 번호를 int형으로 변환해서 num2에 저장
}

int get_create_type() // 사용자가 문제 번호와 점수를 어떤 식으로 저장할지 선택하도록 하는 함수
{
	int num;

	while(1)
	{
		printf("score_table.csv file doesn't exist in TREUDIR!\n");
		printf("1. input blank question and program question's score. ex) 0.5 1\n"); // 빈칸 문제와 프로그램 문제의 점수를 각각 일괄적으로 입력하려면 1 선택
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n"); // 각각의 문제에 대해서 따로 따로 점수를 입력하려면 2 선택
		printf("select type >> ");
		scanf("%d", &num); // 사용자의 선택을 읽어들인다

		if(num != 1 && num != 2) // 사용자의 입력이 1도 아니고 2도 아닐 떄
			printf("not correct number!\n"); //잘못된 입력이므로 다시 입력받음
		else
			break;
	}

	return num; // 사용자가 선택한 숫자 리턴
}

void score_students() // 채점하는 함수
{
	double score = 0; // 모든 학생의 총점을 저장할 변수
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]); // 전체 학생 수

	if((fd = creat("score.csv", 0666)) < 0){ // 채점 결과를 저장할 score.csv 파일 생성
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	write_first_row(fd); // score.csv의 첫번째 열에 문제 번호 등 출력

	for(num = 0; num < size; num++) // 전체 학생 수 만큼 반복
	{
		if(!strcmp(id_table[num], ""))
			break;

		// 학번을 score.csv에 출력
		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp)); 

		score += score_student(fd, id_table[num]); // 해당 학생에 대하여 채점을 한 뒤 학생의 총점을 전체 총점에 더함
	}

	if(pOption) // p옵션이 설정되어 있다면 -> 항상 수행되도록 해야함
		printf("Total average : %.2f\n", score / num); // 전체 평균 점수 출력

	close(fd);
}

double score_student(int fd, char *id) // 한 학생에 대하여 채점을 하는 함수, 리턴값은 해당 학생의 총점
{
	int type; // 해당 문제가 빈칸 문제인지, 프로그램 문제인지 저장할 변수
	double result; // 해당 문제에 대한 정답 여부 또는 감점된 점수를 담을 변수
	double score = 0; // 해당 학생의 총점을 저장할 변수
	int i;
	char tmp[BUFLEN]; // 파일에 write하기 전에 임시로 담아 놓는 배열
	int size = sizeof(score_table) / sizeof(score_table[0]); // 전체 문항 수

	for(i = 0; i < size ; i++) // 전체 문항 수만큼 반복
	{
		if(score_table[i].score == 0) // 해당 문제의 배점이 0점이라면 채점 중단
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname); // 해당 학생의 해당 문제 디렉터리로 이동

		if(access(tmp, F_OK) < 0) // 해당 문제 디렉터리에 접근이 불가하다면
			result = false;
		else
		{
			if((type = get_file_type(score_table[i].qname)) < 0) // 파일의 확장자 명으로 빈칸 문제인지, 프로그램 문제인지 확인
				continue;
			
			if(type == TEXTFILE) // .txt 파일(빈칸 문제)이라면
				result = score_blank(id, score_table[i].qname); // 빈칸문제 채점
			else if(type == CFILE) // .c 파일(프로그램 문제)이라면
				result = score_program(id, score_table[i].qname); // 프로그램 문제 채점
		}

		if(result == false) // 해당 학생이 문제를 틀렸을 때
			write(fd, "0,", 2); // 0점 부여, score.csv에 해당 문제 0점이라고 출력
		else{
			if(result == true){ // 해당 학생이 문제를 맞혔을 때
				score += score_table[i].score; // 해당 문제의 배점을 학생의 점수에 더함
				sprintf(tmp, "%.2f,", score_table[i].score); // tmp에 해당 문제에 대하여 학생이 받은 점수를 기록
			}
			else if(result < 0){ // result 값이 0보다 작다면 감점
				score = score + score_table[i].score + result; // 감점된 점수를 반영하여 학생의 점수에 더함
				sprintf(tmp, "%.2f,", score_table[i].score + result); // tmp에 해당 문제에 대하여 학생이 받은 점수를 기록
			}
			write(fd, tmp, strlen(tmp)); // score.csv에 tmp에 기록해 놨던 점수 출력
		}
	}

	if(pOption) // p옵션이 설정되어 있다면 -> 항상 수행되도록 해야함
		printf("%s is finished.. score : %.2f\n", id, score); // 해당 학생의 총점 출력
	else
		printf("%s is finished..\n", id);

	sprintf(tmp, "%.2f\n", score); // 학생의 총점 tmp에 기록
	write(fd, tmp, strlen(tmp)); // tmp에 있는 학생의 총점 score.csv에 출력

	return score; // 해당 학생의 총점 return
}

void write_first_row(int fd) // score.csv의 첫번째 열을 write하는 함수
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]); // 전체 문항 수

	write(fd, ",", 1);

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)
			break;
		
		// score.csv에 문제 번호 출력
		sprintf(tmp, "%s,", score_table[i].qname);
		write(fd, tmp, strlen(tmp));
	}
	write(fd, "sum\n", 4);
}

char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN); // 전달인자로 받은 result를 0으로 초기화
	while(read(fd, &c, 1) > 0) // fd에서 한문자씩 읽어온다
	{
		if(c == ':') // 읽은 문자가 ':' 이라면
			break; // 반복 종료
		
		result[idx++] = c; // result배열에 읽은 문자 넣는다
	}
	if(result[strlen(result) - 1] == '\n') // result에 들어간 문자열의 마지막 문자가 개행문자라면
		result[strlen(result) - 1] = '\0'; // 널문자로 바꿈

	return result;
}

int score_blank(char *id, char *filename) // 빈칸 문제를 채점하는 함수, 리턴값은 정답 여부 또는 감점된 점수
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN]; // 문제 번호를 저장할 배열
	int fd_std, fd_ans; // fd_std는 학생의 답안파일의 파일디스크립터, fd_ans는 정답 파일의 파일 디스크립터
	int result = true; // 정답인지 오답인지
	int has_semicolon = false; // 학생의 답 맨 끝에 세미콜론이 있었는지 기록해 놓을  변수

	memset(qname, 0, sizeof(qname)); // qname 배열 0 초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.'))); //  qname에 확장자 명을 뺀 파일 이름(문제 번호)을 넣음

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename); // 현재 문제 경로 tmp에 저장
	fd_std = open(tmp, O_RDONLY); // fd_std에 학생의 답안 파일 파일디스크립터 저장
	strcpy(s_answer, get_answer(fd_std, s_answer)); // 학생이 제출한 답안 파일에서 답을 읽어와 s_answer에 저장

	if(!strcmp(s_answer, "")){ // 학생의 답이 비어있다면
		close(fd_std);
		return false; // 오답
	}

	if(!check_brackets(s_answer)){ // 여는 괄호, 닫는 괄호의 짝이 맞지 않으면
		close(fd_std);
		return false; // 오답
	}

	strcpy(s_answer, ltrim(rtrim(s_answer))); // 학생의 답 앞뒤에 있는 white space를 제거하여 다시 s_answer에 담는다

	if(s_answer[strlen(s_answer) - 1] == ';'){ // 학생의 답 제일 뒤에 ;이 있다면
		has_semicolon = true; // 세미콜론이 있었다고 기록하고
		s_answer[strlen(s_answer) - 1] = '\0'; // 널문자를 넣는다
	}

	if(!make_tokens(s_answer, tokens)){ // 400줄짜리 함수
		close(fd_std);
		return false;
	}

	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0);

	sprintf(tmp, "%s/%s/%s", ansDir, qname, filename);
	fd_ans = open(tmp, O_RDONLY);

	while(1)
	{
		ans_root = NULL;
		result = true;

		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		strcpy(a_answer, get_answer(fd_ans, a_answer));

		if(!strcmp(a_answer, ""))
			break;

		strcpy(a_answer, ltrim(rtrim(a_answer)));

		if(has_semicolon == false){
			if(a_answer[strlen(a_answer) -1] == ';')
				continue;
		}

		else if(has_semicolon == true)
		{
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';
		}

		if(!make_tokens(a_answer, tokens))
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);

		compare_tree(std_root, ans_root, &result);

		if(result == true){
			close(fd_std);
			close(fd_ans);

			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;

		}
	}
	
	close(fd_std);
	close(fd_ans);

	if(std_root != NULL)
		free_node(std_root);
	if(ans_root != NULL)
		free_node(ans_root);

	return false;
}

double score_program(char *id, char *filename) // 프로그램 문제를 채점하는 함수, 리턴값은 정답 여부 또는 감점된 점수
{
	double compile; // 컴파일 결과를 저장할 변수
	int result;

	compile = compile_program(id, filename); // 컴파일 수행

	if(compile == ERROR || compile == false) // 컴파일 실패했다면
		return false; // false 리턴
	
	result = execute_program(id, filename); // 정답 파일과 학생의 답안 파일을 실행하고, 결과 파일을 비교하여 정답인지 오답인지 확인한다.

	if(!result) // 오답이라면
		return false; // false 리턴

	if(compile < 0) // 감점됐다면
		return compile; // 감점된 점수 리턴

	return true; // 정답이면 true 리턴
}

int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]); // -lpthread 옵션으로 실행할 프로그램들의 목록이 저장된 threadFiles배열의 크기 계산

	for(i = 0; i < size; i++){ 
		if(!strcmp(threadFiles[i], qname)) // 인자로 전달된 qname이 threadFiles에 있다면
			return true; // true 리턴
	}
	return false; // qname이 threadFiles에 없으면 false 리턴
}

double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	memset(qname, 0, sizeof(qname)); // qname 0초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.'))); // qname에 확장자명 제외한 파일명 복사
	
	isthread = is_thread(qname); // -lpthread 옵션으로 컴파일을 할것인지 확인

	sprintf(tmp_f, "%s/%s/%s", ansDir, qname, filename); // 컴파일 할 파일 명
	sprintf(tmp_e, "%s/%s/%s.exe", ansDir, qname, qname); // 컴파일 결과로 나올 실행파일의 파일 명

	if(tOption && isthread) // -lpthread 옵션으로 컴파일할 파일이라면
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f); // -lpthread 옵션으로 컴파일 할 때 사용할 문자열
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f); // 그냥 컴파일 할 때 사용할 문자열

	sprintf(tmp_e, "%s/%s/%s_error.txt", ansDir, qname, qname); // 컴파일 에러를 출력할 파일명 생성
	fd = creat(tmp_e, 0666); // 에러 출력할 파일 생성

	redirection(command, fd, STDERR); // command를 실행하고 실행하는 동안 STDERR에 출력될 내용을 fd(에러 출력할 파일)에 출력함
	size = lseek(fd, 0, SEEK_END); // 에러 출력된 파일의 크기 저장
	close(fd); // 에러 출력된 파일 close
	unlink(tmp_e); // 에러 출력된 파일 삭제 - 학생 답안 파일이 아닌 정답 파일 컴파일 시 에러 내용이므로 저장할 필요 없음

	if(size > 0) // 컴파일 에러 발생했다면
		return false; // false 리턴

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename); // 컴파일 할 학생 답안 파일
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname); // 컴파일 결과로 나올 실행파일의 파일 명

	if(tOption && isthread) // -lpthread 옵션으로 컴파일할 파일이라면
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f); // -lpthread 옵션으로 컴파일 할 때 사용할 command
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f); // 그냥 컴파일 할 때 사용할 command

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname); // 컴파일 에러 내용을 저장할 파일 명
	fd = creat(tmp_f, 0666); // 에러 저장할 파일 생성

	redirection(command, fd, STDERR); // command를 실행하고 실행하는 동안 STDERR에 출력될 내용을 fd(에러 출력할 파일)에 출력함
	size = lseek(fd, 0, SEEK_END); // 에러 출력된 파일의 크기 저장
	close(fd); // 에러 출력된 파일 close

	if(size > 0){ // 컴파일 에러 발생했다면
		if(eOption) // e 옵션이 지정되어 있다면
		{
			sprintf(tmp_e, "%s/%s", errorDir, id); // 에러 파일 저장할 경로
			if(access(tmp_e, F_OK) < 0) // 해당 경로에 접근 불가하면
				mkdir(tmp_e, 0755); // 디렉터리 생성

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname); // 에러 파일명
			rename(tmp_f, tmp_e); // 위에서 만들어둔 에러파일의 이름을 제대로 된 에러 파일명으로 바꾼다

			result = check_error_warning(tmp_e); // 에러 내용을 확인해 결과 점수를 계산한다
		}
		else{ 
			result = check_error_warning(tmp_f); // 에러 내용을 확인해 결과 점수를 계산한다
			unlink(tmp_f); // 에러내용 저장했던 파일 삭제
		}

		return result; // 결과 점수 리턴
	}

	unlink(tmp_f); // 에러 내용 파일 삭제
	return true; // 컴파일 성공 했으므로 true 리턴
}

double check_error_warning(char *filename) // 컴파일 에러 내용이 저장된 파일을 이용해 error인지 warning인지 확인해서 결과 점수를 리턴하는 함수
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){ // 파일을 읽기 모드로 open
		fprintf(stderr, "fopen error for %s\n", filename);
		return false; // 파일을 열지 못했으면 false 리턴
	}

	while(fscanf(fp, "%s", tmp) > 0){ // 파일 내용 확인
		if(!strcmp(tmp, "error:")) // error: 라는 문자열이 들어있으면
			return ERROR; // ERROR 리턴
		else if(!strcmp(tmp, "warning:")) // warning: 이라는 문자열이 들어있으면
			warning += WARNING; // WARNING 점수를 누적해서 저장
	}

	return warning; // 점수 리턴
}

int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname)); // qname 0초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.'))); // qname에 문제 번호 저장

	sprintf(ans_fname, "%s/%s/%s.stdout", ansDir, qname, qname); // 정답 실행파일의 실행결과를 저장할 파일
	fd = creat(ans_fname, 0666); // 실행결과 저장파일 생성

	sprintf(tmp, "%s/%s/%s.exe", ansDir, qname, qname); // 정답 실행 파일
	redirection(tmp, fd, STDOUT); //  정답 실행 파일을 실행시키고 그 결과를 위에서 만든 실행 결과 저장 파일에 저장
	close(fd); // 실행 결과 저장 파일 close

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname); // 학생의 답안을 실행한 결과를 저장할 파일
	fd = creat(std_fname, 0666); // 학생 답안 실행결과 저장파일 생성

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname); // 학생 답안 실행 파일

	start = time(NULL); // 학생 답안 실행 시작시간 저장
	redirection(tmp, fd, STDOUT);// 학생 답안을 실행하고 결과를 저장
	
	sprintf(tmp, "%s.stdexe", qname); // 학생 답안 실행파일 파일명
	while((pid = inBackground(tmp)) > 0){ // 학생 답안 실행파일이 실행되는 동안 반복하며 체크
		end = time(NULL); // 얼마동안 실행중인지 저장

		if(difftime(end, start) > OVER){ // 실행 제한 시간을 초과했다면
			kill(pid, SIGKILL); // 종료시킴
			close(fd);
			return false; // false 리턴
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname); // 정답과 학생 답안의 결과를 비교해 리턴
}

pid_t inBackground(char *name) // 인자로 전달된 프로세스가 실행중인지 확인하는 함수
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;
	
	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	sprintf(command, "ps | grep %s", name);
	redirection(command, fd, STDOUT); // ps | grep <name> 실행 뒤 그 결과를 background.txt에 저장

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp)); // 위의 실행 결과를 읽어옴

	if(!strcmp(tmp, "")){ // 아무것도 출력되지 않았다면
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " ")); // pid 저장
	close(fd);

	unlink("background.txt");
	return pid;
}

int compare_resultfile(char *file1, char *file2) // 프로그램을 실행한 결과로 나온 파일을 비교하는 함수
{
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	fd1 = open(file1, O_RDONLY); // 첫번째 파일 open
	fd2 = open(file2, O_RDONLY); // 두번째 파일 open

	while(1)
	{
		while((len1 = read(fd1, &c1, 1)) > 0){ // 첫번째 파일 공백문자 제거
			if(c1 == ' ') 
				continue;
			else 
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){ // 두번째 파일 공백문자 제거
			if(c2 == ' ') 
				continue;
			else 
				break;
		}
		
		if(len1 == 0 && len2 == 0) // 파일의 끝이라면
			break; // 반복 종료

		to_lower_case(&c1); // c1에 저장된 문자가 대문자 알파벳인 경우 소문자로 변환
		to_lower_case(&c2); // c2에 저장된 문자가 대문자 알파벳인 경우 소문자로 변환

		if(c1 != c2){ // c1과 c2가 다르다면 프로그램 수행 결과가 다르다는 뜻이므로
			close(fd1);
			close(fd2);
			return false; // 오답
		}
	}
	close(fd1);
	close(fd2);
	return true; // 정답
}

void redirection(char *command, int new, int old) // new 파일디스크립터를 old에 복사한 뒤 command를 실행한다. 실행한 후에는 다시 원래 old에 있던 값으로 복구
{
	int saved;

	saved = dup(old);
	dup2(new, old);

	system(command);

	dup2(saved, old);
	close(saved);
}

int get_file_type(char *filename) // 파일의 확장자를 확인하는 함수
{
	char *extension = strrchr(filename, '.'); // filename에서 '.'이 있는 위치의 포인터를 extension에 저장

	if(!strcmp(extension, ".txt")) // 파일 확장자가 .txt 라면
		return TEXTFILE;
	else if (!strcmp(extension, ".c")) // 파일 확장자가 .c 라면
		return CFILE;
	else // .c도, .txt도 아니라면
		return -1;
}

void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[50];
	
	if((dp = opendir(path)) == NULL) // opendir 실패시
		return;

	while((dirp = readdir(dp)) != NULL) // 디렉토리 내용 확인
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) // . .. 디렉토리는 pass
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if(lstat(tmp, &statbuf) == -1) // stat구조체 가져옴
			continue;

		if(S_ISDIR(statbuf.st_mode)) // 디렉토리 파일일 경우
			rmdirs(tmp); // 재귀호출
		else // 아닌경우
			unlink(tmp); // unlink
	}

	closedir(dp);
	rmdir(path);
}

void to_lower_case(char *c) // 대문자 알파벳을 소문자로 변환하는 함수
{
	if(*c >= 'A' && *c <= 'Z') // 대문자 알파벳 일때만
		*c = *c + 32; // 소문자로 변환
}

void print_usage() // 프로그램 사용법 출력
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	printf(" -h                print usage\n");
	printf(" -p                print student's score and total average\n");
	printf(" -c <IDS>          print ID's score\n");
}
