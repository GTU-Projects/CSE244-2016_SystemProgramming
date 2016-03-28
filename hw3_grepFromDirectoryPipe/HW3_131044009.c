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

int findOccurencesInFile(const char* fileName,const char *word){
  char logFileName[FILE_NAME_MAX];
  char wordCoordinats[COORDINAT_TEXT_MAX];/* dosyaya koordinatlari basmak icin string yuvasi */
  int fdLogFile; /* log dosyasi icin fildes*/
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
          sprintf(logFileName,"%ld",(long)getpid());
          if(FAIL == (fdLogFile = open(logFileName,WRITE_FLAGS,FD_MODE))){
            fprintf(stderr," Failed create \"%s\" : %s ",logFileName,strerror(errno));
            return FAIL;
          }
          /* log dosyasinin basina bilgilendirme olarak path basildi */
          write(fdLogFile,fileName,strlen(fileName));
          write(fdLogFile,"\n",1);
          logCreated =TRUE;
        }
        lseek(fdFileToRead,-i+1,SEEK_CUR);
        column =column - i+1;
        sprintf(wordCoordinats,"\t%d. Row: %d Column: %d\n",found,row,column);
        write(fdLogFile,wordCoordinats,strlen(wordCoordinats));
        i=0;
      }
    }else{
      i=0;
    }
  }
  /* dosyalarin kapatilmasi*/
  close(fdFileToRead);
  if(TRUE == logCreated)
    close(fdLogFile);
  return found;
}


int searchDir(const char *dirPath, const char *word){
  DIR *pDir = NULL;
  struct dirent *pDirent=NULL;
  pid_t pidChild;
  pid_t pidReturned;
  char logFileName[FILE_NAME_MAX];
  int childReadNumber=0;
  /*char cwd[PATH_MAX];*/
  int totalWord=0;
  int fd[2]; /* pipe fd */
  char path[PATH_MAX];


  if(NULL == (pDir = opendir(dirPath))){
    fprintf(stderr,"Failed to open dir : \"%s\". Errno : %s\n",
                                                  dirPath,strerror(errno));
    return FAIL;
  }

  if(FAIL == pipe(fd)){
    fprintf(stderr, "Failed to open pipe. Errno : %s\n",strerror(errno));
    return FAIL;
  }

  while(NULL != (pDirent = readdir(pDir))){
      sprintf(path,"%s/%s",dirPath,pDirent->d_name);
      #ifdef DEBUG
        printf("Item path : %s\n",path);
      #endif
      if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
        if(TRUE == isDirectory(path) || TRUE == isRegularFile(path)){
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

  sprintf(logFileName,"%ld",(long)getpid());

  /* eger cocuk ise*/
  if(pidChild == 0){
      int fd2;
    if(TRUE == isRegularFile(path)){
      totalWord += findOccurencesInFile(path,word);
      fd2 =open(logFileName,READ_FLAGS);
      close(fd[0]); /* READ KAPISI KAPALI */
      copyfile(fd2,fd[1]);
      close(fd[1]);
      close(fd2);
    }else if(TRUE == isDirectory(path)){
      int fd2;
      totalWord += searchDir(path,word);
      fd2 =open(logFileName,READ_FLAGS);
      close(fd[0]); /* READ KAPISI KAPALI */
      copyfile(fd2,fd[1]);
      close(fd[1]);
      close(fd2);
    }

    unlink(logFileName);
    pDir = NULL;
    pDirent = NULL;
    exit(totalWord);
  }else{
    while(FAIL != (pidReturned = wait(&childReadNumber))){
      int fd2;
      totalWord = WEXITSTATUS(childReadNumber);
      fd2 = open(logFileName,WRITE_FLAGS,FD_MODE);
      close(fd[1]); /* wrıte KAPISI KAPALI */
      copyfile(fd[0],fd2);
      close(fd2); /* LOG DOSYASINI KAPA */
    }
    close(fd[0]);
  }
  closedir(pDir);
  pDir=NULL;
  pDirent=NULL;
  return totalWord;
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
