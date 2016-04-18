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


/* PREPROCESSORS DEFINE */

#define FILE_NAME_SIZE 255
#define LOG_FILE_NAME "integralGen.log"
#define MAIN_SERVER_FIFO_NAME "hmenn.ff"

#define RW_FLAG O_RDWR


/* GLOBAL DEGISKENLER */
FILE *fpLog =NULL;
pid_t pid;
pid_t *pPidClients = NULL;
int iMaxClient=0;
int iCurrentClientNumber=0;

void sigHandler(int signalNo);


int main(int argc,char *argv[]){
  int fdMainServerRead=0;
  pid_t pidChild;

  if(argc != 3 || argv[1][0]!='-' || argv[2][0]!='-'){
    fprintf(stderr,"Command-Line arguments failed.\n");
    fprintf(stderr,"USAGE: ./integralGen -resolution -max#OfClients\n");
    exit(0);
  }

  signal(SIGINT,sigHandler);
  fpLog = fopen(LOG_FILE_NAME,"a");
  if(NULL == fpLog){
    fprintf(stderr,"Failed to open %s. [Errno : %s]",LOG_FILE_NAME,strerror(errno));
    exit(0);
  }

  pid = getpid();

  // TODO : UNLINK EMEYI UNUTMA
  mkfifo(MAIN_SERVER_FIFO_NAME,0666);

  fdMainServerRead = open(MAIN_SERVER_FIFO_NAME,O_RDWR);
  if(fdMainServerRead == -1){
    fprintf(stderr,"Failed to open MainServer FIFO\n");
    fprintf(fpLog, "Failed to open MainServer FIFO\n");
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

  pPidClients = (pid_t*)calloc(sizeof(pid_t),iMaxClient);
  printf("Server[%ld] Started.\n",(long)pid);
  while(1){
    pid_t pidConnectedClient=0;
    printf("Main Server waits for clients.\n");
    if(0 !=read(fdMainServerRead,&pidConnectedClient,sizeof(pid_t) )){
      pPidClients[iCurrentClientNumber++]=pidConnectedClient;
      printf("Client[%ld] sent request.\n",(long)pidConnectedClient);
      if(-1 == (pidChild = fork())){
        fprintf(stderr, "Failed to fork operation.\n");
        fprintf(fpLog,"Failed to fork operation.\n");
        fclose(fpLog);
        //TODO : YASAYAN COCUKLARA OLMELERI ICIN SINYAL GONDERILECEK YOKSA ZOMBIE KALIR
        exit(0);
      }

      // child-server
      if(pidChild == 0){
        signal(SIGINT,sigHandler);
        pid_t pidChild;

        int fdMiniServerRead;

        //TODO :sonuclari buradan cliente yolla
        int fdMiniServerWrite;

        char strMiniFifoName[CHAR_MAX];
        pidChild=getpid();
        // client ile olusacak server buradan haberlesecek
        sprintf(strMiniFifoName,"Logs/%ld.sff",(long)pidChild);
        if(mkfifo(strMiniFifoName,0666) != 0){
          if(errno != EEXIST){
            fprintf(stderr,"Failed to open fifo : %s\n",strMiniFifoName);
            fprintf(fpLog,"Failed to open fifo : %s\n",strMiniFifoName);
            exit(1);
          }
        }
        if(-1 == (fdMiniServerRead = open(strMiniFifoName,O_RDWR))){
          fprintf(stderr, "Failed to open MiniServerFifo to read.\n");
          fprintf(fpLog, "Failed to open MiniServerFifo[%ld] to read.\n",(long)pidChild);
          exit(0);
        }
        printf("MiniServer[%ld] served Client[%ld] on %s\n",(long)pidChild,(long)pidConnectedClient,strMiniFifoName);
        int a=3;
        read(fdMiniServerRead,&a,sizeof(int));
        fprintf(stderr,"MiniServer Read : %d\n",a);
        exit(0);
      }else{

        int fdMainServerWrite=0;
        fdMainServerWrite = open(MAIN_SERVER_FIFO_NAME,O_WRONLY);
        if(-1 == fdMainServerWrite){
          fprintf(stderr, "Failed to open MainServerFifo to write.\n");
          fprintf(fpLog, "Failed to open MainServerFifo to write.\n");
          exit(0);
        }
        write(fdMainServerWrite,&pidChild,sizeof(pid_t));
        close(fdMainServerWrite);
        // main fifo uzerinden haberlesildigi icin parentin biraz beklemesi lazim
        // yoksa kendi yazdigi pidyi okuyup hata yapacak
        usleep(500);
      }
    }else break;
  }



  return 0;
}


void sigHandler(int signalNo){

  int i=0;
  for(i=0;i<iMaxClient;++i){
    printf("Killed %d.[%ld].\n",i,(long)pPidClients[i]);
    kill(pPidClients[i],SIGINT);
  }

  unlink(MAIN_SERVER_FIFO_NAME);
  printf("SIGINT HANDLED\n");
  fprintf(fpLog,"SIGINT HANDLED\n");
  fclose(fpLog);
  exit(signalNo);
}
