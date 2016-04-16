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


/* PREPROCESSORS DEFINE */

#define FILE_NAME_SIZE 255
#define LOG_FILE_NAME "integralGen.log"
#define SERVER_FIFO_NAME "hmenn.ff"

#define RW_FLAG O_RDWR


/* GLOBAL DEGISKENLER */
FILE *fpLog =NULL;
pid_t pid;
pid_t *pPidClients = NULL;
int iMaxClient;

void sigHandler(int signalNo);


int main(int argc,char *argv[]){
  int fdFifo=0;
  pid_t pidChild;

  if(argc != 3 || argv[1][0]!='-' || argv[2][0]!='-'){
    fprintf(stderr,"Command-Line arguments failed.\n");
    fprintf(stderr,"USAGE: ./integralGen -resolution -max#OfClients\n");
    exit(0);
  }

  fpLog = fopen(LOG_FILE_NAME,"a");
  if(NULL == fpLog){
    fprintf(stderr,"Failed to open %s. [Errno : %s]",LOG_FILE_NAME,strerror(errno));
    exit(0);
  }

  pid = getpid();


  mkfifo(SERVER_FIFO_NAME,0666);

  fdFifo = open(SERVER_FIFO_NAME,RW_FLAG);
  if(fdFifo == -1){
    fprintf(stderr,"Failed to open Server FIFO\n");
    fprintf(fpLog, "Failed to open Server FIFO\n");
    fclose(fpLog);
    exit(0);
  }

  iMaxClient = atoi(&argv[2][1]);

  // TODO : ERROR CHECKS - ARGUMENT CONTROLS
  if(iMaxClient<=0){
    fprintf(stderr,"Max Client Number[%d] is invalid. Program aborted.",iMaxClient);
    fprintf(fpLog,"Max Client Number[%d] is invalid. Program aborted.",iMaxClient);
    fclose(fpLog);
    exit(0);
  }


  pid_t pidConnectedClient;
  while(0 != read(fdFifo,&pidConnectedClient,sizeof(pid_t))){

    printf("Client[%ld] connected.\n",(long)pidConnectedClient);

    if(-1 == (pidChild = fork())){
      fprintf(stderr, "Failed to fork operation.\n");
      fprintf(fpLog,"Failed to fork operation.\n");
      fclose(fpLog);
      //TODO : YASAYAN COCUKLARA OLMELERI ICIN SINYAL GONDERILECEK YOKSA ZOMBIE KALIR
    }





  }



  return 0;
}


void sigHandler(int signalNo){

  int i=0;
  for(i=0;i<iMaxClient;++i)
    kill(pPidClients[i],SIGINT);

  unlink(SERVER_FIFO_NAME);
  printf("SIGINT HANDLED\n");
  fprintf(fpLog,"SIGINT HANDLED FROM INTEGRALGEN\n");
  fclose(fpLog);
  exit(signalNo);
}
