#ifndef HW3_131044009
#define HW3_131044009

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* pid_t */
#include <fcntl.h> /* open close */
#include <string.h> /* strerror */
#include <errno.h> /* errno */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> /* DIR *, struct dirent * */
#include <wait.h>

#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define READ_FLAGS (O_RDONLY)
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define FD_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FAIL -1
#define SUCCESS 0
#define CHILD_PROCESS 0
#define FILE_NAME_MAX 20
#define COORDINAT_TEXT_MAX 30
#define BLKSIZE 512
#define FILE_PROC_DEAD 1
#define DIR_PROC_DEAD 0


/* Yeni Sayisal Tip*/
typedef enum{
  FALSE=0,TRUE=1
}bool;

typedef struct{
  pid_t pid;
  int fd[2];
  int id;
}proc_t;

/*Maksimum dosya yolu uzunlugu*/
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

/* Standart log dosya ismi*/
#define DEF_LOG_FILE_NAME "gfd.log"

/*
* Bu method verilen yol icindeki tum dosya ve klasorlerde istenilen
* kelimeyi recursive arar. Kendini her klasor ve dosya gordugunde forklayarak
* daha kisa surede arama yapar. Toplam bulunan kelime sayisini return edip log
* dosyasi olusturur. Ana log dosyasinin ayarlanmasi mainda halldilecektir.
* @param dirPart : klasor adi
* @param word : aranacak kelime
* @return toplam bulunan kelime sayisini
*/
int searchDirRec(const char *dirPath,const char *word,int fd);


int searchDir(const char *dirPath,const char *word);

/* BU FONKSIYON DERS KITABINDAN ALINMISTIR
* Bu fonksiyon parametre olarak gelen stringin bir klasor olup olmadigina bakar.
* @param dirName : klasor adi
* @return klasor ise TRUE diger durumlarda false return eder.
*/
bool isDirectory(const char *dirName);

/*
* Bu fonsiyon verilen path icindeki gecici log dosyalarini birlestirir.
* Gecici dosya isimleri childpid lerden olusur.
* Yeni log dosyasi path dizini icinde pid numarasi ile olusturulur.
* @param path : log dosyalarinin aranip birlestirilecegi dizin
* @param fileName : gecici log dosyalarinin adi
* @param logName : yeni, ortak log dosyasinin adi
*/
void addLog(const char* path,const char* fileName,const char* logName);


/*
* Bu fonksiyon verilen dosya isminden yola cikara txt olup olmadigini kontrol
* eder. TXT uzantili olmasi durumanda TRUE, diger durumlarda FALSE return eder.
* @param fileName : dosya adi
* @return txt ise true diger durumlarda false return eder.
*/
bool isRegularFile(const char *fileName);

/*
* Bu method parametre olarak gelen dosyayi acar ve icinde diger parametre olan
* wordu arar. Her buldugunu log dosyasina kaydeder. Dosya uzerinde lseek ile
* gezme yapildi. Memorye alinan birsey yok.
* @param fileName : arama yapiacak dosya adi
* @para word : aranacak kelime
* @return dosyadaki toplam eslesen kelime sayisi
*/
int findOccurencesInFile(int fd,const char* fileName,const char *word);


/*
* Bu method yardimci olarak kullanilicak.
* kendisine gelen pid numarasini string olarak return eder.
* @return string olarak numara  -> free edilmesi lazım
* @param number : pid numarasi
*/
char *getStringOfNumber(long number);

void freePtr(proc_t *proc,int size);

void findContentNumbers(DIR* pdir,const char *dirPath,int *fileNumber,int *dirNumber);

ssize_t r_read(int fd, void *buf, size_t size);
ssize_t r_write(int fd, void *buf, size_t size);
int copyfile(int fromfd, int tofd);
int readwrite(int fromfd, int tofd);



#endif
