#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

char datatype[DATATYPE_SIZE][MINLEN] = {"int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct"};


operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}	
	,{"*", 4}	,{"/", 3}	,{"%", 2}	
	,{"+", 6}	,{"-", 5}	
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

void compare_tree(node *root1,  node *root2, int *result)
{
	node *tmp;
	int cnt1, cnt2;

	if(root1 == NULL || root2 == NULL){
		*result = false;
		return;
	}

	if(!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")){
		if(strcmp(root1->name, root2->name) != 0){

			if(!strncmp(root2->name, "<", 1))
				strncpy(root2->name, ">", 1);

			else if(!strncmp(root2->name, ">", 1))
				strncpy(root2->name, "<", 1);

			else if(!strncmp(root2->name, "<=", 2))
				strncpy(root2->name, ">=", 2);

			else if(!strncmp(root2->name, ">=", 2))
				strncpy(root2->name, "<=", 2);

			root2 = change_sibling(root2);
		}
	}

	if(strcmp(root1->name, root2->name) != 0){
		*result = false;
		return;
	}

	if((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)){
		*result = false;
		return;
	}

	else if(root1->child_head != NULL){
		if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
			*result = false;
			return;
		}

		if(!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))
		{
			compare_tree(root1->child_head, root2->child_head, result);

			if(*result == false)
			{
				*result = true;
				root2 = change_sibling(root2);
				compare_tree(root1->child_head, root2->child_head, result);
			}
		}
		else if(!strcmp(root1->name, "+") || !strcmp(root1->name, "*")
				|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
				|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
				*result = false;
				return;
			}

			tmp = root2->child_head;

			while(tmp->prev != NULL)
				tmp = tmp->prev;

			while(tmp != NULL)
			{
				compare_tree(root1->child_head, tmp, result);
			
				if(*result == true)
					break;
				else{
					if(tmp->next != NULL)
						*result = true;
					tmp = tmp->next;
				}
			}
		}
		else{
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}	


	if(root1->next != NULL){

		if(get_sibling_cnt(root1) != get_sibling_cnt(root2)){
			*result = false;
			return;
		}

		if(*result == true)
		{
			tmp = get_operator(root1);
	
			if(!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")
					|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
					|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{	
				tmp = root2;
	
				while(tmp->prev != NULL)
					tmp = tmp->prev;

				while(tmp != NULL)
				{
					compare_tree(root1->next, tmp, result);

					if(*result == true)
						break;
					else{
						if(tmp->next != NULL)
							*result = true;
						tmp = tmp->next;
					}
				}
			}

			else
				compare_tree(root1->next, root2->next, result);
		}
	}
}

int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;
	char tmp[BUFLEN];
	char str2[BUFLEN];
	char *op = "(),;><=!|&^/+-*\""; // 연산자들 모음
	int row = 0; // 몇번째 토큰인지?
	int i;
 	int isPointer;
	int lcount, rcount;
	int p_str;
	
	clear_tokens(tokens); // token 배열 0으로 초기화

	start = str; // start에 str의 시작 위치 저장
	
	if(is_typeStatement(str) == 0) // 맨 앞에 type이 써있는 구문이라면
		return false;	
	
	while(1)
	{
		if((end = strpbrk(start, op)) == NULL) // 문자열(start)에 연산자가 하나도 포함되어 있지 않으면
			break;

		if(start == end){ // 다음 토큰이 연산자라면

			if(!strncmp(start, "--", 2) || !strncmp(start, "++", 2)){ // -- 또는 ++라면
				if(!strncmp(start, "++++", 4)||!strncmp(start,"----",4)) // ++++ 또는 ----이라면
					return false;

				if(is_character(*ltrim(start + 2))){ // -- 또는 ++ 뒤의 공백문자 제거 후 뒤에 오는 문자가 알파벳이나 숫자 문자라면 (전위 연산자)
					if(row > 0 && /*!*/is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])) // 이전 토큰의 마지막 문자가 알파벳이나 숫자 문자가 아니라면 ////////////////////////////
						return false; 

					end = strpbrk(start + 2, op); // 다음 연산자의 위치를 end에 저장
					if(end == NULL) // 뒤에 더이상 연산자가 없다면
						end = &str[strlen(str)]; // end에 문자열의 끝 위치를 저장
					while(start < end) { // 문자열의 끝에 다다를 때까지
						if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
							return false;
						else if(*start != ' ') // *start가 공백문자가 아니라면
							strncat(tokens[row], start, 1); // tokens[row]에 전위 연산자를 포함한 토큰 넣음
						start++;	
					}
				}
				
				else if(row>0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){ // -- 또는 ++ 앞에 있던 토큰의 마지막 문자가 숫자 또는 알파벳 이라면 (후위 연산자)
					if(strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL) // 바로 앞의 토큰이 ++ 또는 --를 포함하고 있다면
						return false;

					memset(tmp, 0, sizeof(tmp)); // tmp배열 0초기화
					strncpy(tmp, start, 2); //tmp에 -- 또는 ++ 연산자 복사
					strcat(tokens[row - 1], tmp); // 앞의 토큰 뒤에 -- 또는 ++ 연산자 갖다 붙임
					start += 2; // 다음 토큰으로 이동
					row--;
				}
				else{
					memset(tmp, 0, sizeof(tmp)); //tmp 배열 0초기화
					strncpy(tmp, start, 2); // tmp 배열에 ++ 또는 -- 복사
					strcat(tokens[row], tmp); // tokens[row]의 뒤에 ++ 또는 -- 덧붙임
					start += 2; // 다음 토큰으로 이동
				}
			}

			else if(!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2) 
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2) 
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2)	|| !strncmp(start, "-=", 2) 
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)){ // 이 연산자들로 시작하는 문자열이라면

				strncpy(tokens[row], start, 2); // 이 연산자들을 tokens[row]에 복사
				start += 2; // 다음 토큰으로 이동
			}
			else if(!strncmp(start, "->", 2)) // -> 연산자라면
			{
				end = strpbrk(start + 2, op); // 이 뒤에 다른 연산자가 있는지 검사

				if(end == NULL) // 뒤에 다른 연산자가 없다면
					end = &str[strlen(str)];

				while(start < end){ // 다음 연산자 까지 or 뒤에 더이상 연산자가 없다면 문자열의 끝까지
					if(*start != ' ') // *start가 공백문자가 아니라면
						strncat(tokens[row - 1], start, 1); // 앞의 토큰(tokens[row - 1]) 에 덧붙임
					start++; // 다음 문자로 이동
				}
				row--; // 앞의 토큰에 덧붙였으므로 row 하나 감소
			}
			else if(*end == '&') // &연산자라면
			{
				
				if(row == 0 || (strpbrk(tokens[row - 1], op) != NULL)){ // 이 & 연산자가 첫번째 토큰이거나 이전 토큰에 연산자가 포함되어 있다면 (주소 연산자 &)
					end = strpbrk(start + 1, op); // end에 다음 연산자의 위치 저장
					if(end == NULL) // 뒤에 더이상 연산자가 없다면
						end = &str[strlen(str)]; // end에 문자열의 끝 위치 저장
					
					strncat(tokens[row], start, 1); // token[row]에 & 연산자 저장
					start++;

					while(start < end){
						if(*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&') // & 연산자와 뒤의 피연산자 사이에 공백이 있다면 에러
							return false;
						else if(*start != ' ')
							strncat(tokens[row], start, 1); // tokens[row]에 한문자씩 덧붙임
						start++;
					}
				}
				
				else{ // 비트연산자 &
					strncpy(tokens[row], start, 1); // tokens[row] 에 & 저장
					start += 1; // 다음 토큰으로 이동
				}
				
			}
		  	else if(*end == '*') // *연산자라면
			{
				isPointer=0; // 포인터 연산자인지 곱셈 연산자인지를 저장해 놓을 flag 변수

				if(row > 0) // 첫번째 토큰이 아니라면
				{
					
					for(i = 0; i < DATATYPE_SIZE; i++) {
						if(strstr(tokens[row - 1], datatype[i]) != NULL){ // 이전 토큰이 데이터 타입 이라면 (이 * 연산자가 포인터 연산자 *인 경우)
							strcat(tokens[row - 1], "*"); // 앞의 토큰(데이터 타입)의 뒤에 * 덧붙임
							start += 1; // 다음 토큰으로 이동
							isPointer = 1; // 이 연산자가 포인터 연산자임을 저장해놓는다
							break;
						}
					}
					if(isPointer == 1) // 이 연산자가 포인터 연산자라면 다음 토큰으로 이동
						continue;
					if(*(start+1) !=0) // 문자열의 끝이 아니라면
						end = start + 1; // 다음 문자로 이동

					
					if(row>1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)){ // 이 토큰 앞앞의 토큰이 *이고, 앞의 토큰이 전부 '*'로 이루어져 있다면(다중 포인터)
						strncat(tokens[row - 1], start, end - start); // 앞의 토큰 뒤에 덧붙임
						row--;
					}
					
					
					else if(is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1){  // 앞의 토큰의 마지막 문자가 알파벳이나 숫자라면
						strncat(tokens[row], start, end - start);  // tokens[row]의 뒤에 덧붙임
					}

					
					else if(strpbrk(tokens[row - 1], op) != NULL){ // 이전 토큰에 연산자가 포함되어 있다면
						strncat(tokens[row] , start, end - start);  // tokens[row]의 뒤에 덧붙임
							
					}
					else
						strncat(tokens[row], start, end - start);  // tokens[row]의 뒤에 덧붙임

					start += (end - start); // 다음 토큰으로 이동
				}

			 	else if(row == 0) // 첫번째 토큰이라면 
				{
					if((end = strpbrk(start + 1, op)) == NULL){ // start+1 문자열에 연산자가 포함되어 있지 않다면
						strncat(tokens[row], start, 1);
						start += 1;
					}
					else{
						while(start < end){
							if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
								return false;
							else if(*start != ' ')
								strncat(tokens[row], start, 1);
							start++;	
						}
						if(all_star(tokens[row]))
							row--;
						
					}
				}
			}
			else if(*end == '(')  // ( 연산자라면
			{
				lcount = 0;
				rcount = 0;
				if(row>0 && (strcmp(tokens[row - 1],"&") == 0 || strcmp(tokens[row - 1], "*") == 0)){ // 앞의 토큰이 &이나 *이 아니라면
					while(*(end + lcount + 1) == '(') // 다음 '('의 위치 찾는다
						lcount++;
					start += lcount;

					end = strpbrk(start + 1, ")"); // 다음 ')'의 위치 찾는다

					if(end == NULL) // 다음 ')'을 찾지 못했다면
						return false;
					else{
						while(*(end + rcount +1) == ')') // 다음 ')'의 위치 찾는다
							rcount++;
						end += rcount;

						if(lcount != rcount)
							return false;

						if( (row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1){ // 앞앞의 토큰의 마지막 문자가 알파벳, 숫자가 아니거나, 이번이 두번째 토큰인 경우
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1); // 앞의 토큰 뒤에 //////////////////////////////////
							row--;
							start = end + 1;
						}
						else{
							strncat(tokens[row], start, 1);
							start += 1;
						}
					}
						
				}
				else{
					strncat(tokens[row], start, 1);
					start += 1;
				}

			}
			else if(*end == '\"')  // \" 문자라면 (문자열)
			{
				end = strpbrk(start + 1, "\""); // 뒤쪽에 또다른 \"이 있는지 확인
				
				if(end == NULL) // 없다면 짝이 맞지 않으므로
					return false;

				else{
					strncat(tokens[row], start, end - start + 1); // tokens[row]에 \"으로 둘러싸인 토큰 넣는다
					start = end + 1; // 다음 토큰으로 이동
				}

			}

			else{ // 그 외의 연산자라면
				
				if(row > 0 && !strcmp(tokens[row - 1], "++")) // 마지막 토큰이 ++라면 짝이 맞지 않으므로
					return false;

				
				if(row > 0 && !strcmp(tokens[row - 1], "--")) // 마지막 토큰이 ++라면 짝이 맞지 않으므로
					return false;
	
				strncat(tokens[row], start, 1); // tokens[row]에 연산자 넣는다
				start += 1; // 다음 토큰으로 이동
				
			
				if(!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")){ // -, +, --, ++ 연산자라면


				
					if(row == 0) // 첫번째 토큰이었다면
						row--;

					
					else if(!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){ // 앞 토큰의 마지막 문자가 알파벳이나 숫자가 아니었다면
					
						if(strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL) // 앞의 토큰이 ++ 또는 --를 포함하고 있었다면
							row--;
					}
				}
			}
		}
		else{ // 그 외 연산자들
			if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) // 이전 토큰이 전부 '*'로 이루어져 있고 앞앞 토큰의 마지막 문자가 숫자나 알파벳이 아니라면
				row--;				

			if(all_star(tokens[row - 1]) && row == 1)  // 앞의 토큰이 전부 '*'로 이루어져 있고, 이번 토큰이 두번째 토큰이라면 (다중 포인터)
				row--;	

			for(i = 0; i < end - start; i++){
				if(i > 0 && *(start + i) == '.'){
					strncat(tokens[row], start + i, 1);

					while( *(start + i +1) == ' ' && i< end - start )
						i++; 
				}
				else if(start[i] == ' '){
					while(start[i] == ' ')
						i++;
					break;
				}
				else
					strncat(tokens[row], start + i, 1);
			}

			if(start[0] == ' '){
				start += i;
				continue;
			}
			start += i;
		}
			
		strcpy(tokens[row], ltrim(rtrim(tokens[row]))); // 토큰의 앞, 뒤로 있는 공백들 제거

		 if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.' ) ){ // 새로 구한 토큰이 첫번째 토큰이 아니고, 마지막 문자가 숫자나 알파벳 이고, 변수 선언문이거나, 앞 토큰의 마지막 문자가 숫자나 알파벳이거나, 앞 토큰의 마지막 문자가 '.'이라면 

			if(row > 1 && strcmp(tokens[row - 2],"(") == 0) // 앞앞 토큰이 '(' 이라면
			{
				if(strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1],"unsigned") != 0) // 앞 토큰이 struct, unsigned 둘 다 아니라면
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) { // 이번 토큰이 두번째 토큰이고, 마지막 문자가 숫자나 알파벳이라면
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)// 첫번째 토큰이 extern, unsigned, gcc가 아니라면
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){ // 앞 토큰이 gcc라면
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0) // 앞앞 토큰이 unsigned, extern이 아니라면
					return false;
			}
			
		}

		if((row == 0 && !strcmp(tokens[row], "gcc")) ){ // 이번에 구한 토큰이 gcc라면
			clear_tokens(tokens); // 모든 token들을 0으로 초기확
			strcpy(tokens[0], str);	 // tokens[0]에 str을 넣고
			return 1; // 1 리턴
		} 

		row++; // 다음 토큰을 얻기 위해 row + 1
	}

	if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))  // 앞의 토큰이 전부 '*'로 되어있고, 그 앞의 토큰의 마지막 문자가 숫자나 알파벳이 아니라면
		row--;				
	if(all_star(tokens[row - 1]) && row == 1) // 앞의 토큰이 전부 '*'로 되어있고, 이번에 구한 토큰이 두번째 토큰이라면
		row--;	

	for(i = 0; i < strlen(start); i++)   
	{
		if(start[i] == ' ')  // 공백문자라면
		{
			while(start[i] == ' ') // 공백문자 동안 pass
				i++;
			if(start[0]==' ') { // 첫 문자가 공백문자라면
				start += i; // 공백문자가 아닌 첫번째 문자로 start를 이동시킨다
				i = 0;
			}
			else // 첫 문자가 공백문자가 아니라면
				row++; // 다음 토큰
			
			i--;
		} 
		else // 공백문자가 아니라면
		{
			strncat(tokens[row], start + i, 1); // token[row]에 덧붙임
			if( start[i] == '.' && i<strlen(start)){ // 덧붙인 문자가 '.'라면
				while(start[i + 1] == ' ' && i < strlen(start)) // 뒤에 있는 공백들 pass
					i++;

			}
		}
		strcpy(tokens[row], ltrim(rtrim(tokens[row]))); // tokens[row] 앞, 뒤의 공백문자 제거

		if(!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")){  // 토큰이 -lpthread 라면
			strcat(tokens[row - 1], tokens[row]); // 앞의 토큰 뒤에 현재 토큰을 덧붙임
			memset(tokens[row], 0, sizeof(tokens[row])); // 현재 토큰 저장했던 부분 0으로 초기화
			row--;
		}
	 	else if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])  
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.') ){ // 이번에 구한 토큰이 첫번째 토큰이 아니고, 마지막 문자가 숫자나 알파벳이고, 앞 토큰이 gcc이거나, 앞 토큰의 마지막 문자가 숫자나 알파벳이거나, '.'이라면
			
			if(row > 1 && strcmp(tokens[row-2],"(") == 0) // 앞앞 토큰이 (라면
			{
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0) // 앞 토큰이 struct와 unsigned가 아니라면
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) { // 이 토큰이 두번째 토큰이고, 토큰의 마지막 문자가 숫자나 알파벳이라면
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)// 첫번째 토큰이 extern이나 unsigned, gcc가 아니라면
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){ // 앞 토큰이 gcc라면
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0) // 앞앞 토큰이 unsigned, extern이 아니라면
					return false;
			}
		} 
	}


	if(row > 0) // 첫번째 토큰이 아니라면
	{

		
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){ // 첫번째 토큰이 #include, include, struct라면
			clear_tokens(tokens); // 토큰 초기화
			strcpy(tokens[0], remove_extraspace(str)); // 토큰에 공백 제거한 str을 넣는다
		}
	}

	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){ // 첫번째 토큰이 gcc이거나 extern이라면
		for(i = 1; i < TOKEN_CNT; i++){ // 모든 토큰들 확인
			if(strcmp(tokens[i],"") == 0)   // 토큰이 null string이라면 반복 종료
				break;		       

			if(i != TOKEN_CNT -1 ) // 마지막 토큰이 아니라면
				strcat(tokens[0], " "); // 첫번째 토큰 뒤에 공백 추가
			strcat(tokens[0], tokens[i]); // 첫번째 토큰뒤에 tokens[i] 덧붙임
			memset(tokens[i], 0, sizeof(tokens[i])); // tokens[i] 0초기화
		}
	}
	
	
	while((p_str = find_typeSpecifier(tokens)) != -1){ // 토큰들 중 형식 지정자가 있다면
		if(!reset_tokens(p_str, tokens)) // 토큰들을 정리한다
			return false; // 잘못된게 발견되면 false 리턴
	}

	
	while((p_str = find_typeSpecifier2(tokens)) != -1){ // 토큰들 중 struct 형식 지정자가 있다면
		if(!reset_tokens(p_str, tokens)) // 토큰들을 정리한다
			return false; // 잘못된게 발견되면 false 리턴
	}
	
	return true;
}

node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses) // tokens에 들어있는 토큰들을 트리에 넣는 함수
{
	node *cur = root;
	node *new;
	node *saved_operator;
	node *operator;
	int fstart;
	int i;

	while(1)	
	{
		if(strcmp(tokens[*idx], "") == 0) // 토큰이 null string이라면
			break;
	
		if(!strcmp(tokens[*idx], ")")) // 토큰이 ) 라면
			return get_root(cur);

		else if(!strcmp(tokens[*idx], ",")) // 토큰이 , 라면
			return get_root(cur);

		else if(!strcmp(tokens[*idx], "(")) // 토큰이 ( 라면
		{
			
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){ // 이전 토큰이 연산자가 아니고, ','도 아니라면
				fstart = true;

				while(1)
				{
					*idx += 1; // 인덱스 증가

					if(!strcmp(tokens[*idx], ")")) // 토큰이 ) 라면
						break; // 반복 종료
					
					new = make_tree(NULL, tokens, idx, parentheses + 1); // 재귀 호출
					
					if(new != NULL){ // 위의 결과가 NULL이 아니라면
						if(fstart == true){
							cur->child_head = new; // 새로 만든 트리를 자식노드로 넣음
							new->parent = cur;
	
							fstart = false; // 새 트리 생성 종료
						}
						else{
							cur->next = new; // 형제 노드에 새 트리 추가
							new->prev = cur;
						}

						cur = new; // 새로 만든 트리로 이동
					}

					if(!strcmp(tokens[*idx], ")")) // 토큰이 ) 라면
						break; // 반복 종료
				}
			}
			else{ // 이전 토큰이 연산자 이거나 ',' 라면
				*idx += 1; // 인덱스 증가
	
				new = make_tree(NULL, tokens, idx, parentheses + 1); // 트리 생성 재귀 호출

				if(cur == NULL)
					cur = new;

				else if(!strcmp(new->name, cur->name)){ // new의 name과 cur의 name이 같다면
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||")
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))// 이 노드가 |, ||, &, && 연산자라면
					{
						cur = get_last_child(cur); // 제일 끝에 있는 자식노드로 이동

						if(new->child_head != NULL){ // 새로운 트리에 자식 노드가 존재한다면
							new = new->child_head; // 그 자식 노드로 이동

							// 새로 만든 트리에서 root 노드를 제거
							new->parent->child_head = NULL;
							new->parent = NULL;
							// 새로 생성된 트리의 앞쪽에 cur를 추가함  
							new->prev = cur; 
							cur->next = new;
						}
					}
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*")) // 이 노드가 +, * 연산자라면
					{
						i = 0;

						while(1)
						{
							if(!strcmp(tokens[*idx + i], "")) // 마지막 토큰이라면 break
								break;

							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0) // 토큰이 연산자이고, )가 아니라면 break
								break;

							i++; // 다음 토큰으로 이동
						}
						
						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name)) // 다음 토큰의 우선순위가 new 보다 더 낮다면
						{
							cur = get_last_child(cur); // 현재 노드의 가장 마지막 자식 노드 찾아온다
							cur->next = new; // 가장 마지막 자식 노드의 다음 형제 노드에 new를 추가
							new->prev = cur;
							cur = new; // new로 이동
						}
						else //  다음 토큰의 우선순위다 new 보다 더 높다면
						{
							cur = get_last_child(cur); // cur의 가장 마지막 자식 노드 찾아온다

							if(new->child_head != NULL){ // new에 자식 노드가 있다면
								new = new->child_head; // new의 루트 노드 제거

								// cur의 맨 마지막 자식 노드로 new 를 넣는다
								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					else{
						cur = get_last_child(cur); // cur의 가장 마지막 자식 노드 찾아온다
						cur->next = new; // cur의 다음 노드에 new를 추가한다
						new->prev = cur;
						cur = new;
					}
				}
	
				else
				{
					cur = get_last_child(cur); // cur의 가장 마지막 자식 노드 찾아온다

					cur->next = new; // cur의 다음 노드에 new를 추가한다
					new->prev = cur;
	
					cur = new;
				}
			}
		}
		else if(is_operator(tokens[*idx])) // 토큰이 연산자라면
		{
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&") 
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*")) // 토큰이 ||, &&, |, &, +, * 연산자라면
			{
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx])) // cur가 연산자이고, 현재  토큰과 동일하다면
					operator = cur;
		
				else
				{
					new = create_node(tokens[*idx], parentheses); // 새로운 노드 생성
					operator = get_most_high_precedence_node(cur, new); // 가장 우선순위가 높은 연산자를 찾는다

					if(operator->parent == NULL && operator->prev == NULL){ // 가장 우선순위가 높은 연산자 노드가 부모가 없고, 앞에 다른 형제도 없다면

						if(get_precedence(operator->name) < get_precedence(new->name)){ // 새로 만든 트리의 연산자 우선순위가 더 높다면
							cur = insert_node(operator, new); // operator 노드 자리에 new를 삽입
						}

						else if(get_precedence(operator->name) > get_precedence(new->name)) // operator 노드의 우선순위가 더 높다면
						{
							if(operator->child_head != NULL){ // operator 노드가 자식 노드를 갖고 있다면
								operator = get_last_child(operator); // 가장 마지막 자식 노드를 구한다
								cur = insert_node(operator, new); // 가장 마지막 자식 노드 자리에 new를 삽입
							}
						}
						else
						{
							operator = cur; 
	
							while(1)
							{
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx])) // operator가 연산자이고, 현재 토큰과 같다면
									break; // 반복 종료
						
								if(operator->prev != NULL) // operator 앞에 다른 형제 노드가 있다면
									operator = operator->prev; // 앞쪽 형제 노드로 이동
								else // 앞에 다른 형제 노드가 없다면
									break; // 반복 종료
							}

							if(strcmp(operator->name, tokens[*idx]) != 0) // operator의 토큰과 일치하지 않는다면
								operator = operator->parent; // operator의 부모 노드로 이동

							if(operator != NULL){ // operator가 NULL이 아니라면
								if(!strcmp(operator->name, tokens[*idx])) // operator의 토큰과 일치한다면
									cur = operator;
							}
						}
					}

					else
						cur = insert_node(operator, new); // operator 위치에 새로운 노드 삽입
				}

			}
			else
			{
				new = create_node(tokens[*idx], parentheses); // 새로운 노드 생성

				if(cur == NULL)
					cur = new;

				else
				{
					operator = get_most_high_precedence_node(cur, new); // 가장 우선순위가 높은 연산자를 찾는다

					if(operator->parentheses > new->parentheses) // 
						cur = insert_node(operator, new); // operator자리에 새로운 노드 삽입

					else if(operator->parent == NULL && operator->prev ==  NULL){ // operator가 루트노드이면
					
						if(get_precedence(operator->name) > get_precedence(new->name)) // operator가 new보다 연산자 우선순위가 높으면
						{
							if(operator->child_head != NULL){ // operator에 자식 노드가 있다면
	
								operator = get_last_child(operator); // operator의 마지막 자식 노드를 구한다
								cur = insert_node(operator, new); // 마지막 자식노드 위치에 새 노드를 삽입
							}
						}
					
						else	
							cur = insert_node(operator, new); // operator자리에 새 노드를 삽입
					}
	
					else
						cur = insert_node(operator, new); // operator자리에 새 노드를 삽입
				}
			}
		}
		else 
		{
			new = create_node(tokens[*idx], parentheses); // 새로운 노드 생성

			if(cur == NULL)
				cur = new;

			else if(cur->child_head == NULL){ // cur 노드에 자식 노드가 없다면
				cur->child_head = new; // new를 자식 노드로 넣는다
				new->parent = cur;

				cur = new; // 새 노드로 이동
			}
			else{

				cur = get_last_child(cur); // cur의 마지막 자식 노드로 이동

				cur->next = new; // cur의 마지막 자식 노드의 형제 노드로 새 노드를 넣는다
				new->prev = cur;

				cur = new; // 새 노드로 이동
			}
		}

		*idx += 1; // 인덱스 증가시킴
	}

	return get_root(cur); // 트리의 루트노드 구해서 리턴
}

node *change_sibling(node *parent) // 전달인자로 받은 노드의 자식 노드들이 저장된 순서를 바꾸는 함수
{
	node *tmp;
	
	tmp = parent->child_head;

	// 두번째 자식 노드를 첫번째 자식 노드 위치로 옮김
	parent->child_head = parent->child_head->next;
	parent->child_head->parent = parent;
	parent->child_head->prev = NULL;

	// 첫번째 자식 노드를 두번째 자식 노드 위치로 옮김
	parent->child_head->next = tmp;
	parent->child_head->next->prev = parent->child_head;
	parent->child_head->next->next = NULL;
	parent->child_head->next->parent = NULL;		

	return parent;
}

node *create_node(char *name, int parentheses) // 새로운 노드를 생성하는 함수
{
	node *new;

	new = (node *)malloc(sizeof(node)); // node 동적 할당
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1)); // node의 name 동적할당
	strcpy(new->name, name); // node에 name을 넣는다

	// node의 내용 초기화
	new->parentheses = parentheses;
	new->parent = NULL;
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new; // 생성된 노드 리턴
}

int get_precedence(char *op) // 연산자의 우선순위를 구하는 함수
{
	int i;

	for(i = 2; i < OPERATOR_CNT; i++){
		if(!strcmp(operators[i].operator, op)) // 전달인자와 같은 연산자를 찾는다
			return operators[i].precedence; // 해당 연산자의 우선순위를 리턴한다
	}
	return false; // 전달인자가 연산자가 아니었다면 false를 리턴한다
}

int is_operator(char *op) // 전달인자로 받은 문자가 연산자인지 확인하는 함수
{
	int i;

	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL) // 모든 문자열들과 비교했다면
			break; // 반복 중지
		if(!strcmp(operators[i].operator, op)){	// 전달인자와 일치하는 연산자가 있다면
			return true; // true 리턴
		}
	}

	return false;// 전달인자와 일치하는 연산자가 없었다면 false 리턴
}

void print(node *cur)
{
	if(cur->child_head != NULL){ // 자식노드가 있다면
		print(cur->child_head); // 자식노드 출력
		printf("\n");
	}

	if(cur->next != NULL){ // 형제노드가 있다면
		print(cur->next); // 형제노드 출력
		printf("\t");
	}
	printf("%s", cur->name); // 
}

node *get_operator(node *cur)
{
	if(cur == NULL)
		return cur;

	if(cur->prev != NULL) // 앞쪽에 형제노드가 있다면
		while(cur->prev != NULL) // 더이상 앞에 형제노드가 없을 때까지
			cur = cur->prev; // 앞으로 이동

	return cur->parent; // 부모노드(연산자) 리턴
}

node *get_root(node *cur) // 트리의 루트를 찾아 리턴하는 함수
{
	if(cur == NULL)
		return cur;

	while(cur->prev != NULL) // 제일 앞쪽 노드로 이동 (제일 앞 노드에서 부모에게 갈 수 있음)
		cur = cur->prev;

	if(cur->parent != NULL) // 부모 노드로 이동
		cur = get_root(cur->parent);

	return cur; // 루트노드 리턴
}

node *get_high_precedence_node(node *cur, node *new) // 더 우선순위가 높은 노드를 찾는 함수
{
	if(is_operator(cur->name))
		if(get_precedence(cur->name) < get_precedence(new->name)) // cur의 연산자가 우선순위가 더 높다면
			return cur; // cur 리턴

	if(cur->prev != NULL){ // cur 앞에 형제노드가 있다면
		while(cur->prev != NULL){ // 맨 앞의 형제 노드까지 이동
			cur = cur->prev;
			
			return get_high_precedence_node(cur, new); // cur와 비교하며 재귀호출
		}


		if(cur->parent != NULL) // 부모노드가 있다면
			return get_high_precedence_node(cur->parent, new); // 부모노드와 비교하며 재귀호출
	}

	if(cur->parent == NULL) // 루트 노드라면
		return cur; // cur 리턴
}

node *get_most_high_precedence_node(node *cur, node *new) // 가장 우선순위가 높은 연산자를 구하는 함수
{
	node *operator = get_high_precedence_node(cur, new);
	node *saved_operator = operator;

	while(1)
	{
		if(saved_operator->parent == NULL)
			break;

		if(saved_operator->prev != NULL) // 저장해둔 노드의 앞에 다른 형제노드들이 있다면
			operator = get_high_precedence_node(saved_operator->prev, new); // 앞쪽의 형제노드와 새 노드 중 우선순위가 높은 노드 저장

		else if(saved_operator->parent != NULL) // 저장해둔 노드에게 부모노드가 있다면
			operator = get_high_precedence_node(saved_operator->parent, new); // 부모노드와 새 노드 중 우선순위가 높은 노드 저장

		saved_operator = operator;
	}
	
	return saved_operator; // 가장 연산자 우선순위 높은 노드 리턴
}

node *insert_node(node *old, node *new) // 새로운 노드를 old의 자리에 삽입하는 함수
{
	if(old->prev != NULL){ // old노드의 앞쪽에 다른 형제노드가 있다면
		// new 노드의 앞쪽에 그 형제노드를 넣는다
		new->prev = old->prev;
		// 그 형제노드의 뒤쪽에 new 노드를 넣는다
		old->prev->next = new;
		old->prev = NULL;
	}

	new->child_head = old;// old노드를 new 노드의 자식으로 넣는다
	old->parent = new; // old의 부모 노드의 자식노드를 old노드에서 new 노드로 바꾼다

	return new;
}

node *get_last_child(node *cur) // 제일 끝의 자식 노드 찾는 함수
{
	if(cur->child_head != NULL) // 자식노드가 있다면
		cur = cur->child_head; // 자식노드로 이동

	while(cur->next != NULL) // 다음 형제노드가 있다면
		cur = cur->next; // 다음 형제노드로 이동

	return cur; // 찾은 노드 리턴
}

int get_sibling_cnt(node *cur) // 형제 노드의 개수를 세는 함수
{
	int i = 0;

	while(cur->prev != NULL) // 제일 앞쪽 형제 노드로 이동
		cur = cur->prev;

	while(cur->next != NULL){ // 뒤쪽으로 이동하며 형제 노드의 수를 센다
		cur = cur->next;
		i++;
	}

	return i; // 형제 노드의 수 리턴
}

void free_node(node *cur) // 인자로 받은 노드 삭제하는 함수 (자식 노드, 뒤에 있는 형제노드들도 삭제)
{
	if(cur->child_head != NULL) // 자식 노드가 있었다면
		free_node(cur->child_head); // 자식 노드에 대하여 free_node() 호출

	if(cur->next != NULL) // 뒤에 다른 형제노드가 있다면
		free_node(cur->next); // 뒤에있던 형제노드들에 대하여 free_node() 호출

	if(cur != NULL){
		// 현재 노드를 free한다
		cur->prev = NULL;
		cur->next = NULL;
		cur->parent = NULL;
		cur->child_head = NULL;
		free(cur);
	}
}


int is_character(char c) // c가 알파벳, 숫자인지 체크하는 함수
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); // 알파벳 대소문자 또는 숫자로 된 문자라면 1 리턴
}

int is_typeStatement(char *str) // type이 맨 앞에 있는 구문(선언문?)인지 확인하는 함수
{ 
	char *start;
	char str2[BUFLEN] = {0}; 
	char tmp[BUFLEN] = {0}; 
	char tmp2[BUFLEN] = {0}; 
	int i;	 
	
	start = str;
	strncpy(str2,str,strlen(str)); // str2에 str 복사
	remove_space(str2); // str2에 있는 공백문자들 제거

	while(start[0] == ' ') // start 앞에 있는 공백문자들 제거
		start += 1;

	if(strstr(str2, "gcc") != NULL) // str2에 "gcc"라는 문자열이 포함되어 있다면
	{
		strncpy(tmp2, start, strlen("gcc")); // tmp2에 start의 앞에서 세문자(strlen("gcc")) 복사
		if(strcmp(tmp2,"gcc") != 0) // tmp2(start의 맨 앞 문자 세개)가 gcc가 아니라면
			return 0; // 0 리턴
		else // gcc라면
			return 2; // 2 리턴
	}
	
	for(i = 0; i < DATATYPE_SIZE; i++)
	{
		if(strstr(str2,datatype[i]) != NULL) // str2에 특정 데이터 타입이 포함되어 있다면
		{	
			strncpy(tmp, str2, strlen(datatype[i])); // str2의 맨 앞에 있는 단어 tmp로 복사
			strncpy(tmp2, start, strlen(datatype[i])); // start의 맨 앞에 있는 단어 tmp로 복사
			
			if(strcmp(tmp, datatype[i]) == 0) // 위에서 복사한 단어가 해당 데이터 타입 이라면
				if(strcmp(tmp, tmp2) != 0) // tmp와 tmp2가 같다면
					return 0; // 0 리턴
				else // tmp와 tmp2가 다르다면
					return 2; // 2 리턴
		}

	}
	return 1; // 위의 것들 중 아무것도 아니라면 1 리턴

}

int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]) // 토큰들 중에서 형식 지정자를 찾는 함수
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++) // 모든 토큰들 확인
	{
		for(j = 0; j < DATATYPE_SIZE; j++) // 모든 자료형 확인
		{
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0) // 토큰에 datatype이 포함되어 있다면
			{
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")") // 앞 토큰이 ( 이고, 뒤 토큰이 ) 이고
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*'
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '(' 
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+'
							|| is_character(tokens[i + 2][0]))) // 이 토큰 뒤에 있는 ) 바로 뒤에 &,*,),(,-,+,알파벳이나 숫자가 있으면
					return i; // 토큰의 인덱스 리턴
			}
		}
	}
	return -1; // 못찾았으면 -1 리턴
}

int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) // 토큰에서 struct 형식 지정자를 찾는 함수
{
    int i, j;

   
    for(i = 0; i < TOKEN_CNT; i++) // 모든 토큰들 확인
    {
        for(j = 0; j < DATATYPE_SIZE; j++) // 모든 자료형 확인
        {
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1])) // 토큰이 struct이고, 다음 토큰의 마지막 문자가 알파벳이나 숫자라면
                    return i; // 토큰의 인덱스 리턴
        }
    }
    return -1; // 찾지 못했다면 -1 리턴
}

int all_star(char *str) // 이 문자열이 전부 '*' 문자로 이루어졌는지 확인하는 함수
{
	int i;
	int length= strlen(str);
	
 	if(length == 0)	
		return 0;
	
	for(i = 0; i < length; i++)
		if(str[i] != '*')
			return 0; // 이 문자열에 하나라도 '*'이 아닌 문자가 포함되어 있다면 0 리턴
	return 1; // 문자열이 모두 '*' 문자로 이루어져 있다면 1 리턴

}

int all_character(char *str) // 인자로 받은 문자열이 전부 숫자나 영어 알파벳으로만 이루어져 있는지 확인하는 함수
{
	int i;

	for(i = 0; i < strlen(str); i++)
		if(is_character(str[i])) // 문자가 숫자나 알파벳으로 이루어져 있다면
			return 1; // 바로 1 리턴 -> ???
	return 0;
	
}

int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0; // lcount : start 토큰 왼쪽의 여는 괄호 개수, rcount : 오른쪽의 닫는 괄호 개수
	int sub_lcount = 0, sub_rcount = 0;

	if(start > -1){ // start가 음수가 아니라면
		if(!strcmp(tokens[start], "struct")) { // start 토큰이 struct 라면
			strcat(tokens[start], " "); // start 토큰(형식지정자) 뒤에 공백 추가
			strcat(tokens[start], tokens[start+1]);	// start 토큰 뒤에 start + 1 토큰 덧붙임

			for(i = start + 1; i < TOKEN_CNT - 1; i++){ // start 토큰(형식지정자) 다음 토큰부터 마지막 토큰까지
				// 토큰들 앞으로 한칸씩 이동
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) { // start 토큰이 unsigned이고, 그 다음 토큰이 가 아니라면
			strcat(tokens[start], " "); // start 토큰 뒤에 형식 지정자 추가
			strcat(tokens[start], tokens[start + 1]); // 다음 토큰 start 토큰에 덧붙임
			strcat(tokens[start], tokens[start + 2]); // 다다음 토큰 strat 토큰에 덧붙임

			for(i = start + 1; i < TOKEN_CNT - 1; i++){ // start 다음 토큰부터 마지막 토큰까지 ---
				// 토큰들 앞으로 한칸씩 이동
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

     		j = start + 1; // j에 start 다음 토큰의 인덱스를 넣는다
        	while(!strcmp(tokens[j], ")")){ // j번째 토큰이 ) 라면
                	rcount ++; // 오른쪽 ) 괄호 개수 + 1
                	if(j==TOKEN_CNT) // 토큰 끝까지 전부 확인했다면
                        	break; // 반복 종료
                	j++; // 다음 토큰으로
        	}
	
		j = start - 1; // j에 start 이전 토큰의 인덱스를 넣는다
		while(!strcmp(tokens[j], "(")){ // j번째 토큰이 ( 라면
        	        lcount ++;// 왼쪽 ( 괄호 개수 + 1
                	if(j == 0) // 모든 토큰 확인했다면
                        	break; // 반복 종료
               		j--; // 다음 토큰으로
		}
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0) // j가 첫번째 토큰이 아니고 마지막 문자가 알파벳이나 숫자이거나 또는 첫번째 토큰이라면
			lcount = rcount; // lcount에 rcount를 넣는다

		if(lcount != rcount ) // 괄호의 짝이 맞지 않는다면
			return false;

		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){ // start 토큰 왼쪽에 ( 가 아닌 문자가 있고, 그 (들 바로 앞의 토큰이 sizeof라면
			return true; // true 리턴
		}
		
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) { //start 뒤의 토큰이 )이고, start토큰이 unsigned 또는 struct이면
			strcat(tokens[start - lcount], tokens[start]); // start - lcount 번째 토큰에 start 토큰을 덧붙임 ('(' 괄호들 삭제)
			strcat(tokens[start - lcount], tokens[start + 1]); // start - lcount 번째 토큰에 start + 1 번째 토큰을 덧붙임
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]); // start - lount 번째 토큰의 다음 토큰에 start + rcount번째 토큰을 덧붙임
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]); // i + lcount + rcount 번째 토큰부터 제일 끝에 있는 토큰까지 i번째 토큰으로 앞당긴다
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0])); // 앞으로 당겨온 토큰들 0초기화
			}


		}
 		else{
			if(tokens[start + 2][0] == '('){ // start 다다음 토큰이 ( 라면
				j = start + 2;
				while(!strcmp(tokens[j], "(")){ // (가 아닌 토큰이 나올때까지 반복한다
					sub_lcount++; // sub_lcount 하나씩 증가시킨다
					j++;
				} 	
				if(!strcmp(tokens[j + 1],")")){ // j + 1번째 토큰이 )라면
					j = j + 1;
					while(!strcmp(tokens[j], ")")){ // )가 아닌 토큰이 나올때까지 반복한더
						sub_rcount++; // sub_rcount를 증가시킨다
						j++;
					}
				}
				else 
					return false;

				if(sub_lcount != sub_rcount) // sub_lcount와 sub_rcount가 다르다면
					return false;
				
				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]); // 토큰 앞으로 당겨온다
				for(int i = start + 3; i<TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0])); // 뒤에 있던 토큰들 0초기화

			}
			strcat(tokens[start - lcount], tokens[start]); // 왼쪽에 있던 괄호 앞 토큰에 start 토큰 덧붙임
			strcat(tokens[start - lcount], tokens[start + 1]); // start 토큰의 바로 뒤에 있던 토큰 덧붙임
			strcat(tokens[start - lcount], tokens[start + rcount + 1]); // 오른쪽에 있던 괄호 뒤의 토큰 덧붙임
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) { // start 토큰 주변에 있던 괄호들을 없앤다
				strcpy(tokens[i], tokens[i + lcount + rcount +1]);
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));

			}
		}
	}
	return true;
}

void clear_tokens(char tokens[TOKEN_CNT][MINLEN]) // 전달인자로 받은 token 배열을 0으로 초기화하는 함수
{
	int i;

	for(i = 0; i < TOKEN_CNT; i++)
		memset(tokens[i], 0, sizeof(tokens[i]));
}

char *rtrim(char *_str)
{
	char tmp[BUFLEN];
	char *end;

	strcpy(tmp, _str);
	end = tmp + strlen(tmp) - 1; // tmp의 마지막 문자 위치를 가리키는 포인터
	while(end != _str && isspace(*end)) // 문자열 끝(오른쪽)부터 앞쪽으로 이동하며 해당 문자가 white space가 아닐 때까지 이동
		--end;

	*(end + 1) = '\0';
	_str = tmp; // 전달인자로 받은 _str에 tmp를 넣어서
	return _str; // 리턴
}

char *ltrim(char *_str)
{
	char *start = _str; // _str의 맨 앞 문자를 가리키는 포인터

	while(*start != '\0' && isspace(*start)) // 문자열 맨 앞(왼쪽)부터 뒤쪽으로 이동하며 해당 문자가 white space 또는 널문자가 아닐 때까지 이동
		++start;
	_str = start;
	return _str;
}

char* remove_extraspace(char *str) // 의미없는 공백 지우는 함수
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN); // 결과 저장할 문자열 동적할당
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	if(strstr(str,"include<")!=NULL){ // 전달인자로 받은 문자열에 "include<"가 포함되어 있다면
		start = str; // 시작주소 설정
		end = strpbrk(str, "<"); // 끝주소 설정
		position = end - start;
	
		strncat(temp, str, position); // str에서  position 길이만큼 temp에 갖다붙임
		strncat(temp, " ", 1); // temp뒤에 공백 추가
		strncat(temp, str + position, strlen(str) - position + 1); // temp 뒤에 str의 나머지 부분 갖다 붙임

		str = temp; // str을 temp로 바꿈
	}
	
	for(i = 0; i < strlen(str); i++) // 문자열 내 모든 문자 확인
	{
		if(str[i] ==' ') // 공백 문자라면
		{
			if(i == 0 && str[0] ==' ') // 첫번째 문자가 공백문자라면
				while(str[i + 1] == ' ') // 공백문자가 아닌 문자가 나올때까지
					i++; // 다음 문자로 이동
			else{
				if(i > 0 && str[i - 1] != ' ') // 앞문자가 공백문자가 아니라면
					str2[strlen(str2)] = str[i]; // str2에 넣는다
				while(str[i + 1] == ' ')// 공백문자가 아닌 문자가 나올때까지
					i++; // 다음 문자로 이동
			} 
		}
		else
			str2[strlen(str2)] = str[i]; // str2에 넣는다
	}

	return str2; // 쓸데없는 공백들 제거한 새로운 문자열 return
}



void remove_space(char *str) // 문자열 내에 있는 공백문자 지우는 함수
{
	char* i = str;
	char* j = str;
	
	while(*j != 0)
	{
		*i = *j++;
		if(*i != ' ')
			i++;
	}
	*i = 0;
}

int check_brackets(char *str) // 괄호가 짝을 맞춰 제대로 있는지 검사하는 함수
{
	char *start = str;
	int lcount = 0, rcount = 0; // lcount는 여는 괄호 개수, rcount는 닫는 괄호 개수
	
	while(1){
		if((start = strpbrk(start, "()")) != NULL){ // '(',')'이 있는 위치 리턴
			if(*(start) == '(') // 여는 괄호라면
				lcount++;
			else // 닫는 괄호라면
				rcount++;

			start += 1; // 뒤쪽 문자열 검사를 위해 + 1
		}
		else // 괄호 더이상 찾지 못했다면 반복 종료
			break;
	}

	if(lcount != rcount) // 여는 괄호와 닫는 괄호의 개수가 다르면 잘못된 것
		return 0; // 괄호가 잘못됐으면 0 리턴
	else 
		return 1; // 괄호 개수가 일치하면 1 리턴
}

int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]) // 토큰의 총 개수 리턴하는 함수
{
	int i;
	
	for(i = 0; i < TOKEN_CNT; i++)
		if(!strcmp(tokens[i], "")) // 토큰이 nullstring이라면
			break; // 반복 종료

	return i; // 반복한 횟수 리턴
}
