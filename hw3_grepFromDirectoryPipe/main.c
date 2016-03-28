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
#include "HW3_131044009.h"

int main(int argc,char *argv[]){

  char logName[FILE_NAME_MAX];
  int totalFound=0;

  if(argc != 3){
    fprintf(stderr,"Usage : %s DirectoryName \"string\" ",argv[0]);
    return 1;
  }

  totalFound = searchDir(argv[1],argv[2]);
  printf("Total : %d",totalFound);

  sprintf(logName,"%ld",(long)getpid());

  rename(logName,DEF_LOG_FILE_NAME);


  return 0;
}
