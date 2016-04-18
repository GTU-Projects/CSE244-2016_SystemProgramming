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
char strConnectedServer[CHAR_MAX];
pid_t pidConnectedServer;
FILE *fpClientLog;

void sigHandler(int signalNo);

char *parseFile(const char* fileName);

int main(int argc,char* argv[]){

  int iTimeInterval=0;
  int fdServerWrite;
  int fdServerRead;
  pid_t pidClient;
  char cOperator;
  char *fi=NULL;
  char *fj=NULL;



  signal(SIGINT,sigHandler);

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
    printf("Client[%ld] started.\n",(long)pidClient);
    printf("--Fi file : %s\n",&argv[1][1]);
    printf("--Fj file : %s\n",&argv[2][1]);
    printf("--Tİme Interval : %d\n",iTimeInterval);
    printf("--Operator : %c\n",cOperator);
  #endif

  if(-1 == (fdServerWrite = open(SERVER_FIFO_NAME,O_WRONLY))){
    fprintf(stderr,"Client[%ld] failed to connect MainServer.\n",(long)pidClient);
    fprintf(fpClientLog,"Client[%ld] failed to connect MainServer.\n",(long)pidClient);
    fclose(fpClientLog);
    exit(0);
  }


  int iWriteCheck=0;
  // servere pidini yolla
  iWriteCheck = write(fdServerWrite,&pidClient,sizeof(pid_t));
  printf("Client[%ld] writed %d bytes.\n",(long)pidClient,iWriteCheck);
  close(fdServerWrite);

  // serverden yeni serverin pidini oku
  // bitaz bekle ma

  if(-1 == (fdServerRead = open(SERVER_FIFO_NAME,O_RDWR))){
    fprintf(stderr,"Client[%ld] failed to connect MainServer.\n",(long)pidClient);
    fprintf(fpClientLog,"Client[%ld] failed to connect MainServer.\n",(long)pidClient);
    fclose(fpClientLog);
    exit(0);
  }

  usleep(500); // biraz bekletki server tepki versin yoksa takılı kalır
  read(fdServerRead,&pidConnectedServer,sizeof(pid_t));
  sprintf(strConnectedServer,"Logs/%ld.sff",(long)pidConnectedServer);

  mkfifo(strConnectedServer,0666);
  if(-1 == (fdServerWrite = open(strConnectedServer,O_WRONLY))){
    fprintf(stderr,"Client[%ld] failed to connect MiniServerFifo : %s\n",(long)pidClient,strConnectedServer);
    fprintf(fpClientLog,"Client[%ld] failed to connect MiniServerFifo : %s\n",(long)pidClient,strConnectedServer);
    fclose(fpClientLog);
    exit(0);
  }

  printf("Client[%ld] connected MiniServer[%ld].\n",(long)pidClient,(long)pidConnectedServer);
  int a=3;
  write(fdServerWrite,&a,sizeof(int));

  printf("Printed %d\n",a);
  sleep(2); // test for sıgnals
  exit(0);

  return 0;
}

void sigHandler(int signalNo){

  kill(pidConnectedServer,SIGINT);

  printf("SIGINT HANDLED\n");
  fprintf(fpClientLog,"SIGINT HANDLED\n");
  fclose(fpClientLog);
  exit(signalNo);
}
