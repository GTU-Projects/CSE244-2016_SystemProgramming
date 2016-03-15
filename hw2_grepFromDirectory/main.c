#include <stdio.h>
#include <string.h>
#include "131044009_HW2.h"


int main(int argc,char *argv[]){

  if(argc != 3){
    fprintf(stderr,"Usage : %s DirectoryName \"string\" ",argv[0]);
    return 1;
  }

  findOccurencesInFile("words.txt","ece");






  return 0;

}
