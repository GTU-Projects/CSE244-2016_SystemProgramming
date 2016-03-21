/*######################################################################*/
/*       GTU244 System Programming HW2 - Grep From Directory            */
/*                     HASAN MEN - 131044009                            */
/*                                                                      */
/*USAGE  : ./exec "diretory name" "word"                                */
/*######################################################################*/

/* KULLANILAN KUTUPHANELER */
#include <stdio.h>
#include <string.h> /*strerror*/
#include <stdlib.h> /* atoi*/
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h> /*open*/
#include <sys/stat.h>
#include <dirent.h> /* DIR* */
#include <sys/stat.h>
#include <sys/wait.h> /*wait */
#include <errno.h>

/* Yeni Sayisal Tip*/
typedef enum{
  FALSE=0,TRUE=1
}bool;

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
bool isCharacterSpecialFile(const char *fileName);

/*
* Bu method parametre olarak gelen dosyayi acar ve icinde diger parametre olan
* wordu arar. Her buldugunu log dosyasina kaydeder. Dosya uzerinde lseek ile
* gezme yapildi. Memorye alinan birsey yok.
* @param fileName : arama yapiacak dosya adi
* @para word : aranacak kelime
* @return dosyadaki toplam eslesen kelime sayisi
*/
int findOccurencesInFile(const char* fileName,const char *word);


/*
* Bu method yardimci olarak kullanilicak.
* kendisine gelen pid numarasini string olarak return eder.
* @return string olarak numara  -> free edilmesi lazım
* @param number : pid numarasi
*/
char *getStringOfNumber(long number);

#define FAIL -1
#define SUCCESS 0
#define CHILD_PROCESS 0
#define READ_FLAGS (O_RDONLY)
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define FD_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)



int main(int argc,char *argv[]){

  char *pCh_logFileName=NULL;
  int totalFound=0;

  if(argc != 3){
    fprintf(stderr,"Usage : %s DirectoryName \"string\" ",argv[0]);
    return 1;
  }

  /* kelime ara */
  totalFound = searchDir(argv[1],argv[2]);

  /* argv[1] icinde olusan son log dosyasini executable yanina tasi */
  chdir(argv[1]);
  printf("Total Found : %d\n",totalFound);
  pCh_logFileName=getStringOfNumber(getpid());
  chdir("..");
  rename(pCh_logFileName,DEF_LOG_FILE_NAME); /* logun adini degistir */
  free(pCh_logFileName);
  pCh_logFileName=NULL;

  return 0;
}

int searchDir(const char *dirPath,const char *word){

  DIR* pDir=NULL; /* klasor */
  struct dirent *pDirent=NULL; /* klasor icerigi */
  pid_t pidChild;
  pid_t pidTerminated; /* parente geri donen child */
  char cwd[PATH_MAX]; /* mevcut dosya yolu */
  char *pCh_TerminatedProcess=NULL; /* olen processin adi */
  char *pCh_logFileName=NULL;
  char direntName[PATH_MAX]; /* klasor icindeki veri adi */
  int howManyChildRead=0;
  int totalFound=0; /* toplam eslesen kelime */

  if(NULL == (pDir = opendir(dirPath))){
    fprintf(stderr, "Failed open %s -> Errno : %s",dirPath,strerror(errno));
    return FAIL;
  }

  getcwd(cwd,PATH_MAX);
  chdir(dirPath); /* klasorun icine gir */
  getcwd(cwd,PATH_MAX);

  /* tum klasor icine bakar
     eger iceride txt ve klasorler varsa fork edip bekler */
  while(NULL != (pDirent = readdir(pDir))){
   if(strcmp( pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
     if(TRUE == isCharacterSpecialFile(pDirent->d_name) ||
          TRUE == isDirectory(pDirent->d_name)){
       pidChild = fork();
       if(pidChild <= 0){
         strcpy(direntName,pDirent->d_name);
         closedir(pDir); /*Ust tarafta acik olan klasorler kapandi*/
        break;
       }
    }
   }
 }

/* Childlerden gelen exit bilgisine gore log dosyalarini toplar her parent
  loglar toplanip ortak log yapildiktan sonra silinirler
*/
 if(pidChild >0){
   while(FAIL != (pidTerminated = wait(&howManyChildRead))){
     totalFound+=WEXITSTATUS(howManyChildRead); /* child kac gelime buldu */
     pCh_logFileName=getStringOfNumber(getpid());
     pCh_TerminatedProcess = getStringOfNumber(pidTerminated);
     addLog(dirPath,pCh_TerminatedProcess,pCh_logFileName);
     unlink(pCh_TerminatedProcess);
     free(pCh_TerminatedProcess);
     free(pCh_logFileName);
     pCh_logFileName=NULL; /* dangling pointerler silindi */
     pCh_TerminatedProcess=NULL;
     }
 }

/* Eger child txt gorduyse okur , klasor gorduyse revursive olarak devam eder*/
 if(pidChild ==0){
   if(TRUE == isCharacterSpecialFile(direntName)){
     totalFound += findOccurencesInFile(direntName,word);
   }else if(TRUE == isDirectory(direntName)){
     totalFound += searchDir(direntName,word);
    }
    pDir=NULL;
    pDirent=NULL;
    exit(totalFound); /* Toplam bulunan sayi parente yollandi */
 }

  chdir("..");
  closedir(pDir);
  pDir=NULL;
  pDirent=NULL;
  return totalFound;
}

void addLog(const char *dirPath,const char* fileName,const char* logName){
  int fdLog;
  int fdChildLog;
  char ch;

 /* dosya acma kapama kontrolleri */
  fdChildLog = open(fileName,READ_FLAGS);
  if(FAIL == fdChildLog){
    fprintf(stderr, "Failed to open childLog -> errno : %s\n",strerror(errno));
    exit(1);
  }
  chdir(".."); /* log dosyasi bir ust dizinde olusturuldu */
  fdLog = open(logName,WRITE_FLAGS,FD_MODE); /*append modunda acildi*/
  if(FAIL == fdLog){
    fprintf(stderr, "Fail to create logFile errno : %s\n",strerror(errno));
    exit(1);
  }
 /*bir bir kopyasi log dosyasina eklendi*/
  while(read(fdChildLog,&ch,1)){
    write(fdLog,&ch,1);
  }
  chdir(dirPath);
  close(fdChildLog);
  close(fdLog);
}


int findOccurencesInFile(const char* fileName,const char *word){

  pid_t PID;
  char *chrPtrTempFileName;
  int fdTempFile; /* log dosyasi icin fildes*/
  int fdFileToRead; /* okunacak dosya fildesi */
  char buf;
  int foundNum=0; /* bulunan kelime */
  int index=0; /* dosya imlec indexi */
  int equalCh=0; /* eslesen karakter sayisi */
  int i=0;
  int column=0;
  int row=1;
  /* dosyaya koordinatlari basmak icin string yuvasi */
  char *tempCoordinatText = malloc(sizeof(char)*17+sizeof(int)*3);
  char cwd[PATH_MAX]; /* aktif dosya yolu */

  if((fdFileToRead = open(fileName,READ_FLAGS)) == FAIL){
    fprintf(stderr," Failed open \"%s\" : %s ",fileName,strerror(errno));
    return FAIL;
  }

  PID = getpid();
  chrPtrTempFileName = getStringOfNumber((long)PID);
  #ifdef DEBUG
  printf("PID : %ld -- ",(long)PID);
  printf("FileName(char *) :%s\n",chrPtrTempFileName);
  #endif

  if(FAIL == (fdTempFile = open(chrPtrTempFileName,WRITE_FLAGS,FD_MODE))){
    fprintf(stderr," Failed create \"%s\" : %s ",chrPtrTempFileName,strerror(errno));
    return FAIL;
  }

  /* log dosyasinin basina bilgilendirme olarak path basildi */
  getcwd(cwd,PATH_MAX);
  write(fdTempFile,cwd,strlen(cwd));
  write(fdTempFile,"/",1);
  write(fdTempFile,fileName,strlen(fileName));
  write(fdTempFile,"\n",1);

/* Karakter karakter okuam yapilir. Eger kelime tam olarak eslesmenden araya
baska karakter girerse dosya imleci ilk bulunan karakterden sonraya alinir ve
okumaya devam edilir. Boylece tum dosya taranir
*/
while(read(fdFileToRead,&buf,sizeof(char))){
    ++index;
    if(buf == '\n'){
      ++row;
      index=0;
      equalCh=0;
    }else if(buf == word[0]){
      column=index;
      ++equalCh;
      for(i=1;i<strlen(word);++i){ /* tum harfler bulunursa */
        read(fdFileToRead,&buf,sizeof(char));
        ++index;
        if(buf == word[i]){
          ++equalCh;
        }else if(buf == '\n'){ /* araya new line girmesin*/
          equalCh=0;
          ++row;
          index=0;
          break;
        }else{
          break;
        }
      }
      if(equalCh == strlen(word)){ /* kelime bulundu ve dosyaya yazildi*/
        #ifdef DEBUG
        printf("Index of Firt equal Characters : %d\n",column-1);
        printf("Index of end of equality : %d\n",index-1);
        #endif
        ++foundNum;
        lseek(fdFileToRead,-equalCh+1,SEEK_CUR);
        index = index -equalCh+1;
        #ifdef DEBUG
        printf("Index after lseek : %d\n",index);
        #endif
        sprintf(tempCoordinatText,"%d. Row: %d Column: %d\n",foundNum,row,column);
        write(fdTempFile,tempCoordinatText,strlen(tempCoordinatText));
        equalCh=0;
      }
    }
  }

  /* dosyalarin kapatilmasi ve free islemleri*/
  free(tempCoordinatText);
  free(chrPtrTempFileName);
  close(fdFileToRead);
  close(fdTempFile);
  return foundNum;
}

bool isDirectory(const char *dirName){
  struct stat statbuf;
  if(stat(dirName,&statbuf) == -1){
    return FALSE;
  }
  else {
    return S_ISDIR(statbuf.st_mode) ? TRUE : FALSE;
  }
}

char *getStringOfNumber(long number){
  char *string;
  string= malloc(sizeof(long));
  sprintf(string,"%ld",number);
  return string;
}

bool isCharacterSpecialFile(const char *path){
  int i=0;
  int sizeOfFile=0;
  int sizeOfExtension=0;
  char extension[] =".txt";
  sizeOfFile=strlen(path);
  sizeOfExtension=strlen(extension);
  for(i=0;i<strlen(extension);++i){ /* SON 4 KARAKTER KONTROL EDILIR*/
    if(path[sizeOfFile -sizeOfExtension+i] != extension[i]){
      return FALSE;
    }
  }
  return TRUE;
}
