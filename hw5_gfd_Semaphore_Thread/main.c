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
#include <sys/syscall.h> // gettid
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

	mkfifo(FIFO_NAME,0666);
	int fdFifo = open(FIFO_NAME,O_RDWR);

	/*pthread_t reader;
	pthread_create(&reader,NULL,threadRemoveFifo,NULL);*/


	int total = findRec(dirname,word,fdFifo);


	//pthread_join(reader,NULL);

	close(fdLog);
	close(fdFifo);

	return total;
	// fifo ac 
	// fifoyu read edecek threadi olustur
	// read edip loga yazacak :D


	// metod bittikten sonra fifoya bittigini belirtmek icin path = -1 yaz

}


int fdPipe[2];
sem_t sem_mutex;
char **strFiles;
int inumFiles;
char **strDirs;
int inumDirs;
hmThread_t *ths; // threads and their arguments




int findRec(const char *dirPath,const char *word,int fd){

	int i;
	pid_t pidChild=-1;

	sem_init(&sem_mutex,0,1);
	findContentOfDir(dirPath);
	// TODO : RETURN DEGERINI KONTROL EDECEKSIN

	
#ifdef DEBUG
	fprintf(stdout,"[%ld] opened Directory : %s\n",(long)getpid(),dirPath);
	fprintf(stdout,"[%ld] found %d files and %d dirs in %s\n",(long)getpid(),inumFiles,inumDirs,dirPath);
#endif

	if(inumFiles>0){
		pthread_t pipeReadThread;
		pipe(fdPipe); //TODO : PIPE HATA KONTROL
		ths = (hmThread_t *)calloc(sizeof(hmThread_t),inumFiles);

		// bu thread pipe read ucunda kalacak ve gelen herseyi fifoya yonlendirecek
		pthread_create(&pipeReadThread,NULL,threadRemovePipe,NULL);
		
		for(i=0;i<inumFiles;++i){
			ths[i].strFilePath = strFiles[i];
			ths[i].word = word;
			pthread_create(&(ths[i].th),NULL,threadFindOcc,(void *)&ths[i]);
		}

		for(i=0;i<inumFiles;++i){
			pthread_join((ths[i].th),NULL);
		}

		// tum threadler oldugune gore artık pipe a tid -1 veer ve pipe threadi daha fazla beklemesin
		int endOfPipe=-1;
		write(fdPipe[1],&endOfPipe,sizeof(int));
		pthread_join(pipeReadThread,NULL);
	}

	
	freeAll();
	return 0;
}

void freeAll(){

	int i=0;

	// free and handle dangling pointers
	for(i=0;i<inumDirs;++i){
		free(strDirs[i]);
		strDirs[i]=NULL;
	}
	if(inumDirs>0){
		free(strDirs);
	}
	strDirs=NULL;


	// free and handle dangling pointers
	for(i=0;i<inumFiles;++i){
		free(strFiles[i]);
		strFiles[i]=NULL;
	}
	if(inumFiles>0){
		free(strFiles);
	}
	strFiles=NULL;
	
	sem_destroy(&sem_mutex);

	if(ths != NULL){
		free(ths);
		ths=NULL;
	}


}


int findContentOfDir(const char *dirPath){

	char strPath[PATH_MAX];
	struct dirent * pDirentCurr;
	pid_t pid = getpid();
	DIR *pDir;
	inumFiles=0;
	inumFiles=0;

	if((pDir = opendir(dirPath)) ==NULL){
		perror("Dir open :");
		exit(EXIT_FAILURE);
	}

	while(NULL != (pDirentCurr = readdir(pDir))){
		if(strcmp(pDirentCurr->d_name,".")!=0 && strcmp(pDirentCurr->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dirPath,pDirentCurr->d_name);
			#ifdef DEBUG
				fprintf(stdout, "[%ld] %d. Elem is %s\n",(long)pid,inumFiles+inumDirs,strPath);
			#endif
			if(is_directory(strPath)){
				++inumDirs;
			}else // if not directory 
			if(is_regfile(strPath)){
				++inumFiles;
			}
		}
	}
	
	rewinddir(pDir);
	if(inumFiles>0)
		strFiles = (char **)calloc(sizeof(char *),inumFiles);
	else strFiles=NULL;

	if(inumDirs>0)
		strDirs = (char **)calloc(sizeof(char *),inumDirs);
	else strDirs=NULL;


	inumFiles=0;
	inumDirs=0;
	while(NULL != (pDirentCurr = readdir(pDir))){
		if(strcmp(pDirentCurr->d_name,".")!=0 && strcmp(pDirentCurr->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dirPath,pDirentCurr->d_name);
			if(is_regfile(strPath)){
				strFiles[inumFiles]=strdup(strPath);
				++inumFiles;
			}else if(is_directory(strPath)){
				strDirs[inumDirs]=strdup(strPath);
				++inumDirs;
			}
		}
	}	


	closedir(pDir);
	pDir=NULL;	
	pDirentCurr=NULL;
	return inumFiles+inumDirs;
}



void *threadRemoveFifo(void *arg){

}

void *threadFindOcc(void *args){

	hmThread_t *pArgs = (hmThread_t *)args;
	pArgs->tid = syscall(SYS_gettid);
	#ifdef DEBUG
		fprintf(stdout,"Thread[%ld] in treadFindOcc.\n",(long)pArgs->tid);
	#endif
	
	sem_wait(&sem_mutex);
		
		write(fdPipe[1],&(pArgs->tid),sizeof(pid_t));
		int i=0;
		for(i=0;i<3;++i){
			write(fdPipe[1],&i,sizeof(int));
			write(fdPipe[1],&i,sizeof(int));
		}

		sleep(3);
		// TODO : BURADA ARAMA DEVREYE GIRECEK
		int endOfPipe = -1;

		// row col -1 girildi yani bu thread okumayi bitirdi
		write(fdPipe[1],&endOfPipe,sizeof(int));	 
		write(fdPipe[1],&endOfPipe,sizeof(int));
	sem_post(&sem_mutex);


	// dosyada koordinatlari ara
	// kendi tid si ve daha sonra satir sutun olarak yaz
	// bittiyse -1 toplam olarak yaz ve join ol


	// tid row col
	//	   row col
	//     -1  total

}

void *threadRemovePipe(void *arg){

	pid_t tid; 

	// bir tane tid oku eger gecerli ise 2tane olacak sekilde koordinatlar okicak
	while(read(fdPipe[0],&tid,sizeof(pid_t))>0 && tid!=-1){

		printf("tid : %ld\n",(long)tid);
		int row,col;
		while(read(fdPipe[0],&row,sizeof(int))>0 && read(fdPipe[0],&col,sizeof(int))>0){
			printf("\tr: %d - c:%d\n",row,col);
			if(row==-1 || col==-1)
				break;
		}
		

	}
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


// hw3 teki fonksiyonum
// dosya icinde kelimeni gectigi koordinatlari fd ye basar
int findOccurenceInRegular(int fd,const char* fileName,const char *word){

	char str[30];
	int fdFileToRead; /* okunacak dosya fildesi */
	char buf; /* tek karakter okumalik buffer */
	int i=0;
	int column=-1;
	int row=0;
	int found=0;
	int logCreated = 0;

	if((fdFileToRead = open(fileName,O_RDONLY)) == -1){
	fprintf(stderr," Failed open \"%s\" : %s ",fileName,strerror(errno));
	return -1;
	}

	#ifdef DEBUG_FILE_READ
	printf("[%ld]' thread searches in :%s\n",(long)getpid(),fileName);
	#endif
	/* Karakter karakter ilerleyerek kelimeyi bul. Kelimenin tum karekterleri arka
	arkaya bulununca imleci geriye cek ve devam et. Tum eslesen kelimeleri bul
	*/

	while(read(fdFileToRead,&buf,sizeof(char))){
	++column; /* hangi sutunda yer aliriz*/
	if(buf == '\n'){
	i=0;
	column=-1;
      ++row;
    }else if(buf == word[i]){
      ++i;
      if(i == strlen(word)){ /* kelime eslesti dosyaya yaz imleci geri al*/
        ++found;
        lseek(fdFileToRead,-i+1,SEEK_CUR);
        column =column - i+1;
        #ifdef DEBUG_FILE_READ
        	printf("%d. %d %d\n",found,row,column);
        #endif
        write(fd,&row,sizeof(int));
        write(fd,&column,sizeof(int));
        i=0;
      }
    }else{
      i=0;
    }
  }
  /* dosyalarin kapatilmasi*/
  close(fdFileToRead);
  return found;
}