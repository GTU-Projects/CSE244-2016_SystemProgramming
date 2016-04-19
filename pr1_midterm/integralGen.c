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


typedef struct{
  int iFiSize;
  int iFjSize;
  int iTimeInterval;
  char cOperator;
}calculate_t;

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
char *cpFiContent=NULL;
char *cpFjContent=NULL;
calculate_t t_client;

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


      pPidClients[iCurrentClientNumber]=pidConnectedClient;
      ++iCurrentClientNumber;
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

        // client ile haberlesmek icin client pid ile fifo ac
        sprintf(strMiniFifoName,"Logs/%ld.sff",(long)pidConnectedClient);
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
        printf("MiniServer[%2d] served Client[%ld] on %s\n",iCurrentClientNumber,(long)pidConnectedClient,strMiniFifoName);


        // ############ READING INFORMATION FROM SERVER ############# //
        int iSizeOfFi=0;
        int iSizeOfFj=0;
        int iTimeInterval=0;
        char cOperator;


        read(fdMiniServerRead,&t_client,sizeof(calculate_t));
        cpFiContent = (char *)calloc(sizeof(char),t_client.iFiSize+1);
        cpFiContent[0]='\0';
        cpFjContent = (char *)calloc(sizeof(char),t_client.iFjSize+1);
        cpFjContent[0]='\0';
        read(fdMiniServerRead,cpFiContent,sizeof(char)*(t_client.iFiSize));
        cpFiContent[t_client.iFiSize]='\0';
        read(fdMiniServerRead,cpFjContent,sizeof(char)*(t_client.iFjSize));
        cpFjContent[t_client.iFjSize]='\0';
        close(fdMiniServerRead);



        double result=9.99;

      /*  mkfifo("a.f",0666);
        if(-1 == (fdMiniServerWrite = open("a.f",O_WRONLY))){
          fprintf(stderr, "Failed to open MiniServerFifo to write.\n");
          fprintf(fpLog, "Failed to open MiniServerFifo[%ld] to write.\n",(long)pidChild);
          exit(0);
        }*/

        mkfifo(strMiniFifoName,0666);        
        fdMiniServerWrite = open(strMiniFifoName,O_WRONLY);
        write(fdMiniServerWrite,&result,sizeof(double));
        close(fdMiniServerWrite);

        #ifdef DEBUG
        fprintf(stdout,"MiniServer read  fiSize = %d and Fi=%s\n",t_client.iFiSize,cpFiContent);
        fprintf(stdout,"MiniServer read  fjSize = %d and Fj=%s\n",t_client.iFjSize,cpFjContent);
        fprintf(stdout,"MiniServer read  iTimeInterval = %d\n",t_client.iTimeInterval);
        fprintf(stdout,"MiniServer read  cOperator = %c\n",t_client.cOperator);
        #endif

        printf("MiniServer ended.\n");

        //TODO : ADD MY EXIT HERE
        exit(0);
      }else{
        //exit(1);
        // TODO : PARENT YENI CLIENTLER ICIN BEKLEME DURUMUNA GECECEK
      }
    }else break;
  }


  unlink(MAIN_SERVER_FIFO_NAME);
  return 0;
}


void sigHandler(int signalNo){

  int i=0;
  for(i=0;i<iMaxClient;++i){
    printf("Killed %d.[%ld].\n",i,(long)pPidClients[i]);
    kill(pPidClients[i],SIGINT);
  }

  printf("asdasd");
  unlink(MAIN_SERVER_FIFO_NAME);
  printf("SIGINT HANDLED\n");
  fprintf(fpLog,"SIGINT HANDLED\n");
  fclose(fpLog);
  exit(signalNo);
}
