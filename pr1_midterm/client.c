#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>

#define SERVER_FIFO_NAME "hmenn.ff"


char strClientLog[CHAR_MAX]; // max 127 in limits.h


char *parseFile(const char* fileName);

int main(int argc,char* argv[]){

  int iTimeInterval=0;
  int fdServer;
  pid_t pidClient;
  char cOperator;
  char *fi=NULL;
  char *fj=NULL;
  FILE *fpClientLog;
  pid_t pidConnectedServer;

  char tempExp[2]="2";

  if(argc != 5){
    fprintf(stderr,"Command-Line arguments failed.\n");
    fprintf(stderr,"USAGE : ./client -fi -fj -internal -operation\n");
    exit(0);
  }

  pidClient = getpid();
  sprintf(strClientLog,"Logs/c-%ld.log",(long)3);
  if(NULL == (fpClientLog = fopen(strClientLog,"w"))){
    fprintf(stderr,"FAILED TO CREATE CLIENT LOG FILE. Errno : %s\n",strerror(errno));
    exit(0);
  }

  /* COMMAND LINE KONTROLLERI SIMDILIK DOGRU GIBI DEVAM ET */

  cOperator = argv[4][1];
  fi=tempExp;
  fj=tempExp;
  iTimeInterval = atoi(&argv[3][1]);

  #ifdef DEBUG
    printf("# CLIENT COMMAND-LINE DEBUG\n");
    printf("--Fi file : %s\n",&argv[1][1]);
    printf("--Fj file : %s\n",&argv[2][1]);
    printf("--Tİme Interval : %d\n",iTimeInterval);
    printf("--Operator : %c\n",cOperator);
  #endif


  if(-1 == (fdServer = open(SERVER_FIFO_NAME,O_WRONLY))){
    fprintf(stderr,"Client[%ld] failed to connect server.\n",(long)pidClient);
    fprintf(fpClientLog,"Client[%ld] failed to connect server.\n",(long)pidClient);
    fclose(fpClientLog);
    exit(0);
  }

  write(fdServer,&pidClient,sizeof(pid_t));

  read(fdServer,&pidConnectedServer,sizeof(pid_t));
  printf("Client[%ld] connected SERVER[%ld].",(long)pidClient,(long)pidConnectedServer);













  return 0;
}
