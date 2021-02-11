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

struct FileInfo { // 파일 정보를 담는 구조체
	char absolutePathName[PATH_MAX]; // 파일의 절대경로명
	time_t lastModifyTime; // 파일의 최종 수정시간
};

struct FileData { // 파일 정보들의 배열 관리 구조체
	struct FileInfo files[MAX_FILE_CNT]; // 파일 정보 배열
	int filesCnt; // 배열의 유효한 원소의 개수
};

void checkFileDataStatus(const struct FileData *prevFileData, const struct FileData *curFileData); // 이전 파일들 목록과 갱신된 파일들 목록을 확인하여 수정, 추가, 삭제된 파일이 있는지 확인하는 함수
void updateFileDataStatus(struct FileData **prevFileData, struct FileData **curFileData); // 파일 목록을 갱신하는 함수
void getCurFileState(struct FileData *fileData, const char *dirname); // 파일들 상태를 가져오는 재귀호출 함수
void printFileData(const struct FileData *fileData); // 파일 데이터를 출력하는 함수, 디버그용
void sortFileData(struct FileData *fileData); // 파일 데이터를 파일명 기준으로 정렬하는 함수
int compareFileInfo(const void *a, const void *b); // 파일 데이터를 정렬할 때 사용하는 비교함수
int ssu_daemon_init(const char *path); // 디몬 프로세스 실행 함수
const char *getRelativePath(const char *absolutePath); // 절대경로에서 상대경로 구하는 함수

void startMntr(void) // 모니터링 디몬 프로세스를 실행하는 함수. 이 함수는 ssu_mntr.c의 메인함수에서 호출함
{
	pid_t pid;
	if ((pid = fork()) == 0) { // 자식 프로세스 생성, 자식 프로세스라면
		char path[PATH_MAX];
		getcwd(path, PATH_MAX); // 현재 경로 구한다
		ssu_daemon_init(path); // 디몬 프로세스 실행
	} else if (pid < 0) { // fork 에러라면
		fprintf(stderr, "mntr starting error\n");
		exit(1);
	} else { // 부모 프로세스라면
		return;
	}
}

void checkFileDataStatus(const struct FileData *prevFileData, const struct FileData *curFileData) {// 이전 파일들 목록과 갱신된 파일들 목록을 확인하여 수정, 추가, 삭제된 파일이 있는지 확인하는 함수
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

	// 이진 탐색으로 현재 파일 목록의 파일과 과거 파일 목록의 데이터를 비교한다
	for (i = 0; i < curFileData->filesCnt; ++i) {
		int fileNameCmpResult;
		int start, end, mid;
		int sameNameFindFlag = 0;
		// 이진 탐색으로 해당 파일이 이전 파일 목록에도 존재하는지 확인한다
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

		if (sameNameFindFlag) { // 이름이 일치하는 파일을 찾았다면
			if (prevFileData->files[mid].lastModifyTime == curFileData->files[i].lastModifyTime) { // 파일 수정되지 않은 경우
				;
			} else { // 파일 수정된 경우
				// 파일 수정 메세지 출력
				fprintf(logfp, "[%s][modify_%s]\n", timebuf, getRelativePath(curFileData->files[i].absolutePathName));
				//printf("%s is modified\n", curFileData->files[i].absolutePathName);
			}
		} else { // 파일 새로 추가된 경우
			// 파일 추가 메세지 출력
			fprintf(logfp, "[%s][create_%s]\n", timebuf, getRelativePath(curFileData->files[i].absolutePathName));
			//printf("%s is created\n", curFileData->files[i].absolutePathName);
		}
	}

	// 이진 탐색으로 과거 파일 목록의 파일과 현재 파일 목록의 데이터를 비교한다
	for (i = 0; i < prevFileData->filesCnt; ++i) {
		int start, end, mid;
		int fileNameCmpResult;
		int sameNameFindFlag = 0;
		// 이진 탐색으로 해당 파일이 현재 파일 목록에도 존재하는지 확인한다
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
			// 파일 삭제 메세지 출력
			fprintf(logfp, "[%s][delete_%s]\n", timebuf, getRelativePath(prevFileData->files[i].absolutePathName));
			//printf("%s is deleted\n", prevFileData->files[i].absolutePathName);
		}
	}

	fclose(logfp);
}

void updateFileDataStatus(struct FileData **prevFileData, struct FileData **curFileData) { // 파일 목록을 갱신하는 함수
	char path[PATH_MAX];
	struct FileData *tmp;
	getcwd(path, PATH_MAX);

	// 새롭게 데이터를 갱신해야 하므로 curFileData를 prevFileData로 옮김, prevFileData는 새로운 파일 정보를 넣어 재활용
	tmp = *prevFileData;
	*prevFileData = *curFileData;
	*curFileData = tmp;
	
	// 새로운 파일 데이터 생성(getCurFileState 호출) 전에 반드시 filesCnt를 0으로 만들어 줘야 한다
	(*curFileData)->filesCnt = 0; 
	getCurFileState(*curFileData, TARGET_DIRECTORY_NAME);
	sortFileData(*curFileData); // 새로 가져온 파일 목록을 정렬한다

	chdir(path);

	return;
}

void getCurFileState(struct FileData *fileData, const char *dirname){ // 파일들 상태를 가져오는 재귀호출 함수
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

	while((dentry = readdir(dir)) != NULL) { // 모든 파일들 확인
		if (dentry->d_ino == 0) continue;

		memcpy(filename, dentry->d_name, MAXNAMLEN);
		if (!strcmp(filename, ".") || !strcmp(filename, "..")) continue; // 확인할 필요 없는 파일들 패스

		realpath(filename, absolutePathName); // 파일의 상대경로 가져옴

		// 해당 파일의 stat 구조체 가져온다
		if (stat(absolutePathName, &statbuf) == -1) {
			fprintf(stderr, "stat error for %s\n", absolutePathName);
			printf("ERROR:%s\n", strerror(errno));
			exit(1);
		}

		if (S_ISDIR(statbuf.st_mode)) { // 해당 파일이 디렉토리 파일이라면
			// 재귀호출
			getCurFileState(fileData, absolutePathName);
			// 작업 후 원래 디렉토리로 복귀하기
			chdir("..");
		} 
		// 해당 파일의 정보를 파일 목록에 추가한다
		strcpy(fileData->files[fileData->filesCnt].absolutePathName, absolutePathName);
		fileData->files[fileData->filesCnt].lastModifyTime = statbuf.st_mtime;
		++(fileData->filesCnt);
	}

	if (closedir(dir) < 0) {
		fprintf(stderr, "closedir error\n");
		exit(1);
	}
}

void printFileData(const struct FileData *fileData) { // 파일 목록 출력하는 함수 - 디버그용
	int i;
	printf("print file data start\n");
	// 파일 목록 전체 순회하며 출력
	for (i = 0; i < fileData->filesCnt; ++i) { 
		printf("name : %s, st_mtime : %ld\n", fileData->files[i].absolutePathName, fileData->files[i].lastModifyTime);
	}
	printf("print file data end\n");
	printf("\n");
}

void sortFileData(struct FileData *fileData){ // 파일 목록 파일명 기준으로 정렬하는 함수
	qsort(fileData->files, fileData->filesCnt, sizeof(struct FileInfo), compareFileInfo);
}

int compareFileInfo(const void *a, const void *b) { // 파일 목록 정렬하는데 사용하는 비교함수
	return strcmp(((const struct FileInfo *)a)->absolutePathName, ((const struct FileInfo *)b)->absolutePathName);
}

int ssu_daemon_init(const char *path) { // 디몬 프로세스 시작하는 함수
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

	prevFileData = (struct FileData *) malloc(sizeof(struct FileData)); // 이전 파일 목록 저장할 구조체 동적할당
	curFileData = (struct FileData *) malloc(sizeof(struct FileData)); // 현재 파일 목록 저장할 구조체 동적할당
	prevFileData->filesCnt = 0; // 이전 파일 목록 구조체 초기화
	curFileData->filesCnt = 0; // 현재 파일 목록 구조체 초기화

	updateFileDataStatus(&prevFileData, &curFileData); // 파일 목록 갱신
	while(1) {
		updateFileDataStatus(&prevFileData, &curFileData); // 파일 목록 갱신
		checkFileDataStatus(prevFileData, curFileData); // 변경된 파일 있는지 확인
		sleep(1); // 1초 sleep
	}

	return 0;
}

const char *getRelativePath(const char *absolutePath) { // 절대경로에서 상대경로를 구해오는 함수
	int i;
	for (i = strlen(absolutePath) - 1; i >= 0; --i) { // 절대경로 문자열의 맨 뒤부터 앞으로 오면서 확인
		if (absolutePath[i] == '/') break; // '/' 문자열을 만났다면 반복 중지
	}

	return absolutePath + i + 1; // '/' 문자열의 바로 뒤 문자의 포인터 리턴
}
