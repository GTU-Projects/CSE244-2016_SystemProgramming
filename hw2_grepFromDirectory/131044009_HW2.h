#ifndef HW2_131044009
#define HW2_131044009

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum{
  FALSE=0,TRUE=1
}bool;


int findOccurencesInFile(const char* fileName,const char *word);

#define FAIL -1
#define READ_FLAGS (O_RDONLY)



#endif
