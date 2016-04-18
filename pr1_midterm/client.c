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

char *cpFiName;
char *cpFjName;
char *cpFiContent=NULL;
char *cpFjContent=NULL;
char *cpOperator=NULL;
char cOperator;


char strClientLog[CHAR_MAX]; // max 127 in limits.h
char strConnectedServer[CHAR_MAX];
pid_t pidConnectedServer;
FILE *fpClientLog;


void sigHandler(int signalNo);

char giveOperator(const char * cpOperator);

char *parseFile(const char* cpfileName);

int main(int argc,char* argv[]){

  int iTimeInterval=0;
  int fdServerWrite;
  int fdServerRead;
  pid_t pidClient;

  signal(SIGINT,sigHandler);

  char tempExp[2]="2";

  if(argc != 5 ||argv[1][0] !='-' || argv[2][0] !='-' || argv[3][0] !='-'
          ||  argv[4][0] !='-'){
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

  // *****************  Fi Fj control ************ //
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

  // ******** Operator control */

  cpOperator = argv[4];
  if(strlen(cpOperator)>1){
    cOperator = giveOperator(&cpOperator[1]);
    if(cOperator == '\0'){
      fprintf(stderr,"Undefined operator. Please use +,-,/,* operators.");
      exit(0);
    }
  }else{
    fprintf(stderr,"Unentered Operator.\n");
    exit(0);
  }

  // ********* END OF OPERATOR CONTROL *********//
  iTimeInterval = atoi(&argv[3][1]);

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
  sleep(5); // test for sıgnals
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


char *parseFile(const char *cpFileName){

  FILE *fpFunctionFile;
  char *cpFunction =NULL;


  fpFunctionFile = fopen(cpFileName,"r");

  if(NULL == fpFunctionFile)
    return NULL; // errno set edildi, fonksiyonun return degerini kullaninca bak


    // test amacli bir array yolla
  cpFunction = (char *)calloc(sizeof(char),10);

  strcpy(cpFunction,"sin(t)");


  return cpFunction;

}

// buraya gonderirken basinda tire olmadan gelmesi lazım
// daha sonra degisebilecegi icin char * alindi
char giveOperator(const char *cpOperator){

  if(cpOperator==NULL || strlen(cpOperator)!=1)
    return '\0';


  char op = cpOperator[0];

  if(op =='+' || op=='-' || op=='/' || op=='*')
    return op;
  else return '\0';
}
