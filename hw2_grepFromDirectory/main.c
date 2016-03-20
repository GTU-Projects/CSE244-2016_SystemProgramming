#include <stdio.h>
#include <string.h> //strerror
#include <stdlib.h> // atoi
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h> //open
#include <sys/stat.h>
#include <dirent.h> // DIR *
#include <sys/stat.h>
#include <errno.h>
#include "131044009_HW2.h"




int main(int argc,char *argv[]){

  char *pCh_logFileName;

  if(argc != 3){
    fprintf(stderr,"Usage : %s DirectoryName \"string\" ",argv[0]);
    return 1;
  }

    printf("Total Found : %d\n",searchDir(argv[1],argv[2]));
    pCh_logFileName=getStringOfNumber(getpid());
    chdir("..");
    rename(pCh_logFileName,DEF_LOG_FILE_NAME);

    free(pCh_logFileName);
    pCh_logFileName=NULL;

  return 0;
}
