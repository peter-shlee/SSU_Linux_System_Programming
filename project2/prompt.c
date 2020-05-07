#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void doDeleteCommand();
void doExitCommand();
void doHelpCommand();
void doTreeCommand();
void printTree(const char *dirname, int depth);

int main(void)
{
	char command_buf[BUFFER_SIZE];
	char *command;

	while(1) {
		printf("20160548>");
		fgets(command_buf, sizeof(command_buf), stdin);
		command_buf[strlen(command_buf) - 1] = '\0';
		command = strtok(command_buf, " ");

		if (!strcmp(command, "DELETE") || !strcmp(command, "delete")) {
			doDeleteCommand();
		} else if (!strcmp(command, "EXIT") || !strcmp(command, "exit")) {
			doExitCommand();
			break;
		} else if (!strcmp(command, "TREE") || !strcmp(command, "tree")) {
			doTreeCommand();
		} else {
			doHelpCommand();
		}
	}

	exit(0);
}

void doDeleteCommand(){
	char *filename;
	char *endTime;
	char *option;

	filename = strtok(NULL, " ");
	endTime = strtok(NULL, " ");
	option = strtok(NULL, " ");

	printf("%s  %s  %s\n", filename, endTime, option);
	return;
}

void doExitCommand(){

}

void doHelpCommand(){

}

void doTreeCommand(){
	int i;
	printf("check");
	for(i = 0; i < 21 - strlen("check"); ++i) {
		printf("━");
	}
	printTree("check", 1);
	printf("\n");	
	chdir("..");
}

void printTree(const char *dirname, int depth){
	int i;
	int firstFileFlag = 1;
	char filename[MAXNAMLEN];
	struct dirent *dentry;
	struct stat statbuf;
	struct dirent **nextdirlist;
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		fprintf(stderr, "opendir error\n");
		exit(1);
	}
	chdir(dirname);


	while((dentry = readdir(dir)) != NULL) {
		if (dentry->d_ino == 0) continue;

		memcpy(filename, dentry->d_name, MAXNAMLEN);
		if (!strcmp(filename, ".") || !strcmp(filename, "..")) continue;


		if (stat(filename, &statbuf) == -1) {
			fprintf(stderr, "stat error for %s\n", filename);
			printf("ERROR:%s\n", strerror(errno));
			exit(1);
		}

// ┃┗┳┣ ━
		if (!firstFileFlag) {
			for (i = 0; i < depth; ++i) {
				printf("%24s", "┃");
			}
			printf("\n");
			for (i = 0; i < depth; ++i) {
				if (i + 1 == depth)
					printf("%24s", "┣");
				else 
					printf("%24s", "┃");
			}
			printf("%s", filename);

		} else {
			printf("┳%s", filename);
			firstFileFlag = 0;

		}
		if (S_ISDIR(statbuf.st_mode)) {
			int cnt;
			if ((cnt = scandir(filename, &nextdirlist, NULL, NULL)) > 2) {
				for(i = 0; i < 21 - strlen(filename); ++i) {
					printf("━");
				}
				// 재귀호출
				printTree(filename, depth + 1);
				// 작업 후 원래 디렉토리로 복귀하기
				chdir("..");

			}

			if(cnt != -1)
				free(nextdirlist);
		}
		printf("\n");	

	}

	for (i = 0; i < depth; ++i) {
		if (i + 1 == depth)
			printf("%24s", "┻");
		else 
			printf("%24s", "┃");
	}

	if (closedir(dir) < 0) {
		fprintf(stderr, "closedir error\n");
		exit(1);
	}
}
