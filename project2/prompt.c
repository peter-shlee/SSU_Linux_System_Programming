#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void doDeleteCommand();
void doExitCommand();
void doHelpCommand();
void doTreeCommand();
void printTree(const char *dirname, int depth);

struct DeleteFileInfo{
	char *absolutePathName;
	int checkBeforeDelete;
	int deleteImmediately;
	time_t deleteTime;
};

struct DeleteFileHeap{
	struct DeleteFileInfo *deleteFiles[BUFFER_SIZE];
	int size;
};

struct DeleteFileHeap deleteFileHeap;

void pushToHeap(struct DeleteFileInfo* newElement);
struct DeleteFileInfo *popFromHeap();
void checkFileDeleteTime();
void deleteFile(struct DeleteFileInfo *deleteFileInfo);


void checkFileDeleteTime(){
	time_t tmptime;
	time_t currentTime;
	tmptime = time(NULL);
	currentTime = mktime(localtime(&tmptime));
	printf("curtime : %ld\n", currentTime);
	while(1) {
		if (deleteFileHeap.size != 0 && deleteFileHeap.deleteFiles[0]->deleteTime < currentTime) {
			printf("delete time : %ld, current time : %ld\n", deleteFileHeap.deleteFiles[0]->deleteTime, currentTime);
			deleteFile(popFromHeap());
		} else {
			break;
		}
	}
}

void deleteFile(struct DeleteFileInfo *deleteFileInfo){
	if(deleteFileInfo->checkBeforeDelete) { // r옵션 수행
		char yesOrNo;
		printf("Delete [y/n]? ");
		scanf("%c", &yesOrNo);
		while(getchar()!='\n');

		if (yesOrNo != 'y') return;
	}

	if(deleteFileInfo->deleteImmediately) { // i옵션 수행
		if (remove(deleteFileInfo->absolutePathName) < 0) {
			fprintf(stderr, "remove error for %s\n", deleteFileInfo->absolutePathName);
			fprintf(stderr, "%s\n", strerror(errno));
		}
	} else { // i옵션 아닐 때 (trash 디렉토리 이용)
		// trash/file 디렉토리로 이동할 파일의 파일명은 절대경로 + 구분자 + 삭제된 시간으로 하기? 삭제된 시간만 이용? .... 삭제된 시간 + 파일명만?...
		//
		// info디렉토리에 들어갈 파일 이름은 정해져있음 절대경로를 제외하고 최종 파일 이름만 포함되어야 함. 중복되면 숫자붙이자

		//info 디렉토리의 크기가 2kb가 넘으면 오래된 파일부터 삭제해야함 - 오래된 파일 찾는건 mtime 이용하면 될듯
	}

	free(deleteFileInfo);
}

int main(void)
{
	char command_buf[BUFFER_SIZE];
	char *command;
////////////////////////////////////////////
//struct DeleteFileInfo *testElement = (struct DeleteFileInfo *) malloc(sizeof(struct DeleteFileInfo));
//testElement->absolutePathName = "test Element1";
//testElement->deleteTime = 1000;
//pushToHeap(testElement);
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//
//testElement = (struct DeleteFileInfo *) malloc(sizeof(struct DeleteFileInfo));
//testElement->absolutePathName = "test Element2";
//testElement->deleteTime = 500;
//pushToHeap(testElement);
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//
//testElement = (struct DeleteFileInfo *) malloc(sizeof(struct DeleteFileInfo));
//testElement->absolutePathName = "test Element3";
//testElement->deleteTime = 700;
//pushToHeap(testElement);
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//
//testElement = (struct DeleteFileInfo *) malloc(sizeof(struct DeleteFileInfo));
//testElement->absolutePathName = "test Element4";
//testElement->deleteTime = 100;
//pushToHeap(testElement);
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//
//
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//popFromHeap();
//
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//popFromHeap();
//
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//popFromHeap();
//
//printf("%s, %ld\n", deleteFileHeap.deleteFiles[0]->absolutePathName, deleteFileHeap.deleteFiles[0]->deleteTime);
//popFromHeap();
///////////////////////////////////////////



	while(1) {
		printf("20160548>");
		fgets(command_buf, sizeof(command_buf), stdin);
		if(command_buf[0] == '\n') continue;
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
		checkFileDeleteTime();
	}

	exit(0);
}

void doDeleteCommand(){
	struct DeleteFileInfo *newDeleteFileInfo;
	char *filename;
	char *absolutePathName;
	char *nextToken;
	const char *filesDirPath = "trash/files/";
	const char *infoDirPath = "trash/info/";
	char *END_DATE = NULL;
	char *END_TIME = NULL;
	int i_option = 0;
	int r_option = 0;
	int i;
	time_t deleteTime = 0;

	nextToken = strtok(NULL, " ");
	filename = nextToken;

	nextToken = strtok(NULL, " ");
	if (nextToken == NULL) { // 바로 삭제

	}else if (nextToken[0] == '-') { // END_TIME 주어지지 않고 옵션이 전달된 경우
		// 옵션 확인
		if (nextToken[1] == 'i') i_option = 1;
		else if (nextToken[1] == 'r') r_option = 1;
		else;

		// 다음 옵션 확인
		nextToken = strtok(NULL, " ");
		if (nextToken == NULL) {
			;
		}else if (nextToken[0] == '-' && nextToken[1] == 'i') {
			i_option = 1;
		} else if (nextToken[0] == '-' && nextToken[1] == 'r') {
			r_option = 1;
		} else {
			return;
		}
	} else if ('0' <= nextToken[0] && nextToken[0] <= '9') { // END_TIME 옵션 전달된 경우
		END_DATE = nextToken;
		nextToken = strtok(NULL, " ");
		END_TIME = nextToken;
		if (END_TIME == NULL) return;
		// 옵션들 확인
		for (i = 0; i < 2; ++i) {
			nextToken = strtok(NULL, " ");
			if (nextToken == NULL) {
				break;
			}else if (nextToken[0] == '-' && nextToken[1] == 'i') {
				i_option = 1;
			} else if (nextToken[0] == '-' && nextToken[1] == 'r') {
				r_option = 1;
			} else {
				return;
			}

		}
	} else { // 잘못된 옵션 전달
		return;
	}

	printf("i_option : %d  r_option : %d  END_TIME : %s\n", i_option, r_option, END_TIME);


	if (chdir("trash") < 0) {
		if (mkdir("trash", 0766) < 0) {
			fprintf(stderr, "mkdir error\n");
		}
		chdir("trash");
	}

	if (chdir("files") < 0) {
		if (mkdir("files", 0766) < 0) {
			fprintf(stderr, "mkdir error\n");
		}
		chdir("files");
	}
	chdir("..");

	if (chdir("info") < 0) {
		if (mkdir("info", 0766) < 0) {
			fprintf(stderr, "mkdir error\n");
		}
		chdir("info");
	}
	chdir("..");
	chdir("..");
	chdir("check");
	
	if (END_DATE != NULL && END_TIME != NULL) { // 삭제 예정 시간이 주어졌다면
		struct tm deletetm;
		char *dateToken;
		char *timeToken;

		deletetm.tm_year = atoi(strtok(END_DATE, "-")) - 1900;
		deletetm.tm_mon = atoi(strtok(NULL, "-")) - 1;
		deletetm.tm_mday = atoi(strtok(NULL, "-"));
		
		deletetm.tm_hour = atoi(strtok(END_TIME, ":"));
		deletetm.tm_min = atoi(strtok(NULL, ":"));
		deletetm.tm_sec = 0;

		printf("%d-%d-%d %d:%d\n", deletetm.tm_year, deletetm.tm_mon, deletetm.tm_mday, deletetm.tm_hour, deletetm.tm_min);

		deleteTime = mktime(&deletetm); // 삭제 시간을 time_t로 변환해 저장
	}

	newDeleteFileInfo = (struct DeleteFileInfo *) malloc(sizeof(newDeleteFileInfo));
	newDeleteFileInfo->absolutePathName = realpath(filename, NULL);
	if (i_option) {
		newDeleteFileInfo->deleteImmediately = 1;
	} else {
		newDeleteFileInfo->deleteImmediately = 0;
	}
	if (r_option) {
		newDeleteFileInfo->checkBeforeDelete = 1;
	} else {
		newDeleteFileInfo->checkBeforeDelete = 0;
	}
	newDeleteFileInfo->deleteTime = deleteTime;

	// 우선순위 큐에 넣기
	pushToHeap(newDeleteFileInfo);

	printf("deleteTime : %ld, curTime : %ld, i_option : %d, r_option : %d, absolutePathName = %s\n", newDeleteFileInfo->deleteTime, time(NULL), newDeleteFileInfo->deleteImmediately, newDeleteFileInfo->checkBeforeDelete, newDeleteFileInfo->absolutePathName);

	chdir("..");

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

void pushToHeap(struct DeleteFileInfo* newElement){
	int i;

	deleteFileHeap.deleteFiles[deleteFileHeap.size] = newElement;
	i = deleteFileHeap.size;

	while(deleteFileHeap.size != 0 && i >= 0) {
		if (deleteFileHeap.deleteFiles[i]->deleteTime < deleteFileHeap.deleteFiles[(i - 1) / 2]->deleteTime) {
			//printf("swap\n");
			struct DeleteFileInfo *tmp = deleteFileHeap.deleteFiles[i];
			deleteFileHeap.deleteFiles[i] = deleteFileHeap.deleteFiles[(i - 1) / 2];
			deleteFileHeap.deleteFiles[(i - 1) / 2] = tmp;
		} else {
			break;
		}
		i = (i - 1) / 2;
	}
	++(deleteFileHeap.size);
}

struct DeleteFileInfo *popFromHeap(){
	int i;
	struct DeleteFileInfo *popedData = deleteFileHeap.deleteFiles[0];
	deleteFileHeap.deleteFiles[0] = deleteFileHeap.deleteFiles[--(deleteFileHeap.size)];
	deleteFileHeap.deleteFiles[deleteFileHeap.size] = NULL;

	i = 0;
	while (i * 2 + 1 < deleteFileHeap.size) {
		int nextIndex = 0;
		if (i * 2 + 2 < deleteFileHeap.size){
			nextIndex = deleteFileHeap.deleteFiles[2 * i + 1]->deleteTime < deleteFileHeap.deleteFiles[2 * i + 2]->deleteTime ? 2 * i + 1 : 2 * i + 2;
		} else {
			nextIndex = 2 * i + 1;
		}

		if (deleteFileHeap.deleteFiles[i]->deleteTime > deleteFileHeap.deleteFiles[nextIndex]->deleteTime) {
			struct DeleteFileInfo *tmp = deleteFileHeap.deleteFiles[i];
			deleteFileHeap.deleteFiles[i] = deleteFileHeap.deleteFiles[nextIndex];
			deleteFileHeap.deleteFiles[nextIndex] = tmp;
		} else {
			break;
		}

		i = nextIndex;
	}
	
	return popedData;
}

void printHeap(){


}
