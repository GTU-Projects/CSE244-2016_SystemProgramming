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
#include <time.h>
#include "occurance_list.h"
#include "hm.h"

#ifndef PATH_MAX
 #define PATH_MAX 255;
#endif

// sinyal gelince flag kullanilacak
static sig_atomic_t doneflag=0;

key_t shmKey = 941524;
int shmid;
int fdPipe[2]; // pipe -> file proc-dir proc arasinda tek pipe var
char **strFiles=NULL; // dosya pathleri
int inumFiles; // dosya sayisi
char **strDirs=NULL; // klasor adresleri
int inumDirs; // klasor sayisi
sem_t *sem_named=NULL; // named semafor -> dosyaya yazma esnasinda kullanilacak
sem_t *sem_shared=NULL; // 
const char *strWord=NULL; // aranacak kelimenin pointeri
hmThread_t *ths=NULL; // thread e gonderilecek parametre ve thread bilgileri
int totalOccNum; // toplam tekrar sayisi
struct sigaction sigact; // sigaction

long getTimeDif(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
}

int main(int argc,char *argv[]){

	int total=0;
	struct timeval startTime;
	struct timeval endTime;

	if(argc != 3){
		fprintf(stderr, "USAGE : ./exec DirName Word\n");
		exit(EXIT_FAILURE);
	}

	gettimeofday(&startTime,NULL);
	// sinyalleri set et
	sigact.sa_handler = sighandler;
	sigaction(SIGINT,&sigact,NULL);

	unlink(LOG_FILE_NAME);

	shmid = shmget(shmKey,sizeof(int),IPC_CREAT | 0600);
	int *shmData= shmat(shmid,NULL,0);
	if(shmData == (int*)-1){
		perror("shmat");
		exit(1);
	}
	*shmData=0;
	
	strWord = argv[2]; // aranacak kelime
	if(!doneflag) // sinyal yoksa aramaya basla
		total= findRec(argv[1],argv[2]);

	gettimeofday(&endTime,NULL);

	total = *shmData;
	printf("SEARCH COMPLETED IN %ld ms.\n",getTimeDif(startTime,endTime));
	printf("Found %d occurances.\n",total);

	unlink(FIFO_NAME);
	sem_unlink(SEM_NAME);
	sem_unlink(SEM_SHARED_NAME);
	return 0;
}

// signal handler
void sighandler(int signo){
	printf("#### SIGINT(^C) handled ####\n");
	doneflag=1;
}

int findRec(const char *dirPath,const char *word){

	int i;
	pid_t pidChild=-1;
	hmMsg_t *msMessage;
	int msqid;
	key_t key;
	int total=0;

	// pipelarin karsimamami icin pipe ac

	if(!doneflag){
		findContentOfDir(dirPath);
	}

#ifdef DEBUG
	fprintf(stdout,"[%ld] opened Directory : %s\n",(long)getpid(),dirPath);
	fprintf(stdout,"[%ld] found %d files and %d dirs in %s\n",(long)getpid(),inumFiles,inumDirs,dirPath);
#endif

	// once regularlar icin threadleri yolla isleme baslasinlar
	if(inumFiles>0){
		pthread_t pipeReadThread;
			
		msMessage = (hmMsg_t *)malloc(sizeof(hmMsg_t));
		msMessage->type=1;
		memset(msMessage->text,0,MESSAGE_SIZE);

		key = getpid();

		msqid = msgget(key,S_IRUSR | S_IWUSR | IPC_CREAT);
		if(msqid ==-1){
			perror("Failed to create message queue ");
			exit(1);
		}

		ths = (hmThread_t *)calloc(sizeof(hmThread_t),inumFiles);
		
		if(!doneflag){


			// her threadde tek tek acmak yerine bir defa acalim sonra threadler ordan ulassin
			int semStat=getnamed(SEM_NAME,&sem_named,1);
			#ifdef DEBUG
				fprintf(stdout,"Semaphore Open status  : %d\n",semStat);
			#endif

			for(i=0;i<inumFiles;++i){
				if(!doneflag){
					ths[i].strFilePath = strFiles[i];
					ths[i].word = word;
					pthread_create(&(ths[i].th),NULL,threadFindOcc,(void *)&ths[i]);
				}else break;
			}
			int j=0;
			int temp=0;
			for(j=0;j<i;++j){
				pthread_join((ths[j].th),NULL);
				msgrcv(msqid,msMessage,MESSAGE_SIZE,0,0);

				sscanf(msMessage->text,"%d",&temp);
				printf("Message %s\n",msMessage->text);
				total+=temp;
			}

			free(msMessage);
			msgctl(msqid,IPC_RMID,NULL); // message queue sil
			sem_close(sem_named); // semafor kapa
		}
	}


	// klasorler icin fork yapilacak
	if(!doneflag && inumDirs>0){

		char strPath[PATH_MAX];
		pid_t pidChild;
		
		for(i=0;i<inumDirs;++i){
			if(!doneflag){
				pidChild=fork();
				if(pidChild ==-1){
					perror("Fork error");
					exit(0);
				}
				if(pidChild==0){
					strcpy(strPath,strDirs[i]);
					sem_close(sem_named); // recursiveden once kapat semaforu
					freeAll(); // childler isleme girmeden once eski verileri silsinler
					if(!doneflag)
						findRec(strPath,word);
					exit(0);
				}
			}
		}
		// parent child procesleri beklicek
		// sinyal falan gelse bile tum cocuklar olmeden olmek YOOKKK
		while((pidChild = wait(NULL))!=-1){
			#ifdef DEBUG
				printf("[%ld] process dead.\n",(long)pidChild);
			#endif
		}
	}

	getnamed(SEM_SHARED_NAME,&sem_shared,1);
	sem_wait(sem_shared);
	int *shmData = shmat(shmid,NULL,0);
	*shmData = (*shmData)+total;

	shmdt(shmData);
	sem_post(sem_shared);
	sem_close(sem_shared);
	freeAll();
	return total;
}

// alinan yerleri geri ver
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


	if(ths != NULL){
		free(ths);
		ths=NULL;
	}
}

// verilen klasor icindeki dosya ve klasor bilgilerini kaydeder
int findContentOfDir(const char *dirPath){

	char strPath[PATH_MAX]; // buffer
	struct dirent * pDirentCurr;
	pid_t pid = getpid();
	DIR *pDir;

	inumFiles=0;
	inumDirs=0;

	if((pDir = opendir(dirPath)) ==NULL){
		perror("Dir open :");
		exit(EXIT_FAILURE);
	}

	// bir defa tara ve sayilarini ogren
	while(!doneflag && NULL != (pDirentCurr = readdir(pDir))){
		if(strcmp(pDirentCurr->d_name,".")!=0 && strcmp(pDirentCurr->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dirPath,pDirentCurr->d_name);
			#ifdef DEBUG
				//fprintf(stdout, "[%ld] %d. Elem is %s\n",(long)pid,inumFiles+inumDirs,strPath);
			#endif
			if(is_directory(strPath)){
				++inumDirs;
			}else // if not directory
			if(is_regfile(strPath)){
				++inumFiles;
			}
		}
	}

	// sinyal gelme durumlarinda buralari olabildigince atlariz
	// dinamic yerler ac
	rewinddir(pDir);
	if(!doneflag && inumFiles>0){
		strFiles = (char **)calloc(sizeof(char *),inumFiles);
	}
	else strFiles=NULL;

	if(!doneflag && inumDirs>0){
		strDirs = (char **)calloc(sizeof(char *),inumDirs);
	}
	else strDirs=NULL;


	int newFileNum =0;
	int newDirNum=0;

	// pathleri dinamic yerlere kaydet
	while(!doneflag && NULL != (pDirentCurr = readdir(pDir))){
		if(strcmp(pDirentCurr->d_name,".")!=0 && strcmp(pDirentCurr->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dirPath,pDirentCurr->d_name);
			if(is_regfile(strPath)){
				strFiles[newFileNum]=strdup(strPath);
				++newFileNum;
			}else if(is_directory(strPath)){
				strDirs[newDirNum]=strdup(strPath);
				++newDirNum;
			}
		}
	}

	closedir(pDir);
	pDir=NULL;
	pDirentCurr=NULL;
	return inumFiles+inumDirs; // toplam sayiyi return et
}



/*
** Bu fonksiyonu thread kullanacak ve kendisne verilen file icinde kelie ariyacak
** buldugu sonuclari ise yine kendine verilen pipe uzerinde kendi parent threadine yollicak
*/
void *threadFindOcc(void *args){

	hmThread_t *pArgs = (hmThread_t *)args;

	key_t key = getpid();
	int msqid;
	hmMsg_t *message;

	msqid = msgget(key,S_IRUSR|S_IWUSR);
	if(msqid == -1){
		perror("Failed to connect message queue ");
		exit(1);
	}

	message = (hmMsg_t *)malloc(sizeof(hmMsg_t));
	memset(message->text,0,MESSAGE_SIZE);
	message->type=1;

	occurance_t * occ;
	if(!doneflag){ // aramayi baslat
		occ = findOccurenceInRegular(pArgs->strFilePath,pArgs->word);
	}
	// sonuclari loga bas
	sem_wait(sem_named);
	printOccurancesToLog(LOG_FILE_NAME,occ);
	sem_post(sem_named);

	sprintf(message->text,"%d",occ->total);
	msgsnd(msqid,message,MESSAGE_SIZE,0);

	deleteOccurance(occ);
	free(occ);
	free(message);
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
// bu odev icin fd bir pipe tir
occurance_t* findOccurenceInRegular(const char* fileName,const char *word){

	char str[30];
	int fdFileToRead; /* okunacak dosya fildesi */
	char buf; /* tek karakter okumalik buffer */
	int i=0;
	int column=-1;
	int row=0;
	int found=0;
	int logCreated = 0;
	occurance_t *occ = malloc(sizeof(occurance_t));
	occ->head=NULL;

	pid_t tid = syscall(SYS_gettid);


	if((fdFileToRead = open(fileName,O_RDONLY)) == -1){
		fprintf(stderr," Failed open \"%s\" : %s ",fileName,strerror(errno));
		return NULL;
	}

	#ifdef DEBUG_FILE_READ
		fprintf(stdout,"[s%ld] thread searches in :%s\n",(long)tid,fileName);
	#endif

	occ->fileName = fileName;
	/* Karakter karakter ilerleyerek kelimeyi bul. Kelimenin tum karekterleri arka
	arkaya bulununca imleci geriye cek ve devam et. Tum eslesen kelimeleri bul
	*/


	while(!doneflag && read(fdFileToRead,&buf,sizeof(char))){
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
        
        addLastOccurance(occ,row,column);

        i=0;
      }
    }else{
      i=0;
    }
  }

  occ->total = found;
  /* dosyalarin kapatilmasi*/
  #ifdef DEBUG_FILE_READ
  	fprintf(stdout,"[e%ld] thread end of %s\n",(long)tid,fileName);
  #endif
  close(fdFileToRead);
  return occ;
}

/* BU FONKSIYONLAR DERS KITABINDAN ALINDI */
/* SOURCE :UnixTM Systems Programming: Communication, Concurrency, and Threads */
/* Chapter 14 - Semaphores */
int getnamed(char *name, sem_t **sem, int val) {
	while (((*sem = sem_open(name, FLAGS, PERMS, val)) == SEM_FAILED) &&
									(errno == EINTR)) ;
	if (*sem != SEM_FAILED)
		return 0;
	if (errno != EEXIST)
		return -1;
	while (((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR)) ;

	if (*sem != SEM_FAILED)
		return 0;

	return -1;
}

int destroynamed(char *name, sem_t *sem) {
	int error = 0;
	if (sem_close(sem) == -1)
		error = errno;
	if ((sem_unlink(name) != -1) && !error)
		return 0;
	if (error)
		/* set errno to first error that occurred */
		errno = error;
	return -1;
}
