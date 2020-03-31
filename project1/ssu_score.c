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

	set_scoreTable(ansDir);
	set_idTable(stuDir);

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

void set_scoreTable(char *ansDir) // csv 형식의 점수 테이블 파일 생성하는 함수
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
	DIR *dp, *c_dp; // 
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


void set_idTable(char *stuDir)
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
			strcpy(id_table[num++], dirp->d_name);
		else
			continue;
	}

	sort_idTable(num);
}

void sort_idTable(int size)
{
	int i, j;
	char tmp[10];

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

void sort_scoreTable(int size)
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){

			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);


			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];

	strncpy(dup, qname, strlen(qname));
	*num1 = atoi(strtok(dup, "-."));
	
	p = strtok(NULL, "-.");
	if(p == NULL)
		*num2 = 0;
	else
		*num2 = atoi(p);
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

void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);

	if((fd = creat("score.csv", 0666)) < 0){
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	write_first_row(fd);

	for(num = 0; num < size; num++)
	{
		if(!strcmp(id_table[num], ""))
			break;

		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp)); 

		score += score_student(fd, id_table[num]);
	}

	if(pOption)
		printf("Total average : %.2f\n", score / num);

	close(fd);
}

double score_student(int fd, char *id)
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);

		if(access(tmp, F_OK) < 0)
			result = false;
		else
		{
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;
			
			if(type == TEXTFILE)
				result = score_blank(id, score_table[i].qname);
			else if(type == CFILE)
				result = score_program(id, score_table[i].qname);
		}

		if(result == false)
			write(fd, "0,", 2);
		else{
			if(result == true){
				score += score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			else if(result < 0){
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}

	if(pOption)
		printf("%s is finished.. score : %.2f\n", id, score); 
	else
		printf("%s is finished..\n", id);

	sprintf(tmp, "%.2f\n", score);
	write(fd, tmp, strlen(tmp));

	return score;
}

void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	write(fd, ",", 1);

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)
			break;
		
		sprintf(tmp, "%s,", score_table[i].qname);
		write(fd, tmp, strlen(tmp));
	}
	write(fd, "sum\n", 4);
}

char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while(read(fd, &c, 1) > 0)
	{
		if(c == ':')
			break;
		
		result[idx++] = c;
	}
	if(result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0';

	return result;
}

int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	fd_std = open(tmp, O_RDONLY);
	strcpy(s_answer, get_answer(fd_std, s_answer));

	if(!strcmp(s_answer, "")){
		close(fd_std);
		return false;
	}

	if(!check_brackets(s_answer)){
		close(fd_std);
		return false;
	}

	strcpy(s_answer, ltrim(rtrim(s_answer)));

	if(s_answer[strlen(s_answer) - 1] == ';'){
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	if(!make_tokens(s_answer, tokens)){
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

double score_program(char *id, char *filename)
{
	double compile;
	int result;

	compile = compile_program(id, filename);

	if(compile == ERROR || compile == false)
		return false;
	
	result = execute_program(id, filename);

	if(!result)
		return false;

	if(compile < 0)
		return compile;

	return true;
}

int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname))
			return true;
	}
	return false;
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

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	
	isthread = is_thread(qname);

	sprintf(tmp_f, "%s/%s/%s", ansDir, qname, filename);
	sprintf(tmp_e, "%s/%s/%s.exe", ansDir, qname, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_e, "%s/%s/%s_error.txt", ansDir, qname, qname);
	fd = creat(tmp_e, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	unlink(tmp_e);

	if(size > 0)
		return false;

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){
		if(eOption)
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if(access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			rename(tmp_f, tmp_e);

			result = check_error_warning(tmp_e);
		}
		else{ 
			result = check_error_warning(tmp_f);
			unlink(tmp_f);
		}

		return result;
	}

	unlink(tmp_f);
	return true;
}

double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:"))
			return ERROR;
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING;
	}

	return warning;
}

int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(ans_fname, "%s/%s/%s.stdout", ansDir, qname, qname);
	fd = creat(ans_fname, 0666);

	sprintf(tmp, "%s/%s/%s.exe", ansDir, qname, qname);
	redirection(tmp, fd, STDOUT);
	close(fd);

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);

	start = time(NULL);
	redirection(tmp, fd, STDOUT);
	
	sprintf(tmp, "%s.stdexe", qname);
	while((pid = inBackground(tmp)) > 0){
		end = time(NULL);

		if(difftime(end, start) > OVER){
			kill(pid, SIGKILL);
			close(fd);
			return false;
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname);
}

pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;
	
	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	sprintf(command, "ps | grep %s", name);
	redirection(command, fd, STDOUT);

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp));

	if(!strcmp(tmp, "")){
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	return pid;
}

int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while(1)
	{
		while((len1 = read(fd1, &c1, 1)) > 0){
			if(c1 == ' ') 
				continue;
			else 
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){
			if(c2 == ' ') 
				continue;
			else 
				break;
		}
		
		if(len1 == 0 && len2 == 0)
			break;

		to_lower_case(&c1);
		to_lower_case(&c2);

		if(c1 != c2){
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true;
}

void redirection(char *command, int new, int old)
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
	
	if((dp = opendir(path)) == NULL)
		return;

	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if(lstat(tmp, &statbuf) == -1)
			continue;

		if(S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);
		else
			unlink(tmp);
	}

	closedir(dp);
	rmdir(path);
}

void to_lower_case(char *c)
{
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

void print_usage()
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	printf(" -h                print usage\n");
	printf(" -p                print student's score and total average\n");
	printf(" -c <IDS>          print ID's score\n");
}
