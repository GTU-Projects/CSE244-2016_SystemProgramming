#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include "hw1.h"



FILE * createLogFile(const char* searchedFileName){

  FILE * fpLogFile;
  int oldLogNumbers = searchInFile(LOG_FILE,searchedFileName,'l');

  fpLogFile = fopen(LOG_FILE,"a+");
  fprintf(fpLogFile,"\n-> %s ## %d ##\n",searchedFileName,oldLogNumbers+1);

  return fpLogFile;
}



int findOccurencesInLine(const char *line,int startPoint, const char *word){

  int indexOfLine=0;
  int columnNumber =0;
  int i=0;
  int j=0;

  for(i=startPoint;i<strlen(line);++i){
    indexOfLine=i;
    columnNumber=i;

    for(j=0;j<strlen(word);++j){
      if(line[indexOfLine]==word[j]){
        ++indexOfLine;
        if(j == strlen(word)-1){
          return columnNumber;
        }
      }else break;
    }
  }
  return -1;
}


int searchInFile(const char *fileName, const char *word, char logOrNormal){

  FILE *fpWordFile;
  FILE *fpLogFile;
  const unsigned char NEW_LINE = '\n';
  int lineLength=0;
  int lineNumber=0;
  int totalOccurence=0;
  char *line = NULL;
  char ch;
  int columnNumber;
  bool logCreated=false;

  if(NULL == (fpWordFile = fopen(fileName,"r"))){
    fprintf(stderr, "File \"%s\" can't found in directory.\n",fileName);
    return -1;
  }


  do{
    columnNumber=0;
    lineLength=0;
   /* Read char from file to count length of line*/
    for(ch = fgetc(fpWordFile);
        ch != NEW_LINE && ch != EOF;
        ch = fgetc(fpWordFile) , ++lineLength){
    }
    if(ch != EOF){
      // Go back head of line
      // -1 for read new line or eof character
      fseek(fpWordFile,-lineLength-1,SEEK_CUR);

      // +1 for end of string
      free(line);
      line = (char *) calloc(sizeof(char),lineLength+1);
      fgets(line,lineLength+1,fpWordFile);
      line[lineLength]='\0';


      // verilen strinin verilen konumdan itibaren word kontrolu yapılır
      // word bulunmayana kadar devam edilir.
     while( -1 != (columnNumber =findOccurencesInLine(line,columnNumber,word))){
        if(logOrNormal =='n'){
          if(!logCreated){
            fpLogFile = createLogFile(fileName);
            logCreated=true;
          }
          fprintf(fpLogFile,"%d. %s %d %d\n",totalOccurence+1,word,lineNumber,columnNumber);
        }
        ++totalOccurence;
        ++columnNumber;
      }
    }

    ++lineNumber;

    ch = fgetc(fpWordFile);
  }while(ch != EOF);

  if(logCreated)
    fclose(fpLogFile);

  free(line);
  fclose(fpWordFile);
//  fclose(fpLogFile);
  return totalOccurence;
}
