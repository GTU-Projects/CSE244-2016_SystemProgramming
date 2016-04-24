
/*
* Belirtilen process icin fifo acar.
* fifoName : procesID-drStatus.fifo
*/
bool openFifoConnection(proc_t *ppFifoArr,int size,int drStatus){

  char fifoName[FILE_NAME_MAX];

  if(NULL == ppFifoArr || size<=0 || drStatus<0 || drStatus >=size )
    return FALSE;

    /* childin pid si ve status ile acacak*/
  sprintf(fifoName,"/tmp/.%ld-%d.fifo",(long)ppFifoArr[drStatus].pid,drStatus);

  #ifdef DEBUG
  printf("Fifo : %s created.\n",fifoName);
  #endif
  if(FAIL == mkfifo(fifoName,FIFO_PERMS)){
    if(errno != EEXIST){
      fprintf(stderr,"FIFO ERROR : %s",strerror(errno));
      exit(FAIL);
    }
  }
  return TRUE;
}

/*
  Bu fonksiyon wrapper olarak kullanilacak. Kullaniciya daha kolay kullanim
  saglamak amaciyla recursive arama yapacak searchDir metodunu cagirir.
  Detayli bilgi headerde.
*/
int searchDir(const char *dirPath,const char * word){
  int fd;
  int total=0;
  int temp=0;
  FILE *fpTotal;
  /* parent icin log olustur */
  fd = open(DEF_LOG_FILE_NAME,(O_WRONLY | O_CREAT),FD_MODE);
  searchDirRec(dirPath,word,fd);
  close(fd);

  fpTotal = fopen(TOTAL_AMOUNT_LOG,"r");
  while(fscanf(fpTotal,"%d",&temp)!=EOF){
    total+=temp;
  }
  fclose(fpTotal);
  unlink(TOTAL_AMOUNT_LOG);

  return total;
}

/*
  id si verilen procesin array icindeki konumunu return eder.
*/
int getID(proc_t *arr,int size,pid_t pid){
  int i=0;
  if(NULL == arr || size <=0)
    return FAIL;

  for(i=0;i<size;++i)
    if(arr[i].pid==pid)
      return i;
  return FAIL;
}

/*
  Recursive olarak directory icinde arama yapar. Buldugu sonucları fd ye yazar.
*/
int searchDirRec(const char *dirPath, const char *word,int fd){
  DIR *pDir = NULL;
  struct dirent *pDirent=NULL;
  pid_t pidChild=-1;
  pid_t pidReturned;
  int totalWord=0;

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

  /* onceden toplam file/dir sayisini bulalim*/
  findContentNumbers(pDir,dirPath,&fileNumber,&directoryNumber);

  /* Proces bilgilerini kaydetmek icin arayimizi olusturalim*/
  ppPipeArr = createProcessArrays(fileNumber);
  ppFifoArr = createProcessArrays(directoryNumber);

  fdStatus=-1;
  drStatus=-1;
  while(NULL != (pDirent = readdir(pDir))){
    bool isFileProc=FALSE;
      sprintf(path,"%s/%s",dirPath,pDirent->d_name);
      if(strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
        /* klasor bulma durumunda fifoyu ac */
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

        /* FORKING */
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
            ppFifoArr[drStatus].pid = getpid(); /* once annenin pidiyle pipe ac*/
            sprintf(fifoName,"/tmp/.%ld-%d.fifo",(long)ppFifoArr[drStatus].pid,drStatus);
            ppFifoArr[drStatus].fd[0]= open(fifoName, O_RDONLY) ;
             if (ppFifoArr[drStatus].fd[0] == -1) {
                fprintf(stderr, "[%ld]:failed to open named pipe %s for read: %s\n",
                       (long)getpid(), fifoName, strerror(errno));
                return -1;
             }
             ppFifoArr[drStatus].pid = pidChild; /* daha sonradan erisim
              icin cocugun pidini yaz*/
          }
        }
      }
  }

  /* eger cocuk ise*/
  if(pidChild == 0){
    int whichProcDead=0;

    if(TRUE == isRegularFile(path) && fdStatus != -1){
      FILE *fpTotal;
      close(ppPipeArr[fdStatus].fd[0]); /* READ KAPISI KAPALI */
      totalWord += findOccurencesInFile(ppPipeArr[fdStatus].fd[1],path,word);

      fpTotal = fopen(TOTAL_AMOUNT_LOG,"a+");
      fprintf(fpTotal,"%d\n",totalWord);
      fclose(fpTotal);

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
      sprintf(fifoName,"/tmp/.%ld-%d.fifo",(long)getppid(),drStatus);
      ppFifoArr[drStatus].fd[1] = open(fifoName,O_WRONLY);
      if (ppFifoArr[drStatus].fd[1] == -1) {
      fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
             (long)getpid(), fifoName, strerror(errno));
      exit(1);
      }

      /* Recursive cagridan oncedaha onceden acilan arrayleri free et*/
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

    /* anne cocuklari olunce bilgileri toplar*/
    while(FAIL != (pidReturned = wait(&status))){
      whoDead = WEXITSTATUS(status);
      if(whoDead ==  FILE_PROC_DEAD){ /* file ise pipe tan okuma yap */
        id = getID(ppPipeArr,fileNumber,pidReturned);
        close(ppPipeArr[id].fd[1]);
        copyfile(ppPipeArr[id].fd[0],fd);
        close(ppPipeArr[id].fd[0]);
      }else{
        /* dosya processi geldigi icin fifodan okuma yap */
        id = getID(ppFifoArr,directoryNumber,pidReturned);
        copyfile(ppFifoArr[id].fd[0],fd);
        close(ppFifoArr[id].fd[0]);
      }
    }
  }
  /* FREE VE DANGLING POINTER ISLEMLERI*/
  closedir(pDir);
  pDir=NULL;
  pDirent=NULL;
  freePtr(ppPipeArr,fileNumber);
  freePtr(ppFifoArr,directoryNumber);
  ppFifoArr = NULL;
  ppPipeArr = NULL;
  return totalWord;
}

/*
  PROCESS arrayini free eder.
*/
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
