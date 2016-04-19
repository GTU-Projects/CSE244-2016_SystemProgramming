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

typedef enum {
  FALSE=0,TRUE=1
}bool;

typedef struct{
  int iFiSize;
  int iFjSize;
  int iTimeInterval;
  char cOperator;
}calculate_t;

char *cpFiName=NULL;
char *cpFjName=NULL;
char *cpFiContent=NULL;
char *cpFjContent=NULL;
char *cpOperator=NULL;
char cOperator;

calculate_t t_client;


char strClientLog[CHAR_MAX]; // max 127 in limits.h
char strConnectedServer[CHAR_MAX];
pid_t pidConnectedServer;
FILE *fpClientLog;


void sigHandler(int signalNo);

char giveOperator(const char * cpOperator);

void myExit(int exitStatus);
char *parseFile(const char* cpfileName);

int main(int argc,char* argv[]){

  int iTimeInterval=0;
  int fdServerWrite;
  int fdServerRead;
  pid_t pidClient;

  signal(SIGINT,sigHandler); // initialize signal handler

  // ################ Control line arguments ################## //
  if(argc != 5 ||argv[1][0] !='-' || argv[2][0] !='-' || argv[3][0] !='-'
          ||  argv[4][0] !='-'){
    fprintf(stderr,"Command-Line arguments failed.\n");
    fprintf(stderr,"USAGE : ./client -fi -fj -internal -operation\n");
    exit(0);
  }

  pidClient = getpid();
  // TODO : CHANGE CLIENT LOG NAME
  sprintf(strClientLog,"Logs/c-%ld.log",(long)3);
  if(NULL == (fpClientLog = fopen(strClientLog,"w"))){
    fprintf(stderr,"FAILED TO CREATE CLIENT LOG FILE. Errno : %s\n",strerror(errno));
    exit(0);
  }
  // ********** END OF Line Arguments control ************** //


  //  ############   Fi Fj control  ############  //
  cpFiName = &argv[1][1];
  cpFiContent = parseFile(cpFiName);
  if(NULL == cpFiContent){
    if(errno == ENOENT){ // dosya yoksa acilma hatasi ise
      fprintf(stderr,"Function create error. Errno : %s 'File-> %s'.",strerror(errno),cpFiName);
    }else {
      fprintf(stderr,"There is no function in %s.\n",cpFiName);
    }
    exit(0);
  }

  cpFjName = &argv[2][1];
  cpFjContent = parseFile(cpFjName);
  if(NULL == cpFjContent){
    if(errno == ENOENT){ // dosya yoksa acilma hatasi ise
      fprintf(stderr,"Function create error. Errno : %s 'File-> %s'.",
                                              strerror(errno),cpFjName);
    }else {
      fprintf(stderr,"There is no function in %s.\n",cpFjName);
    }
    exit(0);
  }
  // *********** END OF Fi Fj control ************//

  // ############ Time Reveal control ############ //

  iTimeInterval = atoi(&argv[3][1]);
  if(iTimeInterval<=0){
    fprintf(stderr,"Time Reveal must be bigger than zero.\n");
    myExit(0);
  }
  // *********** end of time reveal control ******** //

  // ############ Operator control ############ //

  cpOperator = argv[4];
  if(strlen(cpOperator)>1){
    cOperator = giveOperator(&cpOperator[1]);
    if(cOperator == '\0'){
      fprintf(stderr,"Undefined operator. Please use +,-,/,* operators.");
      myExit(0);
    }
  }else{
    fprintf(stderr,"Unentered Operator.\n");
    myExit(0);
  }

  // ********* END OF OPERATOR CONTROL *********//


  #ifdef DEBUG
    printf("# CLIENT COMMAND-LINE DEBUG\n");
    printf("Client[%ld] started.\n",(long)pidClient);
    printf("--Fi file : %s\n",cpFiName);
    printf("--Fj file : %s\n",cpFjName);
    printf("--Tİme Interval : %d\n",iTimeInterval);
    printf("--Operator : %c\n",cOperator);
  #endif

  if(-1 == (fdServerWrite = open(SERVER_FIFO_NAME,O_WRONLY))){
    fprintf(stderr,"Client[%ld] failed to connect MainServer.\n",(long)pidClient);
    fprintf(fpClientLog,"Client[%ld] failed to connect MainServer.\n",(long)pidClient);
    myExit(0); // close file in here
  }


  int iWriteCheck=0;
  // send client pid to server. Server will open fifo with client pid
  iWriteCheck = write(fdServerWrite,&pidClient,sizeof(pid_t));
  #ifdef DEBUG
  printf("Client[%ld] writed %d bytes.\n",(long)pidClient,iWriteCheck);
  #endif
  close(fdServerWrite);


  sprintf(strConnectedServer,"Logs/%ld.sff",(long)pidClient);

  /* If server didn't create fifo yet , open it */
  if(-1 == mkfifo(strConnectedServer,0666) && errno != EEXIST){
    fprintf(stderr, "Client-MiniServer mkfifo error. %s\n",strerror(errno));
    fprintf(fpClientLog, "Client-MiniServer mkfifo error. %s\n",strerror(errno));
    myExit(0);
  }
  // TODO : MKFIFO ERROR CHECKS
  if(-1 == (fdServerWrite = open(strConnectedServer,O_WRONLY))){
    fprintf(stderr,"Client[%ld] failed to connect MiniServerFifo : %s\n",
                                          (long)pidClient,strConnectedServer);
    fprintf(fpClientLog,"Client[%ld] failed to connect MiniServerFifo : %s\n",
                                          (long)pidClient,strConnectedServer);
    myExit(0);
  }

  //TODO : SERVER KENDI PIDINI YOLLASIN BURADAN GEREKINCE SERVERI KAPATIRSIN
  printf("Client[%ld] connected on %s.\n",(long)pidClient,strConnectedServer);

  // ################ SERVERE BILGI GONDERME CEVAP ALMA ################ //
  // TODO : send parameters to server and take result


  t_client.iFiSize = strlen(cpFiContent);
  t_client.iFjSize = strlen(cpFjContent);
  t_client.iTimeInterval = iTimeInterval;
  t_client.cOperator= cOperator;

  write(fdServerWrite,&t_client,sizeof(calculate_t));
  write(fdServerWrite,cpFiContent,sizeof(char)*(t_client.iFiSize));
  write(fdServerWrite,cpFjContent,sizeof(char)*(t_client.iFjSize));
  close(fdServerWrite);

/*
  mkfifo("a.f",0666);
  if(-1 == (fdServerRead = open("a.f",O_RDWR))){
    fprintf(stderr,"Client[%ld] failed to connect MiniServerFifo : %s\n",
                                          (long)pidClient,strConnectedServer);
    fprintf(fpClientLog,"Client[%ld] failed to connect MiniServerFifo : %s\n",
                                          (long)pidClient,strConnectedServer);
    myExit(0);
  }
*/


  double result=0;
  sleep(1);
  //mkfifo(strConnectedServer,0666);
  fdServerRead = open(strConnectedServer,O_RDWR);
  read(fdServerRead,&result,sizeof(double));
  printf("aa : %d\n",2);

  printf("Result = %.4f\n",result);

  myExit(EXIT_SUCCESS);
}

void sigHandler(int signalNo){

  cpFiName=NULL;
  cpFjName=NULL;

  if(cpFiContent!=NULL)
    free(cpFiContent);
  if(cpFjContent!=NULL)
    free(cpFjContent);

    //lo kapandıysa tekrardan acip icine yaz
  if(fpClientLog==NULL)
    fpClientLog=fopen(strClientLog,"w");
  cpOperator=NULL;
  unlink(strConnectedServer);
  printf("SIGINT HANDLED\n");
  fprintf(fpClientLog,"SIGINT HANDLED\n");
  fclose(fpClientLog);
  exit(signalNo);
}


char *parseFile(const char *cpFileName){

  FILE *fpFunctionFile;
  char *cpFunction =NULL;


  fpFunctionFile = fopen(cpFileName,"r");

  if(NULL == fpFunctionFile)
    return NULL; // errno set edildi, fonksiyonun return degerini kullaninca bak


    // test amacli bir array yolla
  cpFunction = (char *)calloc(sizeof(char),10);

  strcpy(cpFunction,"sin(t)");

  fclose(fpFunctionFile);
  return cpFunction;

}

// buraya gonderirken basinda tire olmadan gelmesi lazım
// daha sonra degisebilecegi icin char * alindi
char giveOperator(const char *cpOperator){

  signal(SIGINT,sigHandler);
  if(cpOperator==NULL || strlen(cpOperator)!=1)
    return '\0';


  char op = cpOperator[0];

  if(op =='+' || op=='-' || op=='/' || op=='*')
    return op;
  else return '\0';
}


void myExit(int exitStatus){

  signal(SIGINT,sigHandler);
  cpFiName=NULL;
  cpFjName=NULL;

  if(cpFiContent!=NULL)
    free(cpFiContent);
  if(cpFjContent!=NULL)
    free(cpFjContent);
  cpOperator=NULL;

  if(fpClientLog!=NULL)
    fclose(fpClientLog);
  unlink(strConnectedServer);

  exit(exitStatus);
}
