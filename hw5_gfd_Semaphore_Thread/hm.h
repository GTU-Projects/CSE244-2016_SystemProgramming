#ifndef HM
 #define HM 0


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>



int findOccurance(const char *dirname,const char *word);

int findRec(const char *dirname,const char *word);

void threadFindOcc(void *arg);

void threadRemovePipe(void *arg);

#endif