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
			
			if(row > 1 && strcmp(tokens[row-2],"(") == 0)
			{
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		} 
	}


	if(row > 0)
	{

		
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){ 
			clear_tokens(tokens); 
			strcpy(tokens[0], remove_extraspace(str)); 
		}
	}

	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){
		for(i = 1; i < TOKEN_CNT; i++){
			if(strcmp(tokens[i],"") == 0)  
				break;		       

			if(i != TOKEN_CNT -1 )
				strcat(tokens[0], " ");
			strcat(tokens[0], tokens[i]);
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}
	
	
	while((p_str = find_typeSpecifier(tokens)) != -1){ 
		if(!reset_tokens(p_str, tokens))
			return false;
	}

	
	while((p_str = find_typeSpecifier2(tokens)) != -1){  
		if(!reset_tokens(p_str, tokens))
			return false;
	}
	
	return true;
}

node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;
	node *new;
	node *saved_operator;
	node *operator;
	int fstart;
	int i;

	while(1)	
	{
		if(strcmp(tokens[*idx], "") == 0)
			break;
	
		if(!strcmp(tokens[*idx], ")"))
			return get_root(cur);

		else if(!strcmp(tokens[*idx], ","))
			return get_root(cur);

		else if(!strcmp(tokens[*idx], "("))
		{
			
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){
				fstart = true;

				while(1)
				{
					*idx += 1;

					if(!strcmp(tokens[*idx], ")"))
						break;
					
					new = make_tree(NULL, tokens, idx, parentheses + 1);
					
					if(new != NULL){
						if(fstart == true){
							cur->child_head = new;
							new->parent = cur;
	
							fstart = false;
						}
						else{
							cur->next = new;
							new->prev = cur;
						}

						cur = new;
					}

					if(!strcmp(tokens[*idx], ")"))
						break;
				}
			}
			else{
				*idx += 1;
	
				new = make_tree(NULL, tokens, idx, parentheses + 1);

				if(cur == NULL)
					cur = new;

				else if(!strcmp(new->name, cur->name)){
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||") 
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						cur = get_last_child(cur);

						if(new->child_head != NULL){
							new = new->child_head;

							new->parent->child_head = NULL;
							new->parent = NULL;
							new->prev = cur;
							cur->next = new;
						}
					}
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;

						while(1)
						{
							if(!strcmp(tokens[*idx + i], ""))
								break;

							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;
						}
						
						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							cur = get_last_child(cur);
							cur->next = new;
							new->prev = cur;
							cur = new;
						}
						else
						{
							cur = get_last_child(cur);

							if(new->child_head != NULL){
								new = new->child_head;

								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					else{
						cur = get_last_child(cur);
						cur->next = new;
						new->prev = cur;
						cur = new;
					}
				}
	
				else
				{
					cur = get_last_child(cur);

					cur->next = new;
					new->prev = cur;
	
					cur = new;
				}
			}
		}
		else if(is_operator(tokens[*idx]))
		{
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&") 
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;
		
				else
				{
					new = create_node(tokens[*idx], parentheses);
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parent == NULL && operator->prev == NULL){

						if(get_precedence(operator->name) < get_precedence(new->name)){
							cur = insert_node(operator, new);
						}

						else if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
						else
						{
							operator = cur;
	
							while(1)
							{
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;
						
								if(operator->prev != NULL)
									operator = operator->prev;
								else
									break;
							}

							if(strcmp(operator->name, tokens[*idx]) != 0)
								operator = operator->parent;

							if(operator != NULL){
								if(!strcmp(operator->name, tokens[*idx]))
									cur = operator;
							}
						}
					}

					else
						cur = insert_node(operator, new);
				}

			}
			else
			{
				new = create_node(tokens[*idx], parentheses);

				if(cur == NULL)
					cur = new;

				else
				{
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parentheses > new->parentheses)
						cur = insert_node(operator, new);

					else if(operator->parent == NULL && operator->prev ==  NULL){
					
						if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
	
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
					
						else	
							cur = insert_node(operator, new);
					}
	
					else
						cur = insert_node(operator, new);
				}
			}
		}
		else 
		{
			new = create_node(tokens[*idx], parentheses);

			if(cur == NULL)
				cur = new;

			else if(cur->child_head == NULL){
				cur->child_head = new;
				new->parent = cur;

				cur = new;
			}
			else{

				cur = get_last_child(cur);

				cur->next = new;
				new->prev = cur;

				cur = new;
			}
		}

		*idx += 1;
	}

	return get_root(cur);
}

node *change_sibling(node *parent)
{
	node *tmp;
	
	tmp = parent->child_head;

	parent->child_head = parent->child_head->next;
	parent->child_head->parent = parent;
	parent->child_head->prev = NULL;

	parent->child_head->next = tmp;
	parent->child_head->next->prev = parent->child_head;
	parent->child_head->next->next = NULL;
	parent->child_head->next->parent = NULL;		

	return parent;
}

node *create_node(char *name, int parentheses)
{
	node *new;

	new = (node *)malloc(sizeof(node));
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(new->name, name);

	new->parentheses = parentheses;
	new->parent = NULL;
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

int get_precedence(char *op)
{
	int i;

	for(i = 2; i < OPERATOR_CNT; i++){
		if(!strcmp(operators[i].operator, op))
			return operators[i].precedence;
	}
	return false;
}

int is_operator(char *op)
{
	int i;

	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL)
			break;
		if(!strcmp(operators[i].operator, op)){
			return true;
		}
	}

	return false;
}

void print(node *cur)
{
	if(cur->child_head != NULL){
		print(cur->child_head);
		printf("\n");
	}

	if(cur->next != NULL){
		print(cur->next);
		printf("\t");
	}
	printf("%s", cur->name);
}

node *get_operator(node *cur)
{
	if(cur == NULL)
		return cur;

	if(cur->prev != NULL)
		while(cur->prev != NULL)
			cur = cur->prev;

	return cur->parent;
}

node *get_root(node *cur)
{
	if(cur == NULL)
		return cur;

	while(cur->prev != NULL)
		cur = cur->prev;

	if(cur->parent != NULL)
		cur = get_root(cur->parent);

	return cur;
}

node *get_high_precedence_node(node *cur, node *new)
{
	if(is_operator(cur->name))
		if(get_precedence(cur->name) < get_precedence(new->name))
			return cur;

	if(cur->prev != NULL){
		while(cur->prev != NULL){
			cur = cur->prev;
			
			return get_high_precedence_node(cur, new);
		}


		if(cur->parent != NULL)
			return get_high_precedence_node(cur->parent, new);
	}

	if(cur->parent == NULL)
		return cur;
}

node *get_most_high_precedence_node(node *cur, node *new)
{
	node *operator = get_high_precedence_node(cur, new);
	node *saved_operator = operator;

	while(1)
	{
		if(saved_operator->parent == NULL)
			break;

		if(saved_operator->prev != NULL)
			operator = get_high_precedence_node(saved_operator->prev, new);

		else if(saved_operator->parent != NULL)
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;
	}
	
	return saved_operator;
}

node *insert_node(node *old, node *new)
{
	if(old->prev != NULL){
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}

	new->child_head = old;
	old->parent = new;

	return new;
}

node *get_last_child(node *cur)
{
	if(cur->child_head != NULL)
		cur = cur->child_head;

	while(cur->next != NULL)
		cur = cur->next;

	return cur;
}

int get_sibling_cnt(node *cur)
{
	int i = 0;

	while(cur->prev != NULL)
		cur = cur->prev;

	while(cur->next != NULL){
		cur = cur->next;
		i++;
	}

	return i;
}

void free_node(node *cur)
{
	if(cur->child_head != NULL)
		free_node(cur->child_head);

	if(cur->next != NULL)
		free_node(cur->next);

	if(cur != NULL){
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

int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]) 
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++)
	{
		for(j = 0; j < DATATYPE_SIZE; j++)
		{
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")") 
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*' 
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '(' 
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+' 
							|| is_character(tokens[i + 2][0])))  
					return i;
			}
		}
	}
	return -1;
}

int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) 
{
    int i, j;

   
    for(i = 0; i < TOKEN_CNT; i++)
    {
        for(j = 0; j < DATATYPE_SIZE; j++)
        {
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))  
                    return i;
        }
    }
    return -1;
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

int all_character(char *str)
{
	int i;

	for(i = 0; i < strlen(str); i++)
		if(is_character(str[i]))
			return 1;
	return 0;
	
}

int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;
	int sub_lcount = 0, sub_rcount = 0;

	if(start > -1){
		if(!strcmp(tokens[start], "struct")) {		
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start+1]);	     

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) {		
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start + 1]);	     
			strcat(tokens[start], tokens[start + 2]);

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

     		j = start + 1;           
        	while(!strcmp(tokens[j], ")")){
                	rcount ++;
                	if(j==TOKEN_CNT)
                        	break;
                	j++;
        	}
	
		j = start - 1;
		while(!strcmp(tokens[j], "(")){
        	        lcount ++;
                	if(j == 0)
                        	break;
               		j--;
		}
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0)
			lcount = rcount;

		if(lcount != rcount )
			return false;

		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){
			return true; 
		}
		
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) {		
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));
			}


		}
 		else{
			if(tokens[start + 2][0] == '('){
				j = start + 2;
				while(!strcmp(tokens[j], "(")){
					sub_lcount++;
					j++;
				} 	
				if(!strcmp(tokens[j + 1],")")){
					j = j + 1;
					while(!strcmp(tokens[j], ")")){
						sub_rcount++;
						j++;
					}
				}
				else 
					return false;

				if(sub_lcount != sub_rcount)
					return false;
				
				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);	
				for(int i = start + 3; i<TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));

			}
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) {
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

char* remove_extraspace(char *str)
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	if(strstr(str,"include<")!=NULL){
		start = str;
		end = strpbrk(str, "<");
		position = end - start;
	
		strncat(temp, str, position);
		strncat(temp, " ", 1);
		strncat(temp, str + position, strlen(str) - position + 1);

		str = temp;		
	}
	
	for(i = 0; i < strlen(str); i++)
	{
		if(str[i] ==' ')
		{
			if(i == 0 && str[0] ==' ')
				while(str[i + 1] == ' ')
					i++;	
			else{
				if(i > 0 && str[i - 1] != ' ')
					str2[strlen(str2)] = str[i];
				while(str[i + 1] == ' ')
					i++;
			} 
		}
		else
			str2[strlen(str2)] = str[i];
	}

	return str2;
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

int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	
	for(i = 0; i < TOKEN_CNT; i++)
		if(!strcmp(tokens[i], ""))
			break;

	return i;
}
