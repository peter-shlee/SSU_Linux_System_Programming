#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void startMntr(void);


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

struct infoFile{
	char filename[MAXNAMLEN];
	char deleteTime[100];
};

struct DeleteFileHeap deleteFileHeap;

int compareInfoFile(const void *a, const void *b){
	const struct infoFile *infoFileA = (const struct infoFile *)a;
	const struct infoFile *infoFileB = (const struct infoFile *)b;

	return strcmp(infoFileA->deleteTime, infoFileB->deleteTime);
}


void doDeleteCommand();
void doExitCommand();
void doHelpCommand();
void doTreeCommand();
void doRecoverCommand();
void doSizeCommand();
void printTree(const char *dirname, int depth);
void pushToHeap(struct DeleteFileInfo* newElement);
struct DeleteFileInfo *popFromHeap();
void alarmHandlercheckFileDeleteTime();
void checkFileDeleteTime();
void deleteFile(struct DeleteFileInfo *deleteFileInfo);
void checkAndEraseTrashFiles();
int compareInfoFile(const void *a, const void *b);
off_t getDirSize(const char *dirname);
void printDirSize(const char *dirname, const char *dirpath, int depth, int maxDepth);


char exec_path[PATH_MAX];
int deleteUsingRFlag = 0;

void doSizeCommand(){
	char *nextToken;
	char *filename;
	struct stat statbuf;
	off_t filesize;

	nextToken = strtok(NULL, " ");
	filename = nextToken;
	if (filename == NULL) return;

	if (stat(filename, &statbuf) == -1) {
		fprintf(stderr, "stat error for %s\n", filename);
		printf("ERROR:%s\n", strerror(errno));
		return;
	}

	nextToken = strtok(NULL, " ");
	if (nextToken == NULL) { // 해당 파일 정보만 출력
		// 디렉터리 사이즈 구해야됨
		if (S_ISDIR(statbuf.st_mode)) {
			filesize = getDirSize(filename);
		} else {
			filesize = statbuf.st_size;
		}
		printf("%ld\t./%s\n", filesize, filename);
		chdir("..");

	}else if (!strcmp(nextToken, "-d")) { // 
		char dirpath[PATH_MAX];
		int maxDepth;
		nextToken = strtok(NULL, " ");
		if(nextToken == NULL) return;
		maxDepth = atoi(nextToken);

		if (!S_ISDIR(statbuf.st_mode)) {
			filesize = statbuf.st_size;
			printf("%ld\t./%s\n", filesize, filename);
			return;
		}

		sprintf(dirpath, "./%s/", filename);
		printf("%ld\t./%s\n", getDirSize(filename), filename);
		chdir("..");
		if (maxDepth > 1) {
			printDirSize(filename, dirpath, 2, maxDepth);
		}
	} else {
		return;
	}

}

void printDirSize(const char *dirname, const char *dirpath, int depth, int maxDepth){
	int i;
	int firstFileFlag = 1;
	char filename[MAXNAMLEN];
	struct dirent *dentry;
	struct stat statbuf;
	struct dirent **nextdirlist;
	DIR *dir = opendir(dirname);
	char nextDirPath[PATH_MAX];
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

		if (S_ISDIR(statbuf.st_mode)) {
			int cnt;
			if ((cnt = scandir(filename, &nextdirlist, NULL, NULL)) >= 2) {
				// 재귀호출
				if(depth < maxDepth) {
					sprintf(nextDirPath, "%s%s/", dirpath, filename);
					printDirSize(filename, nextDirPath, depth + 1, maxDepth);
					chdir("..");
				}
				// 작업 후 원래 디렉토리로 복귀하기
				printf("%ld\t%s%s\n", getDirSize(filename), dirpath, filename);
				chdir("..");
			}

			if(cnt != -1)
				free(nextdirlist);
		} else {
			printf("%ld\t%s%s\n", statbuf.st_size, dirpath, filename);
		}
	}

	if (closedir(dir) < 0) {
		fprintf(stderr, "closedir error\n");
		exit(1);
	}
}

off_t getDirSize(const char *dirname){
	int i;
	int firstFileFlag = 1;
	char filename[MAXNAMLEN];
	struct dirent *dentry;
	struct stat statbuf;
	struct dirent **nextdirlist;
	DIR *dir = opendir(dirname);
	off_t totalSize = 0;
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

		if (S_ISDIR(statbuf.st_mode)) {
			int cnt;
			if ((cnt = scandir(filename, &nextdirlist, NULL, NULL)) >= 2) {
				// 재귀호출
				totalSize += getDirSize(filename);
				// 작업 후 원래 디렉토리로 복귀하기
				chdir("..");

			}

			if(cnt != -1)
				free(nextdirlist);
		} else {
			totalSize += statbuf.st_size;
		}
	}

	if (closedir(dir) < 0) {
		fprintf(stderr, "closedir error\n");
		exit(1);
	}

	return totalSize;
}

int main(void)
{
	char command_buf[BUFFER_SIZE];
	char *command;

	getcwd(exec_path, PATH_MAX);
	startMntr();
	setbuf(stdout, NULL);

	signal(SIGALRM, alarmHandlercheckFileDeleteTime);
	alarm(60);

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
		} else if (!strcmp(command, "RECOVER") || !strcmp(command, "recover")) {
			doRecoverCommand();
		} else if (!strcmp(command, "SIZE") || !strcmp(command, "size")) {
			doSizeCommand();
		} else {
			doHelpCommand();
		}
		chdir(exec_path);
		checkFileDeleteTime(); // 알람으로 1분마다 반복하도록 해보기
		deleteUsingRFlag = 0;
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

	//printf("i_option : %d  r_option : %d  END_TIME : %s\n", i_option, r_option, END_TIME);


	if (chdir("trash") < 0) {
		if (mkdir("trash", 0777) < 0) {
			fprintf(stderr, "mkdir error\n");
		}
		chdir("trash");
	}

	if (chdir("files") < 0) {
		if (mkdir("files", 0777) < 0) {
			fprintf(stderr, "mkdir error\n");
		}
		chdir("files");
	}
	chdir("..");

	if (chdir("info") < 0) {
		if (mkdir("info", 0777) < 0) {
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

		//printf("%d-%d-%d %d:%d\n", deletetm.tm_year, deletetm.tm_mon, deletetm.tm_mday, deletetm.tm_hour, deletetm.tm_min);

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

	//printf("deleteTime : %ld, curTime : %ld, i_option : %d, r_option : %d, absolutePathName = %s\n", newDeleteFileInfo->deleteTime, time(NULL), newDeleteFileInfo->deleteImmediately, newDeleteFileInfo->checkBeforeDelete, newDeleteFileInfo->absolutePathName);

	chdir("..");

	return;
}

void doExitCommand(){

}

void doHelpCommand(){
	printf("- DELETE 지정한 삭제 시간에 자동으로 파일을 삭제해주는 명령어\n");
	printf("\tusage : DELETE [FILENAME] [END_TIME] [OPTION]\n");
	printf("\t-i\t\t삭제 시 'trash' 디렉토리로 삭제 파일과 정보를 이동시키지 않고 파일 삭제\n");
	printf("\t-r\t\t지정한 시간에 삭제 시 삭제 여부 재확인\n");
	printf("- RECOVER \"trash\" 디렉토리 안에 있는 파일을 원래 경로로 복구하는 명령어\n");
	printf("\tusage : RECOVER [FILENAME] [OPTION]\n");
	printf("\t-l\t\t'trash'디렉토리 밑에 있는 파일과 삭제 시간들을 삭제 시간이 오래된 순으로 출력 후, 명령어 진행\n");
	printf("- TREE \"check\" 디렉토리의 구조를 tree 형태로 보여주는 명령어\n");
	printf("\tusage : TREE\n");
	printf("- EXIT 프로그램 종료시키는 명령어\n");
	printf("\tusage : EXIT\n");
	printf("- HELP 명령어 사용법을 출력하는 명령어\n");
	printf("\tusage : HELP\n");

}

void doTreeCommand(){
	int i;
	int cnt;
	struct dirent **nextdirlist;
	printf("check");
	if ((cnt = scandir("check", &nextdirlist, NULL, NULL)) >= 2) {
		for(i = 0; i < 21 - strlen("check"); ++i) {
			printf("━");
		}
		printTree("check", 1);
	}
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

void checkFileDeleteTime(){
	time_t tmptime;
	time_t currentTime;
	char tmpPath[PATH_MAX];
	getcwd(tmpPath, PATH_MAX);
	chdir(exec_path);

	tmptime = time(NULL);
	currentTime = mktime(localtime(&tmptime));
	//printf("curtime : %ld\n", currentTime);
	while(1) {
		if (deleteFileHeap.size != 0 && deleteFileHeap.deleteFiles[0]->deleteTime < currentTime) {
			//printf("delete time : %ld, current time : %ld\n", deleteFileHeap.deleteFiles[0]->deleteTime, currentTime);
			deleteFile(popFromHeap());
		} else {
			break;
		}
	}
	alarm(60);
	
	chdir(tmpPath);
}

void deleteFile(struct DeleteFileInfo *deleteFileInfo){
	if (access(deleteFileInfo->absolutePathName, F_OK) != 0) {
		return;
	}


	if(deleteFileInfo->checkBeforeDelete) { // r옵션 수행
		char yesOrNo;
		printf("Delete [y/n]? ");
		scanf("%c", &yesOrNo);
		while(getchar()!='\n');
		deleteUsingRFlag = 1;

		if (yesOrNo != 'y') return;
	}

	if(deleteFileInfo->deleteImmediately) { // i옵션 수행
		if (remove(deleteFileInfo->absolutePathName) < 0) {
			fprintf(stderr, "remove error for %s\n", deleteFileInfo->absolutePathName);
			fprintf(stderr, "%s\n", strerror(errno));
		}
	} else { // i옵션 아닐 때 (trash 디렉토리 이용)
		struct stat statbuf;
		struct tm *curtm;
		struct tm *modtm;
		time_t curtime;
		char trashFileName[MAXNAMLEN];
		char trashFilePath[PATH_MAX];
		char infoFileName[MAXNAMLEN];
		char curTimeStr[BUFFER_SIZE];
		char modTimeStr[BUFFER_SIZE];
		char *relativePathName;
		int i;
		FILE *fp;

		for(i = strlen(deleteFileInfo->absolutePathName) - 1; i >= 0; --i) {
			if (deleteFileInfo->absolutePathName[i] == '/') {
				relativePathName = deleteFileInfo->absolutePathName + i + 1;
				break;
			}
		}	

		if (stat(deleteFileInfo->absolutePathName, &statbuf) < 0) {
			fprintf(stderr, "stat error for %s\n", deleteFileInfo->absolutePathName);
			return;
		}

		curtime = time(NULL);
		curtm = localtime(&curtime);
		sprintf(curTimeStr, "%d-%02d-%02d %02d:%02d:%02d", curtm->tm_year + 1900, curtm->tm_mon + 1, curtm->tm_mday, curtm->tm_hour, curtm->tm_min, curtm->tm_sec);

		modtm = localtime(&(statbuf.st_mtime));
		sprintf(modTimeStr, "%d-%02d-%02d %02d:%02d:%02d", modtm->tm_year + 1900, modtm->tm_mon + 1, modtm->tm_mday, modtm->tm_hour, modtm->tm_min, modtm->tm_sec);

		sprintf(trashFileName, "%s_%s", curTimeStr, relativePathName);
		sprintf(trashFilePath, "trash/files/%s", trashFileName);
		rename(deleteFileInfo->absolutePathName, trashFilePath);
		sprintf(infoFileName, "trash/info/%s", relativePathName);
		if (access(infoFileName, F_OK) == 0) {
			i = 1;
			do{
				sprintf(infoFileName, "trash/info/%s(%d)", relativePathName, i);
				++i;
			} while (access(infoFileName, F_OK) == 0);
		}

		if ((fp = fopen(infoFileName, "w")) < 0) {
			fprintf(stderr, "fopen error for %s\n", infoFileName);
			return;
		}

		fprintf(fp, "[Trash info]\n");
		fprintf(fp, "%s\n", deleteFileInfo->absolutePathName);
		fprintf(fp, "D : %s\n", curTimeStr);
		fprintf(fp, "M : %s\n", modTimeStr);

		fclose(fp);

		//info 디렉토리의 크기가 2kb가 넘으면 오래된 파일부터 삭제
		checkAndEraseTrashFiles();
	}

	free(deleteFileInfo);
}

void checkAndEraseTrashFiles(){
	struct stat statbuf;
	struct dirent **namelist;
	size_t infoFilesSize;
	int cnt;
	int i, j;
	chdir("trash/info");
	cnt = scandir(".", &namelist, NULL, NULL);
	infoFilesSize = 0;
	for (i = 0; i < cnt; ++i) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
		stat(namelist[i]->d_name, &statbuf);
		infoFilesSize += statbuf.st_size;
	}

	//printf("dirsize:%ld\n", infoFilesSize);
	if (infoFilesSize > 2048){
		while(infoFilesSize > 2048){
			struct stat dirstatbuf;
			time_t oldest = time(NULL) + 100000000;
			char oldestFileName[MAXNAMLEN];
			FILE *fp;
			char deleteTime[BUFFER_SIZE];
			char *relativeFileName;
			char absolutePathName[PATH_MAX];
			char trashFilePath[PATH_MAX];
			//cnt = scandir(".", &namelist, NULL, NULL);
			
			for (i = 0; i < cnt; ++i) {
				if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
				stat(namelist[i]->d_name, &dirstatbuf);
				if (dirstatbuf.st_mtime < oldest){
					oldest = dirstatbuf.st_mtime;
					strcpy(oldestFileName, namelist[i]->d_name);
				}

			}
			// 가장 오래된 파일 삭제
			if ((fp = fopen(oldestFileName, "r")) == NULL) {
				fprintf(stderr, "fopen error for %s, %s\n", oldestFileName, strerror(errno));
				chdir("..");
				chdir("..");
				return;
			}
			fgets(deleteTime, BUFFER_SIZE, fp); // 의미없이 첫줄 읽어옴
			fgets(absolutePathName, PATH_MAX, fp);
			fgets(deleteTime, BUFFER_SIZE, fp);
			deleteTime[strlen(deleteTime) - 1] = '\0';

			for(j = strlen(absolutePathName) - 1; j >= 0; --j) {
				if (absolutePathName[j] == '/') {
					relativeFileName = absolutePathName + j + 1;
					break;
				}
			}	
			relativeFileName[strlen(relativeFileName) - 1] = '\0';

			sprintf(trashFilePath, "../files/%s_%s", deleteTime + 4, relativeFileName);
			if (remove(trashFilePath) < 0) {
				fprintf(stderr, "remove error for %s, %s\n", trashFilePath, strerror(errno));
				chdir("..");
				chdir("..");
				return;
			}

			fclose(fp);
			if (remove(oldestFileName) < 0) {
				fprintf(stderr, "remove error for %s, %s\n", oldestFileName, strerror(errno));
				chdir("..");
				chdir("..");
				return;
			}

			free(namelist);

			cnt = scandir(".", &namelist, NULL, NULL);
			infoFilesSize = 0;
			for (i = 0; i < cnt; ++i) {
				if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;
				stat(namelist[i]->d_name, &statbuf);
				infoFilesSize += statbuf.st_size;
			}
		}
	}
	chdir("..");
	chdir("..");
}

void doRecoverCommand(){
	struct dirent **namelist;
	size_t infoFilesSize;
	int cnt;
	char *nextToken;
	char *filename;
	char realFileName[MAXNAMLEN];
	int l_option;
	int sameNameCnt = 0;
	int sameNameIndex[BUFFER_SIZE];
	int selectedIndex;
	int i;
	int printedFileCnt;
	FILE *fp;
	char deleteTime[BUFFER_SIZE];
	char modifyTime[BUFFER_SIZE];
	char originFilePath[PATH_MAX];
	char tmpOriginFilePath[PATH_MAX];
	char *deleteTimePtr;
	char *modifyTimePtr;
	char trashPathName[PATH_MAX];
	time_t mtime;
	struct tm mtm;
	struct utimbuf mutimbuf;
	struct infoFile infoFiles[BUFFER_SIZE];

	nextToken = strtok(NULL, " ");
	filename = nextToken;

	nextToken = strtok(NULL, " ");

	if (filename == NULL) return; // 파일명이 주어지지 않은경우
	else if (nextToken == NULL){ // 옵션 없이 파일명만 주어진 경우
		l_option = 0;
	} else if (!strcmp(nextToken, "-l")) { // -l 옵션이 주어진 경우
		l_option = 1;
	} else {
		return;
	}


	if (chdir("trash/info") < 0){
		return;
	}
	cnt = scandir(".", &namelist, NULL, NULL);
	sameNameCnt = 0;
	printedFileCnt = 0;
	for (i = 0; i < cnt; ++i) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")) continue;

		if (namelist[i]->d_name[strlen(namelist[i]->d_name) - 1] == ')') {
			strncpy(realFileName, namelist[i]->d_name, strlen(namelist[i]->d_name) - 3);
		} else {
			strcpy(realFileName, namelist[i]->d_name);
		}

		if (!strcmp(realFileName, filename)) {
			sameNameIndex[sameNameCnt] = i;
			++sameNameCnt;
		}
		if (l_option) { // 삭제 시간이 오래된 순으로 출력해야함(scandir의 정렬함수 이용)
			FILE *fp;
			char deleteTime[BUFFER_SIZE];
			char *deleteTimeForPrint;
			if ((fp = fopen(namelist[i]->d_name, "r")) == NULL) {
				fprintf(stderr, "fopen error for %s\n", namelist[i]->d_name);
				continue;
			}

			fgets(deleteTime, BUFFER_SIZE, fp); // 첫줄 의미없이 읽어들임
			fgets(deleteTime, BUFFER_SIZE, fp); // 둘째줄 의미없이 읽어들임
			fgets(deleteTime, BUFFER_SIZE, fp);
			deleteTime[strlen(deleteTime) - 1] = '\0';
			deleteTimeForPrint = deleteTime + 4;
			strcpy(infoFiles[printedFileCnt].filename, realFileName);
			strcpy(infoFiles[printedFileCnt].deleteTime, deleteTimeForPrint);

			++printedFileCnt;
			//printf("%d. %s \t%s\n", ++printedFileCnt, realFileName, deleteTimeForPrint);
		}
	}
	if(l_option) {
		//sort
		qsort(infoFiles, cnt - 2, sizeof(struct infoFile),  compareInfoFile);

		for(i = 0; i < cnt - 2; ++i) {
			printf("%d. %s \t%s\n", i + 1, infoFiles[i].filename, infoFiles[i].deleteTime);
		}
		printf("\n");
	}

	if (sameNameCnt == 0) {
		chdir("..");
		chdir("..");
		return;
	} else if (sameNameCnt == 1) {
		selectedIndex = 0;
	} else if (sameNameCnt > 1) {
		for (i = 0; i < sameNameCnt; ++i) {
			if ((fp = fopen(namelist[sameNameIndex[i]]->d_name, "r")) == NULL) {
				fprintf(stderr, "foopen error for %s\n", namelist[sameNameIndex[i]]->d_name);
				continue;
			}

			fgets(deleteTime, BUFFER_SIZE, fp); // 첫줄 의미없이 읽어들임
			fgets(deleteTime, BUFFER_SIZE, fp); // 둘째줄 의미없이 읽어들임
			fgets(deleteTime, BUFFER_SIZE, fp);
			fgets(modifyTime, BUFFER_SIZE, fp);
			deleteTime[strlen(deleteTime) - 1] = '\0';
			modifyTime[strlen(modifyTime) - 1] = '\0';
			printf("%d. %s %s %s\n", i + 1, filename, deleteTime, modifyTime);

			fclose(fp);
		}
		printf("Choose : ");
		scanf("%d", &selectedIndex);
		while(getchar()!='\n');
		--selectedIndex;
		if(selectedIndex < 0 || sameNameCnt <= selectedIndex) {
			chdir("..");
			chdir("..");
			return;
		}
	} else {
		chdir("..");
		chdir("..");
		return;
	}

	if ((fp = fopen(namelist[sameNameIndex[selectedIndex]]->d_name, "r")) == NULL) {
		fprintf(stderr, "foopen error for %s\n", namelist[sameNameIndex[i]]->d_name);
		chdir("..");
		chdir("..");
		return;
	}

	fgets(deleteTime, BUFFER_SIZE, fp); // 첫줄 의미없이 읽어들임
	fgets(originFilePath, PATH_MAX, fp);
	fgets(deleteTime, BUFFER_SIZE, fp);
	fgets(modifyTime, BUFFER_SIZE, fp);
	originFilePath[strlen(originFilePath) - 1] = '\0';
	deleteTime[strlen(deleteTime) - 1] = '\0';
	modifyTime[strlen(deleteTime) - 1] = '\0';
	deleteTimePtr = deleteTime + 4;
	modifyTimePtr = modifyTime + 4;
	sprintf(trashPathName, "../files/%s_%s", deleteTimePtr, filename);
	// 선택된거 복원 ㄱ
	//rename
	//파일 복원시 중복되는 문제...
	if (access(originFilePath, F_OK) == 0) { // 파일명 중복되는 경우
		strcpy(tmpOriginFilePath, originFilePath);
		for (i = strlen(tmpOriginFilePath) - 1; i >= 0; --i) {
			if (tmpOriginFilePath[i] == '/') {
				tmpOriginFilePath[i] = 0;
				break;
			}
		}

		i = 1;
		do {
			sprintf(originFilePath, "%s/%d_%s", tmpOriginFilePath, i, filename);
			++i;
		} while (access(originFilePath, F_OK) == 0);
	}
	rename(trashPathName, originFilePath);
	//mtime 수정
	mtime;
	mtm.tm_year = atoi(strtok(modifyTimePtr, "-")) - 1900;
	mtm.tm_mon = atoi(strtok(NULL, "-")) - 1;
	mtm.tm_mday = atoi(strtok(NULL, " "));
	
	mtm.tm_hour = atoi(strtok(NULL, ":"));
	mtm.tm_min = atoi(strtok(NULL, ":"));
	mtm.tm_sec = atoi(strtok(NULL, " "));

	//printf("recover modtime : %d-%d-%d %d:%d\n", mtm.tm_year, mtm.tm_mon, mtm.tm_mday, mtm.tm_hour, mtm.tm_min);
	mtime = mktime(&mtm); // 수정 시간을 time_t로 변환해 저장
	mutimbuf.actime = mtime;
	mutimbuf.modtime = mtime;
	utime(originFilePath, &mutimbuf);
	
	//info 파일 삭제
	remove(namelist[sameNameIndex[selectedIndex]]->d_name);

	chdir("..");
	chdir("..");
	return;
}

void alarmHandlercheckFileDeleteTime(){
	checkFileDeleteTime();
	if(deleteUsingRFlag) {
		printf("20160548>");
		deleteUsingRFlag = 0;
	}
}

