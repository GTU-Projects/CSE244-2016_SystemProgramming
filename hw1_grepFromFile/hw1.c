#include <stdio.h>
#include "hw1.h"


int main(int argc,char *argv[])
{

  if(argc<2){
    fprintf(stderr, "Usage: %s filename word\n",argv[0]);
    return -1;
  }




  printf("Total occurence : %d times",searchInFile(argv[1],argv[2],'n'));

  return 0;
}
