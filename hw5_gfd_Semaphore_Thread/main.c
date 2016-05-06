#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h> // strerror_r: thread_safe
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include "hm.h"

#ifndef PATH_MAX
 #define PATH_MAX 255;
#endif






int main(int argc,char *argv[]){


	int total=0;

	if(argc != 3){
		fprintf(stderr, "USAGE : ./exec DirName Word\n");
		exit(EXIT_FAILURE);
	}


	total= findOccurence(argv[1],argv[2]);

	printf("Total : %d\n",total);

	return 0;
}




int findOccurence(const char *dirname,const char *word){


	int fdLog = open(LOG_FILE_NAME,(O_WRONLY | O_CREAT),0666);
	// TODO : assert fopen



	int total = findRec(dirname,word,fdLog);

	close(fdLog);

	return total;
	// fifo ac 
	// fifoyu read edecek threadi olustur
	// read edip loga yazacak :D


	// metod bittikten sonra fifoya bittigini belirtmek icin path = -1 yaz

}


char **strFiles;

int findRec(const char *dirPath,const char *word,int fd){


	pid_t pidChild=-1;
	struct dirent *pDirent;
	DIR* pDir=NULL;
	
	int inumFiles=0;
	int inumDirs=0;



	if((pDir = findContentOfDir(dirPath,&inumFiles,&inumDirs))==NULL){
		fprintf(stderr,"Klasor icerik okuma hatasi.\n");
		exit(0);
	}

#ifdef DEBUG
	fprintf(stdout,"[%ld] opened Directory : %s\n",(long)getpid(),dirPath);
	fprintf(stdout,"[%ld] found %d dirs and %d files in %s\n",(long)getpid(),inumFiles,inumDirs,dirPath);
#endif


	


	// daha sonra hepsi icin ayrı thread ve mutex pipe ac
	// bir thread ise mutexi habire okumasi icin


	return 0;
}

DIR* findContentOfDir(const char *dirPath,int *pFileNum, int *pDirNum){

	char strPath[PATH_MAX];
	struct dirent * pDirentCurr;
	pid_t pid = getpid();
	DIR *pDir;


	*pFileNum=0;
	*pDirNum=0;

	if((pDir = opendir(dirPath)) ==NULL){
		perror("Dir open :");
		exit(EXIT_FAILURE);
	}

	while(NULL != (pDirentCurr = readdir(pDir))){
		if(strcmp(pDirentCurr->d_name,".")!=0 && strcmp(pDirentCurr->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dirPath,pDirentCurr->d_name);
			#ifdef DEBUG
				fprintf(stdout, "[%ld] %d. Elem is %s\n",(long)pid,(*pFileNum)+(*pDirNum),strPath);
			#endif
			if(is_directory(strPath)){
				++(*pDirNum);
			}else // if not directory 
			if(is_regfile(strPath)){
				++(pFileNum);
			}
		}
	}
	
	rewinddir(pDir);
	strFiles = (char **)calloc(sizeof(char *),*pFileNum);

	*pFileNum=0;

	while(NULL != (pDirentCurr = readdir(pDir))){
		if(strcmp(pDirentCurr->d_name,".")!=0 && strcmp(pDirentCurr->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dirPath,pDirentCurr->d_name);
			if(is_regfile(strPath)){
				strFiles[*pFileNum]=strdup(strPath);
				++(*pFileNum);
			}
		}
	}

	rewinddir(pDir);	
	pDirentCurr=NULL;
	return pDir;
}



void threadFindOcc(void *arg){

	// kendisine gelen argumandan pipe a yazacak ama mutex yapacakki karismasin
	// dosyada koordinatlari ara
	// kendi tid si ve daha sonra satir sutun olarak yaz
	// bittiyse -1 toplam olarak yaz ve join ol


	// tid row col
	//	   row col
	//     -1  total

}

void threadRemovePipe(void *arg){

	// kendisine gelen toplam thread sayisi kadar tid -1 okuyana kadar pipe i bosalt
	// okuduklarini fifoya yaz

	// read : tid row col
	//			  -1 total

	// write : pathlen path row col
	//                      -1 total
}


// regular dosyami kontrol et
int is_regfile(const char * fileName){
  struct stat statbuf;

  if(stat(fileName,&statbuf) == -1){
    return 0;
  }else{
    return S_ISREG(statbuf.st_mode);
  }
}

// klasormu kontrol et
int is_directory(const char *dirName){
  struct stat statbuf;
      if(stat(dirName   ,& statbuf) == -1){
    return 0;
  }
  else {
    return S_ISDIR(statbuf.st_mode);
  }
}