/*######################################################################*/
/*       GTU244 System Programming HW3 - Grep From Directory            */
/*                     HASAN MEN - 131044009                            */
/*                                                                      */
/*USAGE  : ./exec "diretory name" "word"                                */
/*######################################################################*/

/* KULLANILAN KUTUPHANELER */
#include <stdio.h>
#include <string.h> /*strerror*/
#include <stdlib.h> /* atoi*/
#include <unistd.h>
#include "HW3_131044009.h"

int main(int argc,char *argv[]){

  char logName[FILE_NAME_MAX];
  int totalFound=0;int fdFR;char test[FILE_NAME_MAX];
    char fifoName[FILE_NAME_MAX];

  if(argc != 3){
    fprintf(stderr,"Usage : %s DirectoryName \"string\" ",argv[0]);
    return 1;
  }

  totalFound = searchDir(argv[1],argv[2]);
  exit(1);

  sprintf(fifoName,".%ld.log",(long)getpid());
  fdFR= open(fifoName,READ_FLAGS);
    while(read(fdFR,test,FILE_NAME_MAX)!=0);
  printf("Total : %d\n",totalFound);

  sprintf(logName,"%ld",(long)getpid());

  rename(logName,DEF_LOG_FILE_NAME);


  return 0;
}
