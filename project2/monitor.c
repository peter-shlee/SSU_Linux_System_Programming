#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILE_CNT 1024

#define TARGET_DIRECTORY_NAME "check"

struct FileInfo {
	char absolutePathName[PATH_MAX];
	time_t lastModifyTime;
};

struct FileData {
	struct FileInfo files[MAX_FILE_CNT];
	int filesCnt;
};

void checkFileDataStatus(const struct FileData *prevFileData, const struct FileData *curFileData);
void updateFileDataStatus(struct FileData **prevFileData, struct FileData **curFileData);
void getCurFileState(struct FileData *fileData, const char *dirname);
void printFileData(const struct FileData *fileData);
void sortFileData(struct FileData *fileData);
int compareFileInfo(const void *a, const void *b);
int ssu_daemon_init(const char *path);
const char *getRelativePath(const char *absolutePath);

void startMntr(void)
{
	pid_t pid;
	if ((pid = fork()) == 0) {
		char path[PATH_MAX];
		struct FileData *prevFileData;
		struct FileData *curFileData;
		prevFileData = (struct FileData *) malloc(sizeof(struct FileData));
		curFileData = (struct FileData *) malloc(sizeof(struct FileData));
		prevFileData->filesCnt = 0;
		curFileData->filesCnt = 0;

		getcwd(path, PATH_MAX);
		ssu_daemon_init(path);
	} else if (pid < 0) {
		fprintf(stderr, "mntr starting error\n");
		exit(1);
	} else {
		return;
	}
}

void checkFileDataStatus(const struct FileData *prevFileData, const struct FileData *curFileData) {
	int i, j;
	FILE *logfp;
	time_t curtime;
	struct tm *tmpointer;
	char timebuf[64];
	time(&curtime);
	tmpointer = localtime(&curtime);
	sprintf(timebuf, "%d-%02d-%02d %02d:%02d:%02d", 1900 + tmpointer->tm_year, tmpointer->tm_mon + 1, tmpointer->tm_mday, tmpointer->tm_hour, tmpointer->tm_min, tmpointer->tm_sec);

	if ((logfp = fopen("log.txt", "a")) == NULL) {
		fprintf(stderr, "fopen error for log.txt\n");
		exit(1);
	}

	// 이진탐색 구현

	for (i = 0; i < curFileData->filesCnt; ++i) {
		int fileNameCmpResult;
		int start, end, mid;
		int sameNameFindFlag = 0;
		start = 0;
		end = prevFileData->filesCnt - 1;
		while(start <= end) {
			mid = (start + end) / 2;
			fileNameCmpResult = strcmp(prevFileData->files[mid].absolutePathName, curFileData->files[i].absolutePathName);
			if (fileNameCmpResult == 0) {
				sameNameFindFlag = 1;
				break;
			} else if (fileNameCmpResult > 0) {
				end = mid - 1;
			} else {
				start = mid + 1;
			}
		}

		if (sameNameFindFlag) {
			if (prevFileData->files[mid].lastModifyTime == curFileData->files[i].lastModifyTime) { // 파일 수정되지 않은 경우
				;
			} else { // 파일 수정된 경우
				// 파일 수정 메세지 출력
				fprintf(logfp, "[%s][modify_%s]\n", timebuf, getRelativePath(curFileData->files[i].absolutePathName));
				//printf("%s is modified\n", curFileData->files[i].absolutePathName);
			}
		} else { // 파일 새로 추가된 경우
			fprintf(logfp, "[%s][create_%s]\n", timebuf, getRelativePath(curFileData->files[i].absolutePathName));
			//printf("%s is created\n", curFileData->files[i].absolutePathName);
		}
	}

	for (i = 0; i < prevFileData->filesCnt; ++i) {
		int start, end, mid;
		int fileNameCmpResult;
		int sameNameFindFlag = 0;
		start = 0;
		end = curFileData->filesCnt - 1;
		mid = (start + end) / 2;
		while(start <= end) {
			mid = (start + end) / 2;
			fileNameCmpResult = strcmp(curFileData->files[mid].absolutePathName, prevFileData->files[i].absolutePathName);

			if (fileNameCmpResult == 0) {
				sameNameFindFlag = 1;
				break;
			} else if (fileNameCmpResult > 0) {
				end = mid - 1;
			} else {
				start = mid + 1;
			}
		}

		if (sameNameFindFlag) { // 위에서 이미 검사 함
			continue;
		} else { // 파일 삭제된 경우
			fprintf(logfp, "[%s][delete_%s]\n", timebuf, getRelativePath(prevFileData->files[i].absolutePathName));
			//printf("%s is deleted\n", prevFileData->files[i].absolutePathName);
		}
	}

	fclose(logfp);
}

void updateFileDataStatus(struct FileData **prevFileData, struct FileData **curFileData) {
	char path[PATH_MAX];
	struct FileData *tmp;
	getcwd(path, PATH_MAX);

	// 새롭게 데이터를 갱신해야 하므로 curFileData를 prevFileData로 옮김, prevFileData는 새로운 파일 정보를 넣어 재활용
	tmp = *prevFileData;
	*prevFileData = *curFileData;
	*curFileData = tmp;

	(*curFileData)->filesCnt = 0; // 새로운 파일 데이터 생성(getCurFileState 호출) 전에 반드시 filesCnt를 0으로 만들어 줘야 한다
	getCurFileState(*curFileData, TARGET_DIRECTORY_NAME);
	sortFileData(*curFileData);

	chdir(path);

	return;
}

void getCurFileState(struct FileData *fileData, const char *dirname){
	char filename[MAXNAMLEN];
	char absolutePathName[PATH_MAX];
	struct dirent *dentry;
	struct stat statbuf;
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

		realpath(filename, absolutePathName);

		if (stat(absolutePathName, &statbuf) == -1) {
			fprintf(stderr, "stat error for %s\n", absolutePathName);
			printf("ERROR:%s\n", strerror(errno));
			exit(1);
		}

		if (S_ISDIR(statbuf.st_mode)) {
			// 재귀호출
			getCurFileState(fileData, absolutePathName);
			// 작업 후 원래 디렉토리로 복귀하기
			chdir("..");
		} 
		strcpy(fileData->files[fileData->filesCnt].absolutePathName, absolutePathName);
		fileData->files[fileData->filesCnt].lastModifyTime = statbuf.st_mtime;
		++(fileData->filesCnt);
	}

	if (closedir(dir) < 0) {
		fprintf(stderr, "closedir error\n");
		exit(1);
	}
}

void printFileData(const struct FileData *fileData) {
	int i;
	printf("print file data start\n");
	for (i = 0; i < fileData->filesCnt; ++i) {
		printf("name : %s, st_mtime : %ld\n", fileData->files[i].absolutePathName, fileData->files[i].lastModifyTime);
	}
	printf("print file data end\n");
	printf("\n");
}

void sortFileData(struct FileData *fileData){
	qsort(fileData->files, fileData->filesCnt, sizeof(struct FileInfo), compareFileInfo);
}

int compareFileInfo(const void *a, const void *b) {
	return strcmp(((const struct FileInfo *)a)->absolutePathName, ((const struct FileInfo *)b)->absolutePathName);
}

int ssu_daemon_init(const char *path) {
	pid_t pid;
	int fd, maxfd;

	struct FileData *prevFileData;
	struct FileData *curFileData;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid != 0)
		exit(0);

	pid = getpid();
	//printf("process %d running as daemon\n", pid);
	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for (fd = 0; fd < maxfd; fd++)
		close(fd);

	umask(0);
	chdir(path);
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	prevFileData = (struct FileData *) malloc(sizeof(struct FileData));
	curFileData = (struct FileData *) malloc(sizeof(struct FileData));
	prevFileData->filesCnt = 0;
	curFileData->filesCnt = 0;

	updateFileDataStatus(&prevFileData, &curFileData);
	while(1) {
		updateFileDataStatus(&prevFileData, &curFileData);
		checkFileDataStatus(prevFileData, curFileData);
		sleep(1);
	}

	return 0;
}

const char *getRelativePath(const char *absolutePath) {
	int i;
	for (i = strlen(absolutePath) - 1; i >= 0; --i) {
		if (absolutePath[i] == '/') break;
	}

	return absolutePath + i + 1;
}
