#ifndef HM
 #define HM 0


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>

#define LOG_FILE_NAME "gfd.log"
#define FIFO_NAME "fifo.ff"

#define DEBUG

typedef struct{
	pthread_t th;
	char *filePath;
	char *word;
}hmthread_t;





int findOccurance(const char *dirname,const char *word);

int findRec(const char *dirPath,const char *word,int fd);

void threadFindOcc(void *arg);

void threadRemovePipe(void *arg);


DIR* findContentOfDir(const char *dirPath,int *pFileNum, int *pDirNum);

#endif