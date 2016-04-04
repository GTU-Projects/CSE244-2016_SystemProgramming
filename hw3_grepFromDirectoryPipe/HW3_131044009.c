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

  #ifdef DEBUG2
  printf("[%ld] searches in :%s\n",(long)getpid(),fileName);
  #endif
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
        #ifdef DEBUG2
        printf("%d. %d %d\n",found,row,column);
        #endif
        sprintf(wordCoordinats,"%d%c%d%c%d%c",found,'.',row,' ',column,'\n');
        write(fd,wordCoordinats,strlen(wordCoordinats));
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


void findContentNumbers(DIR* pDir,const char *dirPath,int *fileNumber,int *dirNumber){
  struct dirent * pDirent=NULL;
  char path[PATH_MAX];

  *fileNumber=0;
  *dirNumber=0;

  while(NULL != (pDirent = readdir(pDir))){
    sprintf(path,"%s/%s",dirPath,pDirent->d_name);
    if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
      #ifdef DEBUG
        printf("-->%d. Elem in %s is %s\n",((*dirNumber)+(*fileNumber)),dirPath,path);
      #endif
      if(TRUE == isDirectory(path)){
          ++(*dirNumber);
      }
      if(TRUE == isRegularFile(path)){
        ++(*fileNumber);
      }
    }
  }
    #ifdef DEBUG
      printf("-->Founded %d file and %d in %s.\n",*fileNumber,*dirNumber,dirPath);
    #endif
    rewinddir(pDir);
    pDirent=NULL;
}

/* her procesin bilgisini kaydetmek icin dizi olustur*/
proc_t *createProcessArrays(int size){

  proc_t *arr=NULL;
  if(size <=0)
    return arr;

  #ifdef DEBUG
  printf("[%ld] create process array.\n",(long)getpid());
  #endif
  return (proc_t*)malloc(sizeof(proc_t)*size);
}


/**
  Process dizisinden processi ilklendirmek icin kullanilir.
  File icin olan processlere pipe acar.
  @param proc : hangi process dizisine islem yapilacagi
  @param size : toplam process sayisi
  @param fdStatus : processin dizideki konumu
  @param pid : processin id si
  @return : islem sonucu
*/
bool openPipeConnection(proc_t *ppPipeArr,int size,int fdStatus){

  if(NULL == ppPipeArr || size<=0 || fdStatus<0 || fdStatus >=size )
    return FALSE;

    if(FAIL == pipe(ppPipeArr[fdStatus].fd)){
      fprintf(stderr, "Failed to open pipe. Errno : %s\n",strerror(errno));
          return FALSE;
    }

    ppPipeArr[fdStatus].pid = getpid();
    #ifdef DEBUG
      printf("#Pipe opened. Status : %d. Pid : %ld\n",
                                      fdStatus,(long)ppPipeArr[fdStatus].pid);
    #endif

  return TRUE;
}

bool openFifoConnection(proc_t *ppFifoArr,int size,int drStatus){

  char fifoName[FILE_NAME_MAX];

  if(NULL == ppFifoArr || size<=0 || drStatus<0 || drStatus >=size )
    return FALSE;


    /* childin pid si ile acacak*/
  sprintf(fifoName,"%ld-%d.fifo",(long)ppFifoArr[drStatus].pid,drStatus);

  printf("Fifo : %s created.\n",fifoName);
  if(FAIL == mkfifo(fifoName,FIFO_PERMS)){
    if(errno != EEXIST){
      fprintf(stderr,"FIFO ERROR : %s",strerror(errno));
      exit(FAIL);
    }
  }
  return TRUE;
}

int searchDir(const char *dirPath,const char * word){

  int fd;
  char fifoName[FILE_NAME_MAX];
  int total=0;
  /* parent icin log olustur */
  sprintf(fifoName,"%ld.mercan",(long)9415);

  fd = open(fifoName,WRITE_FLAGS,FIFO_PERMS);
  total = searchDirRec(dirPath,word,fd);

  close(fd);

  return total;
}

int getID(proc_t *arr,int size,pid_t pid){

  int i=0;
  if(NULL == arr || size <=0)
    return FAIL;

  for(i=0;i<size;++i)
    if(arr[i].pid==pid)
      return i;
  return FAIL;
}

int searchDirRec(const char *dirPath, const char *word,int fd){
  DIR *pDir = NULL;
  struct dirent *pDirent=NULL;
  pid_t pidChild=-1;
  pid_t pidReturned;
  int totalWord=0;
  proc_t *ppPipeArr=NULL;
  proc_t *ppFifoArr=NULL;
  int fileNumber=0;
  int fdStatus;
  int drStatus;
  int directoryNumber=0;
  char path[PATH_MAX];
  char fifoName[FILE_NAME_MAX];

  #ifdef DEBUG
  printf("[%ld]",(long)getpid());
  printf("+>Directory : %s opened!\n",dirPath);
  #endif

  if(NULL == (pDir = opendir(dirPath))){
    fprintf(stderr,"Failed to open: \"%s\". Errno : %s\n",dirPath,strerror(errno));
    return FAIL;
  }

  findContentNumbers(pDir,dirPath,&fileNumber,&directoryNumber);

  ppPipeArr = createProcessArrays(fileNumber);
  ppFifoArr = createProcessArrays(directoryNumber);

  fdStatus=-1;
  drStatus=-1;
  while(NULL != (pDirent = readdir(pDir))){
    bool isFileProc=FALSE;
      sprintf(path,"%s/%s",dirPath,pDirent->d_name);

      if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
        if(TRUE == isDirectory(path)){
          ++drStatus;
          ppFifoArr[drStatus].pid = getpid();
          openFifoConnection(ppFifoArr,directoryNumber,drStatus);
        }else if(TRUE == isRegularFile(path)){
          isFileProc = TRUE;
          ++fdStatus;
          /* forktan once sirasiyla pipe ac*/
          openPipeConnection(ppPipeArr,fileNumber,fdStatus);
        }

        if((pidChild = fork()) == FAIL){
          fprintf(stderr,"Failed to create fork. Errno : %s\n",strerror(errno));
          exit(FAIL);
        }
        if(pidChild == 0){
          break;
        }else{
          if(isFileProc == TRUE){
            /* pipe lere id leri koyki daha sonradan olen cocugun
            pid sine gore pipe ini bul ve ordan okuma yap */
            ppPipeArr[fdStatus].pid = pidChild;
          }else{
            /* FIFO NUN READ UCUNU AC*/
            ppFifoArr[drStatus].pid = getpid();
            sprintf(fifoName,"%ld-%d.fifo",(long)ppFifoArr[drStatus].pid,drStatus);
            ppFifoArr[drStatus].fd[0]= open(fifoName, O_RDONLY) ;
             if (ppFifoArr[drStatus].fd[0] == -1) {
                fprintf(stderr, "[%ld]:failed to open named pipe %s for read: %s\n",
                       (long)getpid(), fifoName, strerror(errno));
                return -1;
             }
             ppFifoArr[drStatus].pid = pidChild;
          }
        }
      }
  }

  /* eger cocuk ise*/
  if(pidChild == 0){
    int whichProcDead=0;
    if(TRUE == isRegularFile(path) && fdStatus != -1){
      close(ppPipeArr[fdStatus].fd[0]); /* READ KAPISI KAPALI */
      totalWord += findOccurencesInFile(ppPipeArr[fdStatus].fd[1],path,word);
      close(ppPipeArr[fdStatus].fd[1]);
      whichProcDead = FILE_PROC_DEAD;

      closedir(pDir);
      freePtr(ppPipeArr,fileNumber);
      freePtr(ppFifoArr,directoryNumber);
      pDir = NULL;
      pDirent = NULL;
      ppPipeArr = NULL;
      ppFifoArr = NULL;
      exit(whichProcDead);
    }else if(TRUE == isDirectory(path) && drStatus != -1){
      pid_t temp;
      sprintf(fifoName,"%ld-%d.fifo",(long)getppid(),drStatus);
      ppFifoArr[drStatus].fd[1] = open(fifoName,O_WRONLY);
      if (ppFifoArr[drStatus].fd[1] == -1) {
      fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
             (long)getpid(), fifoName, strerror(errno));
      exit(1);
      }

      closedir(pDir);
      temp = ppFifoArr[drStatus].fd[1];
      freePtr(ppPipeArr,fileNumber);
      freePtr(ppFifoArr,directoryNumber);
      searchDirRec(path,word,temp);
      close(temp);
      unlink(fifoName);
      whichProcDead = (DIR_PROC_DEAD); /* directory oldugunu bildir */
      exit(whichProcDead);
    }


  }else if(pidChild > 0){
    int whoDead;
    int status;
    int id;

    while(FAIL != (pidReturned = wait(&status))){
      whoDead = WEXITSTATUS(status);
      if(whoDead ==  FILE_PROC_DEAD){
        id = getID(ppPipeArr,fileNumber,pidReturned);
        close(ppPipeArr[id].fd[1]);
        copyfile(ppPipeArr[id].fd[0],fd);
        close(ppPipeArr[id].fd[0]);
      }else{
        id = getID(ppFifoArr,directoryNumber,pidReturned);
        copyfile(ppFifoArr[id].fd[0],fd);
        close(ppFifoArr[id].fd[0]);
      }
    }
  }
  closedir(pDir);
  pDir=NULL;
  pDirent=NULL;
  freePtr(ppPipeArr,fileNumber);
  freePtr(ppFifoArr,directoryNumber);
  ppFifoArr = NULL;
  ppPipeArr = NULL;
  return totalWord;
}

void freePtr(proc_t *arr,int size){
  if(size != 0){
    #ifdef DEBUG
    printf("[%ld] free process array.\n",(long)getpid());
    #endif
    free(arr);
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
