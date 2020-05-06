#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
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

int main(void)
{
	char path[PATH_MAX];
	struct FileData *prevFileData;
	struct FileData *curFileData;
	prevFileData = (struct FileData *) malloc(sizeof(struct FileData));
	curFileData = (struct FileData *) malloc(sizeof(struct FileData));
	prevFileData->filesCnt = 0;
	curFileData->filesCnt = 0;

//	getcwd(path, PATH_MAX);
//
//	curFileData->filesCnt = 0; // 새로운 파일 데이터 생성(getCurFileState 호출) 전에 반드시 filesCnt를 0으로 만들어 줘야 한다
//	getCurFileState(curFileData, TARGET_DIRECTORY_NAME);
//	chdir(path);
//	// 이진 검색을 위해 생성된 file 데이터 sort
//
//	sortFileData(curFileData);
//

	while(1) {
		printf("-----------------------------------------\n");
		updateFileDataStatus(&prevFileData, &curFileData);
		checkFileDataStatus(prevFileData, curFileData);
		printf("cur\n");
		printFileData(curFileData);
		printf("prev\n");
		printFileData(prevFileData);
		sleep(10);
	}

}

void checkFileDataStatus(const struct FileData *prevFileData, const struct FileData *curFileData) {
	int i, j;

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
				printf("%s is modified\n", curFileData->files[i].absolutePathName);
			}
		} else { // 파일 새로 추가된 경우
			printf("%s is created\n", curFileData->files[i].absolutePathName);
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
			printf("%s is deleted\n", prevFileData->files[i].absolutePathName);
		}
	}

	///////////////////////
//	for (i = 0; i < curFileData->filesCnt; ++i) {
//		int sameNameFindFlag = 0;
//		for (j = 0; j < prevFileData->filesCnt; ++j) {
//			int fileNameCmpResult = strcmp(prevFileData->files[i].absolutePathName, curFileData->files[j].absoluthFilePath);
//			if (fileNameCmpResult) {
//				sameNameFindFlag = 1;
//				break;
//			}
//		}
//
//		if (sameNameFindFlag) {
//			if (prevFileData->files[i].lastModifyTime == curFileData->files[i].lastModifyTime) { // 파일 수정되지 않은 경우
//				;
//			} else { // 파일 수정된 경우
//				// 파일 수정 메세지 출력
//			}
//		} else {
//
//		}
//	}
//	
//
//
//
//
//
//
//
//
//
//	for (i = 0, j = 0; i < prevFileData->filesCnt && j < curFileData->filesCnt; ) {
//		int fileNameCmpResult = strcmp(prevFileData->files[i].absolutePathName, curFileData->files[j].absoluthFilePath);
//		if (fileNameCmpResult == 0) {
//			// 파일 수정 시간 비교
//
//			if (prevFileData->files[i].lastModifyTime == curFileData->files[i].lastModifyTime) { // 파일 수정되지 않은 경우
//				;
//			} else { // 파일 수정된 경우
//				// 파일 수정 메세지 출력
//			}
//			++i;
//			++j;
//			continue;
//		} else if (fileNameCmpReuslt > 0) {
//
//		} else {
//
//		}
//
//
//	}


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
