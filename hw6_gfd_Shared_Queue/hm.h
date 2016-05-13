#ifndef HM
 #define HM 0


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>

#define LOG_FILE_NAME "gfd.log"
#define FIFO_NAME ".fifo.ff"
#define SEM_NAME "/hm.sem"
#define SEM_SHARED_NAME "/hm.shmsem"

#define MESSAGE_SIZE 20

#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

//#define DEBUG
//#define DEBUG_FILE_READ

typedef struct{
	pthread_t th;
	const char *strFilePath; // hangi dosyadan okuyacak
	const char *word; // ne okuyacak
}hmThread_t;

typedef struct{
	long type;
	char text[MESSAGE_SIZE];
}hmMsg_t;

/*
** Bu fonksiyon sinyal geldiginde flag degerini 1 yapacak.
*/
void sighandler(int signo);

/*
** Bu fonksiyon eger memoryden alinmis yerler varsa onlari geri vericel
*/
void freeAll();


long getTimeDif(struct timeval start, struct timeval end);


// starter method
int startSearching(char *dirname,char *word);


/*
** Bu fonksiyon findOccuranceye yardimci olarak recursive kollarda kullanilacak
** NOT :: BURADADA BIR TANE THREAD SUREKLI PIPE UCUNU DINLICEK VE GELENLERI 
** FIFOYA YONLENDIRECEK BOYLECE HEM ALAN HEMDE ZAMAN KAZANIRIZ.
*/
int findRec(const char *dirPath,const char *word);



void *threadFindOcc(void *arg);



/*
** Bu fonksiyon bir dir icindeki toplam klasor ve dosya sayisini
** ve bunlarin path aderslerini global dinamic yerlere kaydeder
** Return : klasor + dosya sayisi
*/
int findContentOfDir(const char *dirPath);

/*
** Bu fonksiyon dosya icinde kelimeleri ariyacak ve buldugu her koordinati
** kendine gelen fildes uzerinden yazacak
*/
occurance_t * findOccurenceInRegular(const char* fileName,const char *word);

/* BU FONKSIYON KITAPTAN ALINDI */
// named semafor acar
int getnamed(char *name, sem_t **sem, int val);


#endif