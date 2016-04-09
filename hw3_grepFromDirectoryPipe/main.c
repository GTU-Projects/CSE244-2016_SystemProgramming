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
#include <signal.h>
#include "HW3_131044009.h"


void sigIntHandler(int sigNo){
  FILE *fpLOG;
  printf("[%ld] handled INT[%d] SIGNAL\n",(long)getpid(),sigNo);
  fpLOG = fopen(DEF_LOG_FILE_NAME,"w");
  fprintf(fpLOG,"FAILED TO SEARCH WORDS\n");
  fprintf(fpLOG,"SIGINT(CTRL + C) Handled.\n");
  fclose(fpLOG);

  freePtr(NULL,0);
  exit(1);
}

int main(int argc,char *argv[]){

  int totalFound=0;
  signal(SIGINT,sigIntHandler);

  if(argc != 3){
    fprintf(stderr,"Usage : %s DirectoryName \"string\" ",argv[0]);
    return 1;
  }

  totalFound = searchDir(argv[1],argv[2]);
  printf("TOTAL : %d\n",totalFound);
  return 0;
}
