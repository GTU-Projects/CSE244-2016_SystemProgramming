#ifndef HW3_131044009
#define HW3_131044009

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

/* Her process icin tutulacak bilgiler*/
typedef struct{
  int place; /* processin indisi*/
  pid_t pid; /* process pid numarasi */
  int fd[2]; /* processin erisebilecegi fd ler */
}proc_t;

/*Maksimum dosya yolu uzunlugu*/
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

/* Standart log dosya ismi*/
#define DEF_LOG_FILE_NAME "gfd.log"

/*
* Bu fonksiyon parametre olarak verilen dosyanin icindeki klasor ve regular file
* sayisini output paramtre olarak return eder.
* @paran pDir : arama yapilacak yer
* @paran dirPath : aramanın local dizin adresi
* @param fileNumber : buluan regular dosya sayisi
* @param directoryNumber : bulunan klasor sayisi
*/
void findContentNumbers(DIR* pdir,const char *dirPath,int *fileNumber,int *dirNumber);


/*
  Bu fonksiyon verilan array icinde pid arar ve buldugu konumu return eder.
  @param arr : process arrayi
  @param size : array boyutu
  @param pid : aranacak pid_t
  @return : Bulunursa konumu, diger durumlarda FAIL(-1) return eder
*/
int getID(proc_t *arr,int size,pid_t pid);

/*
* Bu foksiyon verilen yol icindeki tum dosya ve klasorlerde istenilen
* kelimeyi recursive arar. Kendini her klasor ve dosya gordugunde forklayarak
* daha kisa surede arama yapar. Toplam bulunan kelime sayisini return edip log
* dosyasi olusturur. Ana log dosyasinin ayarlanmasi mainda halldilecektir.
* @param dirPart : klasor adi
* @param word : aranacak kelime
* @return toplam bulunan kelime sayisini
*/
int searchDirRec(const char *dirPath,const char *word,int fd);

/*
 Bu metod verilen klasor icinde kelime arar. Bulunan tum koordinatlar log
 dosyasina yazilir. DEF_LOG_FILE_NAME kullanilmistir. Recursive arama yapar.
 @param dirParh : aramanin baslanacagi klasor yolu/adi
 @param word : aranacak kelime
 @return : toplam bulunan kelime sayisi
*/
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
* wordu arar. Her buldugunu log descriptoruna kaydeder. Dosya uzerinde lseek ile
* gezme yapildi. Memorye alinan birsey yok.
* @param fileName : arama yapiacak dosya adi
* @param word : aranacak kelime
* @param fd : log file descritor
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

/*
* Bu fonksiyon istenilen size kadar proc_t arrayi olusturur.
* Her ana process kendinden tureyen procesler hakkında bilgi tutacak.
* @param size : istenilen proc_t array boyutu
* @return proc_t* : olusturulan dinamik array
*/
proc_t *createProcessArrays(int size);


/*
  Process dizisinden processi ilklendirmek icin kullanilir.
  File icin olan processlere pipe acar.
  @param proc : hangi process dizisine islem yapilacagi
  @param size : toplam process sayisi
  @param fdStatus : processin dizideki konumu
  @param pid : processin id si
  @return : islem sonucu
*/
bool openPipeConnection(proc_t *ppPipeArr,int size,int fdStatus)

void freePtr(proc_t *proc,int size);


/*
  Bu fonksiyon statusu ve pid si belli process icin fifo acar.
  @param ppFifoArr : dosya procesinin aranacagi array
  @param size : array boyutu
  @param drStatus : dosya procesinin dizi icindeki konumu
  @return : fifo acilirsa true, diger durumlarda false
*/
bool openFifoConnection(proc_t *ppFifoArr,int size,int drStatus);


ssize_t r_read(int fd, void *buf, size_t size);
ssize_t r_write(int fd, void *buf, size_t size);
int copyfile(int fromfd, int tofd);
int readwrite(int fromfd, int tofd);



#endif
