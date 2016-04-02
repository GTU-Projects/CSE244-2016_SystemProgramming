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
#include "HW3_131044009.h"

#define DEBUG

int findOccurencesInFile(int fd,const char* fileName,const char *word){
  char wordCoordinats[COORDINAT_TEXT_MAX];/* dosyaya koordinatlari basmak icin string yuvasi */
  int fdFileToRead; /* okunacak dosya fildesi */
  char buf;
  int i=0;
  int column=-1;
  int row=0;
  int found=0;
  bool logCreated = FALSE;

  if((fdFileToRead = open(fileName,READ_FLAGS)) == FAIL){
    fprintf(stderr," Failed open \"%s\" : %s ",fileName,strerror(errno));
    return FAIL;
  }

  printf("[%ld] :%s\n",(long)getpid(),fileName);

/* Karakter karakter ilerleyerek kelimeyi bul. Kelimenin tum karekterleri arka
arkaya bulununca imleci geriye cek ve devam et. Tum eslesen kelimeleri bul
*/
  while(read(fdFileToRead,&buf,sizeof(char))){
    ++column; /* hangi sutunda yer aliriz*/
    if(buf == '\n'){
      i=0;
      column=-1;
      ++row;
    }else if(buf == word[i]){
      ++i;
      if(i == strlen(word)){ /* kelime eslesti dosyaya yaz imleci geri al*/
        ++found;
         /*  EGER KELIME VARSA LOG FILE AC VE YAZ YOKSA ELLEME */
        if(logCreated == FALSE){
          /* log dosyasinin basina bilgilendirme olarak path basildi */
          write(fd,fileName,strlen(fileName));
          write(fd,"\n",1);
          logCreated =TRUE;
        }
        lseek(fdFileToRead,-i+1,SEEK_CUR);
        column =column - i+1;
        printf("%d. %d %d\n",found,row,column);
        sprintf(wordCoordinats,"%d%c%d%c%d%c",found,'.',row,' ',column,'\n');
        write(fd,wordCoordinats,strlen(wordCoordinats)+1);
        i=0;
      }
    }else{
      i=0;
    }
  }
  /* dosyalarin kapatilmasi*/
  close(fdFileToRead);
  return found;
}


int searchDir(const char *dirPath, const char *word){
  DIR *pDir = NULL;
  struct dirent *pDirent=NULL;
  pid_t pidChild;
  pid_t pidReturned;
  int totalWord=0;
  int **fd;
  int **fifoD;
  int fileNumber=0;
  int fdStatus;
  int drStatus;
  int directoryNumber=0;
  char path[PATH_MAX];
  int i=0;


  if(NULL == (pDir = opendir(dirPath))){
    fprintf(stderr,"Failed to open dir : \"%s\". Errno : %s\n",
                                                  dirPath,strerror(errno));
    return FAIL;
  }


  while(NULL != (pDirent = readdir(pDir))){
    sprintf(path,"%s/%s",dirPath,pDirent->d_name);
    if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
      if(TRUE == isDirectory(path)){
          ++directoryNumber;
      }
      if(TRUE == isRegularFile(path)){
        ++fileNumber;
      }
    }
  }

  #ifdef DEBUG
    printf("%s - ",dirPath);
    printf("File number : %d  - Directory Number : %d\n",fileNumber,directoryNumber);
  #endif
  rewinddir(pDir);


  if(fileNumber != 0){
    fd = (int **)malloc(fileNumber*sizeof(int *));
    for(i=0;i<fileNumber;++i){
      fd[i]=(int *)malloc(2*sizeof(int));
      if(FAIL == pipe(fd[i])){
        fprintf(stderr, "%d Failed to open pipe. Errno : %s\n",i,strerror(errno));
        return FAIL;
      }
    }
}

  if(directoryNumber != 0){
    fifoD = (int **)malloc(directoryNumber*sizeof(int *));
    if(fifoD == NULL)
      printf("control");
    for(i=0;i<directoryNumber;++i){
      fifoD[i]=(int *)malloc(2*sizeof(int));
    }
}

fdStatus=-1;
drStatus=-1;
  while(NULL != (pDirent = readdir(pDir))){
      sprintf(path,"%s/%s",dirPath,pDirent->d_name);
      #ifdef DEBUG
        printf("Item path : %s\n",path);
      #endif
      if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
        if(TRUE == isDirectory(path)){
          /*TODO :DIRECTORY SEARCH*/
        }else if(TRUE == isRegularFile(path)){
          ++fdStatus;
          if((pidChild = fork()) == FAIL){
            fprintf(stderr,"Failed to create fork. Errno : %s\n",strerror(errno));
            exit(FAIL);
          }

          if(pidChild == 0){
            closedir(pDir);
            break;
          }
        }
      }
  }

  /* eger cocuk ise*/
  if(pidChild == 0){
    if(TRUE == isRegularFile(path)){
      close(fd[fdStatus][0]); /* READ KAPISI KAPALI */
      totalWord += findOccurencesInFile(fd[fdStatus][1],path,word);
      close(fd[fdStatus][1]);
    }else if(TRUE == isDirectory(path)){
      /* TODO : CONTROL DIRECTORY */
    }
    pDir = NULL;
    pDirent = NULL;
    freePtr(fd,fileNumber);
    freePtr(fifoD,directoryNumber);
    fd = NULL;
    fifoD = NULL;
    exit(fdStatus);
  }else{
    int status;
    int logfd = open("log.log",(O_WRONLY),FIFO_PERMS);
    while(FAIL != (pidReturned = wait(&status))){
      fileNumber = WEXITSTATUS(status);
      close(fd[fileNumber][1]);/* wrıte KAPISI KAPALI */
      copyfile(fd[fileNumber][0],logfd);
      close(fd[fileNumber][0]);
    }
    close(logfd);
  }
  closedir(pDir);
  pDir=NULL;
  pDirent=NULL;
  freePtr(fd,fileNumber);
  freePtr(fifoD,directoryNumber);
  fd = NULL;
  fifoD = NULL;
  return totalWord;
}

void freePtr(int **ptr,int size){
  int i=0;
  printf("Size : %d",size);
  if(size != 0){
    for(i=0;i<size;++i)
      free(ptr[i]);
    free(ptr);
  }
}


/* DERS KITABINDAN ALINDI */
ssize_t r_read(int fd, void *buf, size_t size) {
   ssize_t retval;
   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
   return retval;
}

ssize_t r_write(int fd, void *buf, size_t size) {
   char *bufp;
   size_t bytestowrite;
   ssize_t byteswritten;
   size_t totalbytes;

   for (bufp = buf, bytestowrite = size, totalbytes = 0;
        bytestowrite > 0;
        bufp += byteswritten, bytestowrite -= byteswritten) {
      byteswritten = write(fd, bufp, bytestowrite);
      if ((byteswritten) == -1 && (errno != EINTR))
         return -1;
      if (byteswritten == -1)
         byteswritten = 0;
      totalbytes += byteswritten;
   }
   return totalbytes;
}

int copyfile(int fromfd, int tofd) {
   int bytesread;
   int totalbytes = 0;

   while ((bytesread = readwrite(fromfd, tofd)) > 0)
      totalbytes += bytesread;
   return totalbytes;
}

int readwrite(int fromfd, int tofd) {
   char buf[BLKSIZE];
   int bytesread;

   if ((bytesread = r_read(fromfd, buf, BLKSIZE)) < 0)
      return -1;
   if (bytesread == 0)
      return 0;
   if (r_write(tofd, buf, bytesread) < 0)
      return -1;
   return bytesread;
}

bool isRegularFile(const char * fileName){
  struct stat statbuf;

  if(stat(fileName,&statbuf) == -1){
    return FALSE;
  }else{
    return S_ISREG(statbuf.st_mode) ? TRUE : FALSE;
  }
}

bool isDirectory(const char *dirName){
  struct stat statbuf;
      if(stat(dirName   ,& statbuf) == -1){
    return FALSE;
  }
  else {
    return S_ISDIR(statbuf.st_mode) ? TRUE : FALSE;
  }
}
