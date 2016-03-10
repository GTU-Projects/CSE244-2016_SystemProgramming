#ifndef HW1_131044009
#define HW1_131044009

#define LOG_FILE "gff.log"

typedef enum{
  false=0,true=1
}bool;


int findOccurencesInLine(const char *line, int lineNumber, const char *word);

int searchInFile(const char *fileName, const char *word,char logOrNormal);
FILE * createLogFile(const char* searchedFileName);




#endif
