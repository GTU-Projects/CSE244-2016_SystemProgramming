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
#define FIFO_NAME ".fifo.ff"
#define SEM_NAME "/hm.sem"
#define MESSAGE_MAX 100


#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

//#define DEBUG
//#define DEBUG_FILE_READ

typedef struct{
	pthread_t th;
	pid_t tid;
	const char *strFilePath; // hangi dosyadan okuyacak
	const char *word; // ne okuyacak
}hmThread_t;

/*
** Bu fonksiyon sinyal geldiginde flag degerini 1 yapacak.
*/
void sighandler(int signo);

/*
** Bu fonksiyon eger memoryden alinmis yerler varsa onlari geri vericel
*/
void freeAll();

/*
** Wrapper olarak kullanilacak.
** Bu fonksiyon verilen dir altinda recursive olarak kelime ariacak
** NOT ::: BIR TANE THREAD SUREKLI FIFODAN OKUMA YAPACAK.
** HER GELEN BILGIYI LOGA AKTARARAK DAHA IYI BIR PERFORMANS SAGLANDI
*/
int findOccurance(const char *dirname,const char *word);

/*
** Bu fonksiyon findOccuranceye yardimci olarak recursive kollarda kullanilacak
** NOT :: BURADADA BIR TANE THREAD SUREKLI PIPE UCUNU DINLICEK VE GELENLERI 
** FIFOYA YONLENDIRECEK BOYLECE HEM ALAN HEMDE ZAMAN KAZANIRIZ.
*/
int findRec(const char *dirPath,const char *word);

void *threadFindOcc(void *arg);

/*
** Bu fonksiyonu remove pipe threadi kullanacak
** Surekli olarak pipetan bilgileri cekip fifoya yonlendirecek
**
** Pipe tan neler gelicek :
** 
**	tid row col // tid si ve ilk koordinatlar
**      row col // 2.koordinatlar
**      -1 total  // tid pipe yazmayi kesi yeni yid bekle
**   -1  . .  // tid -1 geldi yani pipe bilgi akisini kapat
** 
** Fifoya nasi ahtaracak : 
** 
**	path_size path row col // tid ye ait pathi bul ve yolla
**      			row col
**  				-1  total // row -1 olana kadar oku -1 ise farklı pathtan bilgi gelicek demektir
**   -1   // path uzunlugu -1 ise o zaman bu threadin isi bitmistir olsun
*/
void *threadRemovePipe(void *arg);



/*
** Bu fonksiyonu remove fifo threadi kullanacak
** Surekli olarak fifodan bilgileri cekip loga  yonlendirecek
** 
** Fifodan okuaycaklarıi : 
**	path_size path row col // path size al gecerli ise oku
**      			row col
**  				-1  total // row -1 olana kadar oku -1 ise farklı pathtan bilgi gelicek demekti
**   -1   // path uzunlugu -1 ise o zaman bu threadin isi bitmistir olsun
*/
void *threadRemoveFifo(void *arg);

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
int findOccurenceInRegular(int fd,const char* fileName,const char *word);

/* BU FONKSIYONLAR KITAPTAN ALINDI */

// named semafor acar
int getnamed(char *name, sem_t **sem, int val);

// named semaforu kapatır
int destroynamed(char *name, sem_t *sem);



#endif