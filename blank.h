#ifndef BLANK_H_
#define BLANK_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef BUFLEN
	#define BUFLEN 1024
#endif

#define OPERATOR_CNT 24 // 연산자 개수
#define DATATYPE_SIZE 35 // 데이터 타입 개수
#define MINLEN 64
#define TOKEN_CNT 50 // 토큰 최대 개수

typedef struct node{ // 트리의 노드
	int parentheses; // 몇번째 괄호 안에 있는 토큰인지
	char *name; // 토큰
	struct node *parent; // 부모 노드
	struct node *child_head; // 자식 노드
	struct node *prev; // 앞쪽 형제 노드
	struct node *next; // 뒤쪽 형제 노드
}node;

typedef struct operator_precedence{ // 연산자, 연산자 우선순위 함께 저장된 구조체
	char *operator; // 연산자
	int precedence; // 연산자 우선순위
}operator_precedence;

void compare_tree(node *root1,  node *root2, int *result);// 두 트리(답안)을 비교하여 같은 내용인지, 다른 내용인지 알아내는 함수
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);// tokens에 들어있는 토큰들을 트리에 넣는 함수
node *change_sibling(node *parent);// 전달인자로 받은 노드의 자식 노드들이 저장된 순서를 바꾸는 함수
node *create_node(char *name, int parentheses);// 새로운 노드를 생성하는 함수
int get_precedence(char *op);// 연산자의 우선순위를 구하는 함수
int is_operator(char *op);// 전달인자로 받은 문자가 연산자인지 확인하는 함수
void print(node *cur); // 노드 정보 출력
node *get_operator(node *cur); // 해당 노드에 대한 연산자 찾는 함수
node *get_root(node *cur); // 트리의 루트를 찾아 리턴하는 함수
node *get_high_precedence_node(node *cur, node *new);// 더 우선순위가 높은 노드를 찾는 함수
node *get_most_high_precedence_node(node *cur, node *new);// 가장 우선순위가 높은 연산자를 구하는 함수
node *insert_node(node *old, node *new);// 새로운 노드를 old의 자리에 삽입하는 함수
node *get_last_child(node *cur);// 제일 끝의 자식 노드 찾는 함수
void free_node(node *cur);// 인자로 받은 노드 삭제하는 함수 (자식 노드, 뒤에 있는 형제노드들도 삭제)
int get_sibling_cnt(node *cur);// 형제 노드의 개수를 세는 함수

int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);// 답안을 토큰으로 분해하는 함수
int is_typeStatement(char *str);// type이 맨 앞에 있는 구문(선언문?)인지 확인하는 함수
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);// 토큰들 중에서 형식 지정자를 찾는 함수
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]);// 토큰에서 struct 형식 지정자를 찾는 함수
int is_character(char c);// c가 알파벳, 숫자인지 체크하는 함수
int all_star(char *str);// 이 문자열이 전부 '*' 문자로 이루어졌는지 확인하는 함수
int all_character(char *str);// 인자로 받은 문자열이 전부 숫자나 영어 알파벳으로만 이루어져 있는지 확인하는 함수
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]);// 토큰들 정리하는 함수
void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);// 전달인자로 받은 token 배열을 0으로 초기화하는 함수
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);// 토큰의 총 개수 리턴하는 함수
char *rtrim(char *_str);// 문자열 오른쪽의 white space 제거
char *ltrim(char *_str);// 문자열 왼쪽의 white space 제거
void remove_space(char *str);// 문자열 내에 있는 공백문자 지우는 함수
int check_brackets(char *str);// 괄호가 짝을 맞춰 제대로 있는지 검사하는 함수
char* remove_extraspace(char *str); // 의미없는 공백 지우는 함수

#endif
