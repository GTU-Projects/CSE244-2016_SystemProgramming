#include <stdio.h>
#include <string.h> //strerror
#include <stdlib.h> // atoi
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h> //open
#include <sys/stat.h>
#include <dirent.h> // DIR *
#include <sys/stat.h>
#include <sys/wait.h> //wait
#include <errno.h>
#include "131044009_HW2.h"

//#define DEBUG

char *getStringOfNumber(long number){
  char *string;
  string= malloc(sizeof(long));
  sprintf(string,"%ld",number);
  return string;
}


int findOccurencesInFile(const char* fileName,const char *word){

  pid_t PID;
  char *chrPtrTempFileName;
  int fdTempFile;
  int fdFileToRead;
  char buf;
  int foundNum=0;
  int index=0;
  int equalCh=0;
  int i=0;
  int column=0;
  int row=1;
  char *tempCoordinatText = malloc(sizeof(char)*17+sizeof(int)*3);
  char cwd[PATH_MAX];

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

  getcwd(cwd,PATH_MAX);
  write(fdTempFile,cwd,strlen(cwd));
  write(fdTempFile,"/",1);
  write(fdTempFile,fileName,strlen(fileName));
  write(fdTempFile,"\n",1);

while(read(fdFileToRead,&buf,sizeof(char))){
    ++index;
    if(buf == '\n'){
      ++row;
      index=0;
      equalCh=0;
    }else if(buf == word[0]){
      column=index;
      ++equalCh;
      for(i=1;i<strlen(word);++i){
        read(fdFileToRead,&buf,sizeof(char));
        ++index;
        if(buf == word[i]){
          ++equalCh;
        }else if(buf == '\n'){
          equalCh=0;
          ++row;
          index=0;
          break;
        }else{
          break;
        }
      }
      if(equalCh == strlen(word)){
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

  free(tempCoordinatText);
  free(chrPtrTempFileName);
  close(fdFileToRead);
  close(fdTempFile);
  return foundNum;
}

bool isDirectory(const char *path){
  struct stat statbuf;

  if(stat(path,&statbuf) == -1)
    return FALSE;
  else {
    return S_ISDIR(statbuf.st_mode) ? TRUE : FALSE;
  }
}

bool isCharacterSpecialFile(const char *path){
  int i=0;
  int sizeOfFile=0;
  int sizeOfExtension=0;
  char extension[] =".txt";
  sizeOfFile=strlen(path);
  sizeOfExtension=strlen(extension);
  for(i=0;i<strlen(extension);++i){
    if(path[sizeOfFile -sizeOfExtension+i] != extension[i]){
      return FALSE;
    }
  }
  return TRUE;
}

int addLog(const char *dirPath,const char* fileName,const char* logName){

  int fdLog;
  int fdChildLog;
  char ch;

  fdChildLog = open(fileName,READ_FLAGS);
  if(FAIL == fdChildLog){
    fprintf(stderr, "Failed to create childLog -> errno : %s\n",strerror(errno));
    return FAIL;
  }

  chdir("..");
  fdLog = open(logName,WRITE_FLAGS,FD_MODE);
  if(FAIL == fdLog){
    fprintf(stderr, "Fail -> errno : %s\n",strerror(errno));
    return FAIL;
  }

  while(read(fdChildLog,&ch,1)){
    write(fdLog,&ch,1);
  }

  chdir(dirPath);
  close(fdChildLog);
  close(fdLog);

  return 0;
}


int searchDir(const char *dirPath, const char *word){

  DIR* pDir;
  pid_t terminated;
  pid_t pidChild;
  struct dirent * pDirent;
  char mycwd[PATH_MAX];
  char *pCh_logFileName;
  char *pCh_TerminatedProcess;

  if(NULL == (pDir = opendir(dirPath))){
    fprintf(stderr, "Fail -> Errno : %s",strerror(errno));
    return FAIL;
  }
  // open file and go in
  chdir(dirPath);
  getcwd(mycwd,PATH_MAX);
  //printf("%s\n",mycwd);

  while(NULL != (pDirent = readdir(pDir))){
   if(strcmp( pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0){
     if(TRUE == isCharacterSpecialFile(pDirent->d_name) ||
          TRUE == isDirectory(pDirent->d_name)){
       pidChild = fork();
       if(pidChild == CHILD_PROCESS){
         break;
       }
       //fprintf(stderr,"childpid : %d , pid : %d - TXT File : %s\n",pidChild,getpid(),pDirent->d_name);
    }
   }
 }

 if(pidChild >0){
  while(FAIL != (terminated = wait(NULL))){
    pCh_logFileName=getStringOfNumber(getpid());
    pCh_TerminatedProcess = getStringOfNumber(terminated);
    addLog(dirPath,pCh_TerminatedProcess,pCh_logFileName);
    unlink(pCh_TerminatedProcess);
    free(pCh_logFileName);
    free(pCh_TerminatedProcess);
    }
  }else if(pidChild == CHILD_PROCESS){
    #ifdef DEBUG
      printf("File Path : %s\n",mycwd);
    #endif
    if(TRUE == isDirectory(pDirent->d_name)){
      searchDir(pDirent->d_name,word);
    }else {
      #ifdef DEBUG
        printf("TXT File : %s\n",pDirent->d_name);
      #endif
        findOccurencesInFile(pDirent->d_name,word);
    }
    pDir = NULL;
    pDirent=NULL;
    exit(0);
  }
  return 0;
}
