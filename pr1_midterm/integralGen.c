/* GTU CSE 244 - 2016 System Programming Midterm */
/* HASAN MEN - 131044009 */
/* DATE : 22.4.2016 */
/* Teacher : Erkan Zergeroglu */
/*
/* PLEASE READ REPORT TO LOOK DETAILS */

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
#include <time.h>

/* THIS IS A FREE 3RD PART LIBRARY. WILL USE TO PARSE AND SOLVE EQUATIONS */
/* REF: https://github.com/codeplea/tinyexpr - 22.4.16 */
#include "tinyexpr.h"

/* PREPROCESSORS DEFINE */
#define MILLION 1000000L
#define MAX_FILE_NAME 50
#define LOG_FILE_NAME "integralGen.log"
#define MAIN_SERVER_FIFO_NAME "hmenn.ff"
#define ALARM_TIME 5

/* Struct to store information which, sended from client. */
typedef struct {
    double dTimeInterval;
    int iFiSize;
    int iFjSize;
    char cOperator;
} t_information;

/* Strcut to store results of integration */
typedef struct {
    double dResult;
    double dStartTime;
    double dEndTime;
} t_integralRes;


/* Global verialbles */
FILE *fpLog = NULL;
FILE *fpExpression=NULL;
/* main server log file */
pid_t pid;
pid_t *pPidClients = NULL;
int iMaxClient = 0;
int iCurrentClientNumber = 0;
char strFromClientFifo[MAX_FILE_NAME];
/* read fifo */
char strToClientFifo[MAX_FILE_NAME];
/* write fifo */
pid_t pidConnectedClient = 0;
t_information tInfClient;
/* read from client */
te_expr *expr = NULL; /* TINYEXPR TO SOLVE FUNCTIONS*/

/* actual expression to evaluate */
char *strExpression=NULL;

/* default equations */
char *strF1 = NULL;
char *strF2 = NULL;
char *strF3 = NULL;
char *strF4 = NULL;
char *strF5 = NULL;
char *strF6 = NULL;

/* client request files */
char strFiName[MAX_FILE_NAME];

char strFjName[MAX_FILE_NAME];

/* signal handler for MAIN SERVER  */
void sigHandler(int signalNo);

/* reads content of file and returs */
char *readFile(FILE *fp);

/* combine two expression with operator */
char *getExpression(char * fi,char *fj,char op);

/* read pre defined files and initialize expressions */
void readAllFiles(){

  fpExpression = fopen("f1.txt","r");
  if(fpExpression!=NULL){
  strF1 = readFile(fpExpression);
  if(fpExpression!=NULL)
    fclose(fpExpression);
  fpExpression=NULL;
  }

  fpExpression = fopen("f2.txt","r");
  if(fpExpression!=NULL){
  strF2 = readFile(fpExpression);
  if(fpExpression!=NULL)
    fclose(fpExpression);
  fpExpression=NULL;
  }

  fpExpression = fopen("f3.txt","r");
  if(fpExpression!=NULL){
  strF3 = readFile(fpExpression);
  if(fpExpression!=NULL)
    fclose(fpExpression);
  fpExpression=NULL;
  }

  fpExpression = fopen("f4.txt","r");
  if(fpExpression!=NULL){
  strF4 = readFile(fpExpression);
  if(fpExpression!=NULL)
    fclose(fpExpression);
  fpExpression=NULL;
  }

  fpExpression = fopen("f5.txt","r");
  if(fpExpression!=NULL){
  strF5 = readFile(fpExpression);
  if(fpExpression!=NULL)
    fclose(fpExpression);
  fpExpression=NULL;
  }

  fpExpression = fopen("f6.txt","r");
  if(fpExpression!=NULL){
  strF6 = readFile(fpExpression);
  if(fpExpression!=NULL)
    fclose(fpExpression);
  fpExpression=NULL;
  }
}

char * readFile(FILE *fpFile){

  if(fpFile==NULL)
    return NULL;

  char c;
  int i=0;
  while (fscanf(fpFile, "%c", &c)!=EOF)
    if(c!='\n')
        ++i;

  if(i==0)
    return NULL;

  rewind(fpFile);

  char * strContent = (char *)malloc(sizeof(char)*(i+1));
  i=0;
  while (fscanf(fpFile, "%c", &c)!=EOF)
    if(c!='\n')
        strContent[i++]=c;
  strContent[i]='\0';

  //printf(" %s \n",strContent);
  return strContent;
}

/* HANDLER FOR MINI SERVERS */
void sigHandlerMini(int signalNo);


/* Handler to catch SIGCHLD */
void sigDeadHandler(int signalNo);


void exitHmenn(int exitStatus); /* my exit function */

/* Returns different microsecond between two time*/
long double getdifMic(struct timeval *start, struct timeval *end) {
    return MILLION * (end->tv_sec - start->tv_sec) + end->tv_usec - start->tv_usec;
}

/* Returns different miliseconds between two time*/
long double getdifMil(struct timeval *start, struct timeval *end) {
    return 1000 * (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) / 1000.0;
}

long double getdifSec(struct timeval *start,struct timeval *end){
        return (end->tv_sec - start->tv_sec) +(end->tv_usec - start->tv_usec)/1000000.0;
}

/* This funciton takes a equation and solves it according to integral step*/
/* I used 3rd TINYEXPR to parse string and solve equaiton. */
/* Equation unknown varilable must b 't' */
/* @param str : un parsed equation
   /* @param up : upper bound of integral
   /* @param down : lower bound of integral
   /* @param step : integration step */
double calculateIntegration(char *str, double up, double down, double step) {

    double t = 0;
    double firstValue = 0;
    double lastValue = 0;
    double total = 0;
    /* Store variable names and pointers. */
    te_variable vars[] = {{"t", &t}};

    int err;
    /* Compile the expression with variables. */
    expr = te_compile(str, vars, 1, &err);
    double rate = (up - down) / step;
    double xi = rate; /* xi = x1 */

    if (expr) {
        int i = 1;
        total = 0;
        for (i = 1; i < step; ++i) {
            t = xi;
            total += te_eval(expr);
            xi += rate;
        }

        t = down;
        firstValue = te_eval(expr);
        t = up;
        lastValue = te_eval(expr);
        te_free(expr);
        expr = NULL;
    } else {
        //TODO : LOOK THERE AGAIN
        fprintf(stderr, "Parse error. Server[%ld] closed!\n", (long) getpid());
        kill(pidConnectedClient,SIGUSR2);
        exitHmenn(0);
    }
    double result = (up - down) * (firstValue + 2 * total + lastValue) / (2.0 * step);
    return result;
}

/* Main function */
int main(int argc, char *argv[]) {
    int fdMainServerRead = 0; /* main server fifo */
    double dResulution = 0.0; /* double resolutin time */
    pid_t pidChild;
    struct timeval tMainStart; /* times */
    struct timeval tMiniStart;
    struct timeval tClientReq;
    gettimeofday(&tMainStart, NULL);

    /* Command line arguments check */
    if (argc != 3 || argv[1][0] != '-' || argv[2][0] != '-') {
        fprintf(stderr, "Command-Line arguments failed.\n");
        fprintf(stderr, "USAGE: ./integralGen -resolution(ms) -max#OfClients\n");
        fprintf(stderr, "Sample USAGE: ./integralGen -5000 -15\n");
        exit(0);
    }
    /* initialize signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTSTP, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGQUIT, sigHandler);
    signal(SIGHUP, sigHandler);
    signal(SIGSEGV,sigHandler);
    signal(SIGCHLD, sigDeadHandler); /* when child server died, will catch */

    /* open main log file */
    fpLog = fopen(LOG_FILE_NAME, "w");
    if (NULL == fpLog) {
        fprintf(stderr, "Failed to open %s. [Errno : %s]", LOG_FILE_NAME, strerror(errno));
        exit(0);
    }

    pid = getpid();

    /*create main fifo */
    if (-1 == mkfifo(MAIN_SERVER_FIFO_NAME, 0666) && errno != EEXIST) {
        fprintf(stderr, "Main Server[%ld] mkfifo error. %s\n", (long) pid, strerror(errno));
        fprintf(fpLog, "Main Server[%ld] mkfifo error. %s\n", (long) pid, strerror(errno));
        exitHmenn(0);
    }

    /* open main server to read */
    fdMainServerRead = open(MAIN_SERVER_FIFO_NAME, O_RDWR);
    if (fdMainServerRead == -1) {
        fprintf(stderr, "Failed to open MainServer FIFO to read\n");
        fprintf(fpLog, "Failed to open MainServer FIFO to read\n");
        exitHmenn(0);
    }

    iMaxClient = atoi(&argv[2][1]);
    dResulution = atof(&argv[1][1]);

    if (iMaxClient <= 0) {
        fprintf(stderr, "Max Client Number : %d is invalid. Program aborted.", iMaxClient);
        fprintf(fpLog, "Max Client Number : %d is invalid. Program aborted.", iMaxClient);
        exitHmenn(0);
    }

    if (dResulution <= 0) {
        fprintf(stderr, "Resolution : %lf is invalid. Program aborted.\n", dResulution);
        fprintf(fpLog, "Resolution : %lf is invalid. Program aborted.\n", dResulution);
        exitHmenn(0);
    }

    readAllFiles();


    /* mini server pid array*/
    pPidClients = (pid_t *) calloc(sizeof(pid_t), iMaxClient);
    printf("Server[%ld] Started.\n", (long) pid);
    while (1) {
        printf("Main Server waits for clients.\n");
        if (0 <= read(fdMainServerRead, &pidConnectedClient, sizeof(pid_t))) {
            if (iCurrentClientNumber == iMaxClient) {
                fprintf(stderr,"All[%d] servers served. Please try later. Good Bye!\n", iCurrentClientNumber);
                kill(pidConnectedClient, SIGUSR1);
                continue;
            }
            /* collect connected clients */
            pPidClients[iCurrentClientNumber] = pidConnectedClient;
            printf("herehrerhere");
            ++iCurrentClientNumber;

            gettimeofday(&tClientReq,NULL);
            long double ldClientReq = getdifSec(&tMainStart,&tClientReq);
            printf("Client[%ld] sent request at [%.4Lf]sec.\n", (long) pidConnectedClient,ldClientReq);
            //fprintf(fpLog,"Client[%ld] sent request at [%.4Lf]sec.\n", (long) pidConnectedClient,ldClientReq);

            // create child servers
            if (-1 == (pidChild = fork())) {
                fprintf(stderr, "Failed to fork operation.\n");
                fprintf(fpLog, "Failed to fork operation.\n");
                exitHmenn(0);
            }

            /* child runsss */
            if (pidChild == 0) {
                gettimeofday(&tMiniStart, NULL);
                gettimeofday(&tClientReq, NULL);
                /* SET signals for childs */
                signal(SIGINT, sigHandlerMini);
                signal(SIGTSTP, sigHandlerMini);
                signal(SIGTERM, sigHandlerMini);
                signal(SIGQUIT, sigHandlerMini);
                signal(SIGHUP, sigHandlerMini);
                signal(SIGALRM, sigHandlerMini);
                signal(SIGSEGV,sigHandlerMini);
                pid_t pidChild;
                int fdMiniServerRead;

                int fdMiniServerWrite; // for send result to client

                time_t connected = time(NULL);
                pidChild = getpid();

                // create fifos to comminicate with client
                sprintf(strFromClientFifo, "Logs/%ld-cwsr", (long) pidConnectedClient);
                if (mkfifo(strFromClientFifo, 0666) != 0) {
                    if (errno != EEXIST) {
                        fprintf(stderr, "Failed to open fifo : %s\n", strFromClientFifo);
                        fprintf(fpLog, "Failed to open fifo : %s\n", strFromClientFifo);
                        exitHmenn(0);
                    }
                }

                if (-1 == (fdMiniServerRead = open(strFromClientFifo, O_RDWR))) {
                    fprintf(stderr, "Failed to open MiniServerFifo to read.\n");
                    fprintf(fpLog, "Failed to open MiniServerFifo[%ld] to read.\n", (long) pidChild);
                    exitHmenn(0);
                }
                printf("MiniServer[%2d-%ld] served Client[%ld] on %s\n",
                       iCurrentClientNumber, (long) pidChild, (long) pidConnectedClient,
                       strFromClientFifo);


                /* ######### READING INFORMATION FROM MINI SERVER FIFO ############# */
                t_information tInfClient;
                alarm(15);
                read(fdMiniServerRead, &tInfClient, sizeof(t_information));
                read(fdMiniServerRead, strFiName, tInfClient.iFiSize);
                read(fdMiniServerRead, strFjName, tInfClient.iFjSize);
                close(fdMiniServerRead);
                alarm(0);


                double result = 9.99;
                sprintf(strToClientFifo, "Logs/%ld-crsw", (long) pidConnectedClient);
                if (mkfifo(strToClientFifo, 0666) != 0) {
                    if (errno != EEXIST) {
                        fprintf(stderr, "Failed to open fifo : %s\n", strToClientFifo);
                        fprintf(fpLog, "Failed to open fifo : %s\n", strToClientFifo);
                        exitHmenn(0);
                    }
                }

                if (-1 == (fdMiniServerWrite = open(strToClientFifo, O_WRONLY))) {
                    fprintf(stderr, "Failed to open MiniServerFifo to write.\n");
                    fprintf(fpLog, "Failed to open MiniServerFifo[%ld] to write.\n", (long) pidChild);
                    exitHmenn(0);
                }

                /* send server pid to client*/
                write(fdMiniServerWrite, &pidChild, sizeof(pid_t));


#ifdef DEBUG
                fprintf(stdout,"MiniServer read Fi=%s\n",strFiName);
                fprintf(stdout,"MiniServer read Fj=%s\n",strFjName);
                fprintf(stdout,"MiniServer read iTimeInterval = %f\n",tInfClient.dTimeInterval);
                fprintf(stdout,"MiniServer read cOperator = %c\n",tInfClient.cOperator);
#endif

                if(getExpression(strFiName,strFjName,tInfClient.cOperator)==NULL)
                {
                  kill(pidConnectedClient,SIGUSR2);
                  exitHmenn(0);
                }

                long double timedif;
                timedif = getdifSec(&tMainStart, &tClientReq);
                printf("Client connected in %Lf miliseconds\n", timedif);
                double upper;
                double lower;
                lower = timedif;
                upper = lower + (tInfClient.dTimeInterval);
#ifdef DEBUG
                printf("Time Interval %f\n",tInfClient.dTimeInterval);
                printf("Resolution %f\n",dResulution);
                printf("Lower %f\n",lower);
                printf("Up %f\n",upper);
#endif

          /* RESOLUTIONU NERDE KULLANACAK ANLAYAMADIM */
                int h = 0;
                t_integralRes pIntegoutput;
                while (1) {
                    result = calculateIntegration(strExpression, upper, lower, 3);
                    /* catch seg. faults */
                    if(isnanl(result) || isinfl(result)){
                      kill(pidConnectedClient,SIGSEGV);
                      kill(getpid(),SIGSEGV);
                    }
                    pIntegoutput.dStartTime = lower;
                    pIntegoutput.dEndTime = upper;
                    pIntegoutput.dResult = result;

                    /* write result to client */
                    write(fdMiniServerWrite, &pIntegoutput, sizeof(t_integralRes));

                    lower = upper;
                    upper = upper + tInfClient.dTimeInterval;

                    sleep(tInfClient.dTimeInterval);
                }

                close(fdMiniServerWrite);
                exitHmenn(0);
            } else {
                // PARENT WILL WAIT FOR NEW CLIENTS
            }
        }/* end of client connection read */
    }

    unlink(MAIN_SERVER_FIFO_NAME);
    printf("Exit Here");
    return 0;
}

/* this method exits process and remove all traces */
void exitHmenn(int exitStatus) {

    if(pPidClients!=NULL){
      free(pPidClients);
      pPidClients = NULL;
    }

    if(strExpression!=NULL){
      free(strExpression);
      strExpression=NULL;
    }

    if(fpExpression!=NULL){
      fclose(fpExpression);
      fpExpression=NULL;
    }

    if (expr != NULL)
        te_free(expr);
    expr = NULL;

    if(fpLog!=NULL){
      fclose(fpLog);
      fpLog=NULL;
    }

    if(strF1 != NULL){
      free(strF1);
      strF1=NULL;
    }
    if(strF2 != NULL){
      free(strF2);
      strF2=NULL;
    }
    if(strF3 != NULL){
      free(strF3);
      strF3=NULL;
    }
    if(strF4 != NULL){
      free(strF4);
      strF4=NULL;
    }
    if(strF5 != NULL){
      free(strF5);
      strF5=NULL;
    }
    if(strF6 != NULL){
      free(strF6);
      strF6=NULL;
    }
    exit(exitStatus);
}

/* signal handler for main process */
void sigHandler(int signalNo) {

    int i = 0;

    for (i = 0; i < iCurrentClientNumber; ++i) {
        kill(pPidClients[i], signalNo);
    }

    unlink(MAIN_SERVER_FIFO_NAME); /* delete main fifo */
    if (signalNo == SIGINT) {
        printf("CTRL-C Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
        fprintf(fpLog, "CTRL-C Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
    } else if (signalNo == SIGHUP) {
        printf("SIGUP Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
        fprintf(fpLog, "SIGUP Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
    } else if (signalNo == SIGTERM) {
        printf("SIGTERM Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
        fprintf(fpLog, "SIGTERM Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
    } else if (signalNo == SIGQUIT) {
        printf("SIGQUIT Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
        fprintf(fpLog, "SIGQUIT Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
    } else if (signalNo == SIGTSTP) {
        printf("SIGTSTP Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
        fprintf(fpLog, "SIGTSTP Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
    }else if (signalNo == SIGSEGV) {
        printf("Calculate Expressin failed.SIGSEGV Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
        fprintf(fpLog, "Calculate Expressin failed.SIGSEGV Signal Handled. Main Server[%ld] closed.\n", (long) getpid());
    }
    fclose(fpLog);
    fpLog=NULL;

    exitHmenn(signalNo);
}

/* mini server signal handler */
void sigHandlerMini(int signalNo) {

    unlink(strFromClientFifo);
    unlink(strToClientFifo);
    if (signalNo == SIGINT) {
        kill(pidConnectedClient, SIGINT);
        printf("CTRL-C Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
        fprintf(fpLog, "CTRL-C Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
    } else if (signalNo == SIGTSTP) {
        kill(pidConnectedClient, SIGTSTP);
        printf("CTRL-Z Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
        fprintf(fpLog, "CTRL-Z Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
    } else if (signalNo == SIGTERM) {
        kill(pidConnectedClient, SIGTERM);
        printf("SIGTERM Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
        fprintf(fpLog, "SIGTERM Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
    } else if (signalNo == SIGQUIT) {
        kill(pidConnectedClient, SIGQUIT);
        printf("SIGQUIT Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
        fprintf(fpLog, "SIGQUIT Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
    } else if (signalNo == SIGHUP) {
        kill(pidConnectedClient, SIGHUP);
        printf("SIGHUP Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
        fprintf(fpLog, "SIGHUP Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
    } else if (signalNo == SIGALRM) {
        kill(pidConnectedClient, SIGALRM);
        printf("SIGALRM[%d sec.] Signal Handled. Mini Server[%ld][%ld] closed.\n", ALARM_TIME, (long) getppid(),
               (long) getpid());
        fprintf(fpLog, "SIGALRM[%d sec.] Signal Handled. Mini Server[%ld][%ld] closed.\n", ALARM_TIME, (long) getppid(),
                (long) getpid());
    }else if (signalNo == SIGSEGV) {
        kill(pidConnectedClient, SIGSEGV);
        printf("Calculate Expressin failed.SIGSEGV Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
        fprintf(fpLog, "Calculate Expressin failed.SIGSEGV Signal Handled. Mini Server[%ld][%ld] closed.\n", (long) getppid(), (long) getpid());
    }

    fclose(fpLog);
    fpLog=NULL;
    exitHmenn(signalNo);
}

/* when child dead will send SIGCHLD to paranet*/
void sigDeadHandler(int signalNo) {
    //printf("Hererere\n");
    --iCurrentClientNumber;
    pid_t child = wait(NULL);
    //exitHmenn(signalNo);
}

/* this function takes 2file names and if its valid. Combines two expression
according to operator */
char *getExpression(char * fi,char *fj,char op){

  if(fi==NULL || fj==NULL)
    return NULL;

    /* (fi)*(fj) */
    char *content1=NULL;
    char *content2=NULL;

  if(strcmp(fi,"f1.txt")==0){
    content1=strF1;
  }else if(strcmp(fi,"f1.txt")==0){
    content1=strF2;
  }else if(strcmp(fi,"f3.txt")==0){
    content1=strF3;
  }else if(strcmp(fi,"f4.txt")==0){
    content1=strF4;
  }else if(strcmp(fi,"f5.txt")==0){
    content1=strF5;
  }else if(strcmp(fi,"f6.txt")==0){
    content1=strF6;
  }else{
    free(strExpression);
    strExpression=NULL;
    return NULL;
  }

  if(strcmp(fj,"f1.txt")==0){
    content2=strF1;
  }else if(strcmp(fj,"f2.txt")==0){
    content2=strF2;
  }else if(strcmp(fj,"f3.txt")==0){
    content2=strF3;
  }else if(strcmp(fj,"f4.txt")==0){
    content2=strF4;
  }else if(strcmp(fj,"f5.txt")==0){
    content2=strF5;
  }else if(strcmp(fj,"f6.txt")==0){
    content2=strF6;
  }else{
    free(strExpression);
    strExpression=NULL;
    return NULL;
  }

  int size = strlen(content1)+strlen(content2)+6;

  /* dynamic allocation */
  strExpression = (char *)malloc(sizeof(char)*size);
  strcpy(strExpression,"(");
  strcat(strExpression,content1);
  strcat(strExpression,")");
  strncat(strExpression,&op,1);
  strcat(strExpression,"(");
  strcat(strExpression,content2);
  strcat(strExpression,")");
  strExpression[size-1]='\0';
  #ifdef DEBUG
  printf("SIZE : %d\n",size);
  printf("Expression : %s\n",strExpression);
  #endif
  return strExpression;
}
