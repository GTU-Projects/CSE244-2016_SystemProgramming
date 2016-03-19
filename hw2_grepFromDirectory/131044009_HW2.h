#ifndef HW2_131044009
#define HW2_131044009

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum{
  FALSE=0,TRUE=1
}bool;


#ifndef PATH_MAX
#define PATH_MAX 255
#endif



int searchDir(const char *dirPath,const char *word);
bool isDirectory(const char *path);
int addLog(const char* path,const char* fileName);
bool isCharacterSpecialFile(const char *path);

int findOccurencesInFile(const char* fileName,const char *word);
char *getStringOfNumber(long number);
#define FAIL -1
#define SUCCESS 0
#define CHILD_PROCESS 0
#define READ_FLAGS (O_RDONLY)
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define FD_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)



#endif
