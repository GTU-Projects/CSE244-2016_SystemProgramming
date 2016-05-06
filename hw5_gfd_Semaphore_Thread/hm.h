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
	pid_t tid;
	const char *strFilePath; // hangi dosyadan okuyacak
	const char *word; // ne okuyacak
}hmThread_t;



void freeAll();


int findOccurance(const char *dirname,const char *word);

int findRec(const char *dirPath,const char *word,int fd);

void *threadFindOcc(void *arg);

void *threadRemovePipe(void *arg);

void *threadRemoveFifo(void *arg);

int findContentOfDir(const char *dirPath);

int findOccurenceInRegular(int fd,const char* fileName,const char *word);

#endif