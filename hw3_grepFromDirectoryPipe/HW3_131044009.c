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

  #ifdef DEBUG
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
        #ifdef DEBUG
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
      printf("+->Founded %d file and %d in %s.\n",*fileNumber,*dirNumber,dirPath);
    #endif
    rewinddir(pDir);
    pDirent=NULL;
}

/* her procesin bilgisini kaydetmek icin dizi olustur*/
proc_t *createProcessArrays(int size){
  if(size <=0)
    return NULL;
  return (proc_t *)malloc(sizeof(proc_t)*size);
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
bool openPipeConnection(proc_t *proc,int size,int fdStatus){

  if(NULL == proc || size<=0 || fdStatus<0 || fdStatus >=size )
    return FALSE;

    if(FAIL == pipe(proc[fdStatus].fd)){
      fprintf(stderr, "Failed to open pipe. Errno : %s\n",strerror(errno));
          return FALSE;
    }
    proc[fdStatus].id=fdStatus;
    proc[fdStatus].pid = getpid();

    #ifdef DEBUG
      printf("#Pipe opened. Status : %d, Pid : %ld\n",fdStatus,(long)proc[fdStatus].pid);
    #endif

  return TRUE;
}

bool openFifoConnection(proc_t *proc,int size,int drStatus){

  char fifoName[FILE_NAME_MAX];

  if(NULL == proc || size<=0 || drStatus<0 || drStatus >=size )
    return FALSE;

  proc[drStatus].id = drStatus;

  sprintf(fifoName,"%ld.fifo",(long)proc[drStatus].pid);

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

int getID(proc_t *proc,int size,pid_t pid){

  int i=0;
  if(NULL == proc || size <=0)
    return FAIL;

  for(i=0;i<size;++i){
    if(proc[i].pid == pid)
      return i;
  }
  return FAIL;
}


int searchDirRec(const char *dirPath, const char *word,int fd){
  DIR *pDir = NULL;
  struct dirent *pDirent=NULL;
  pid_t pidChild=-1;
  pid_t pidReturned;
  int totalWord=0;
  proc_t *t_procFile=NULL;
  proc_t *t_procDir=NULL;
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

  t_procFile = createProcessArrays(fileNumber);
  t_procDir = createProcessArrays(directoryNumber);

  fdStatus=-1;
  drStatus=-1;
  while(NULL != (pDirent = readdir(pDir))){
    bool isFileProc=FALSE;
      sprintf(path,"%s/%s",dirPath,pDirent->d_name);

      if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
        if(TRUE == isDirectory(path)){
          ++drStatus;
        }else if(TRUE == isRegularFile(path)){
          isFileProc = TRUE;
          ++fdStatus;
          openPipeConnection(t_procFile,fileNumber,fdStatus);
        }

        if((pidChild = fork()) == FAIL){
          fprintf(stderr,"Failed to create fork. Errno : %s\n",strerror(errno));
          exit(FAIL);
        }

        if(pidChild == 0){
          break;
        }else{
          if(isFileProc == TRUE){
            /* daha sonra hangi cocuk olduysa onun pidinden dosyalara ulas*/
            t_procFile[fdStatus].pid = pidChild;
          }else{
            /* FIFO NUN READ UCUNU AC*/
            t_procDir[drStatus].pid = pidChild;
            openFifoConnection(t_procDir,directoryNumber,drStatus);
            sprintf(fifoName,"%ld.fifo",(long)t_procDir[drStatus].pid);
            t_procDir[drStatus].fd[0]= open(fifoName, O_RDONLY) ;
             if (t_procDir[drStatus].fd[0] == -1) {
                fprintf(stderr, "[%ld]:failed to open named pipe %s for read: %s\n",
                       (long)getpid(), fifoName, strerror(errno));
                return -1;
             }

          }
        }
      }
  }

  /* eger cocuk ise*/
  if(pidChild == 0){
    int whichProcDead=0;
    if(TRUE == isRegularFile(path) && fdStatus != -1){
      close(t_procFile[fdStatus].fd[0]); /* READ KAPISI KAPALI */
      totalWord += findOccurencesInFile(t_procFile[fdStatus].fd[1],path,word);
      close(t_procFile[fdStatus].fd[1]);
      whichProcDead = FILE_PROC_DEAD;
    }else if(TRUE == isDirectory(path) && drStatus != -1){

      sprintf(fifoName,"%ld.fifo",(long)getpid());
      t_procDir[drStatus].fd[1] = open(fifoName,O_WRONLY);
      if (t_procDir[drStatus].fd[1] == -1) {
      fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
             (long)getpid(), fifoName, strerror(errno));
      exit(1);
      }

      searchDirRec(path,word,t_procDir[drStatus].fd[1]);
      close(t_procDir[drStatus].fd[1]);
      unlink(fifoName);
      whichProcDead = (DIR_PROC_DEAD); /* directory oldugunu bildir */
    }

    closedir(pDir);
    freePtr(t_procFile,fileNumber);
    freePtr(t_procDir,directoryNumber);
    pDir = NULL;
    pDirent = NULL;
    t_procFile = NULL;
    t_procDir = NULL;
    exit(whichProcDead);
  }else if(pidChild > 0){
    int whoDead;
    int status;
    int id;

    while(FAIL != (pidReturned = wait(&status))){
      whoDead = WEXITSTATUS(status);
      if(whoDead ==  FILE_PROC_DEAD){
        id = getID(t_procFile,fileNumber,pidReturned);
        close(t_procFile[id].fd[1]);
        copyfile(t_procFile[id].fd[0],fd);
        close(t_procFile[id].fd[0]);
      }else{
        id = getID(t_procDir,directoryNumber,pidReturned);
        copyfile(t_procDir[id].fd[0],fd);
        close(t_procDir[id].fd[0]);
      }
    }
  }
  closedir(pDir);
  pDir=NULL;
  pDirent=NULL;
  freePtr(t_procFile,fileNumber);
  freePtr(t_procDir,directoryNumber);
  t_procFile = NULL;
  t_procDir = NULL;
  return totalWord;
}

void freePtr(proc_t *proc,int size){
  if(size != 0){
    free(proc);
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
