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
#define SEM_LOG_NAME "/hm.sem" // log dosyasini kilitlemek icin
#define SEM_SHARED_NAME "/hm.shmsem" // shared memory kilitlemek icin

#define MESSAGE_SIZE 12 // max integer boyutu kadar 11+1

// dosya,semafor izinleri
#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

//#define DEBUG
//#define DEBUG_FILE_READ

// Bu struct thread icin kullanilacak
typedef struct{
	pthread_t th;
	const char *strFilePath; // hangi dosyadan okuyacak
	const char *word; // ne okuyacak
}hmThread_t;

// bu struct message queue icin kullanilacak
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


/*
** Iki zaman arasindaki farki milisaniye(ms) cinsinden bulur
*/
long getTimeDif(struct timeval start, struct timeval end);


/* Ana arama islemini, yapiyi kontrol edecek bir starter fonksiyon.
** verilen klasor altinda tum okunabilir dosya ve klasorlerde kelime arar.
** toplam tekrar sayisini return eder.
*/
int startSearching(char *dirname,char *word);


/*
** Bu fonksiyon verilen klasor altindaki dosyalarda recursive olarak kelime ariyacak
** her klasor icin fork her dosya cin threadler araciligiyla okuma yapilacak
** threadler file proceslere bilgiler(toplam kac tane bulduklarini) message queue ile
** fork ile olusan procesler ise shared memorye toplam sonuclarini eklicekler 
*/
int findRec(const char *dirPath,const char *word);



/*
** Bu methodu dosyalar icin arama yapacak olan threadler kullanacaklar
** Genel olarak starter yapili bir metoddur. findOccuranceInRegulari 
** cagirir.
*/
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