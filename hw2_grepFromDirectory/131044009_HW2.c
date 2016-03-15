#include <stdio.h>
#include <string.h> //strerror
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "131044009_HW2.h"


int findOccurencesInFile(const char* fileName,const char *word){

  int fileToReadfd;
  char buf;
  int foundNum=0;
  int index=0;
  int equalCh=0;
  int i=0;
  int column=0;


  if((fileToReadfd = open(fileName,READ_FLAGS)) == FAIL){
    fprintf(stderr," Failed open \"%s\" : %s ",fileName,strerror(errno));
    return FAIL;
  }

  while(read(fileToReadfd,&buf,sizeof(char))){
    if(buf == word[0]){
      index=column;
      equalCh=1;
      for(i=1;i<strlen(word);++i){
        read(fileToReadfd,&buf,sizeof(char));
        if(buf == word[i]){
          ++equalCh;
        }else{
          break;
        }
      }
      if(equalCh == strlen(word)){
        ++foundNum;
        printf("equalCh : %d\n",equalCh);
        lseek(fileToReadfd,equalCh-1,SEEK_CUR);
        equalCh=0;
      }
    }
    ++column;
  }

  printf("index : %d - foundNum : %d \n",index,foundNum);





  close(fileToReadfd);

  return 0;
}
