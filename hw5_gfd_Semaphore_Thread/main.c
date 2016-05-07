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
#include "hm.h"

#ifndef PATH_MAX
 #define PATH_MAX 255;
#endif

// sinyal gelince flag kullanilacak
static sig_atomic_t doneflag=0;

int fdPipe[2]; // pipe -> file proc-dir proc arasinda tek pipe var
sem_t sem_mutex; // pipe icin mutex
char **strFiles=NULL; // dosya pathleri
int inumFiles; // dosya sayisi
char **strDirs=NULL; // klasor adresleri
int inumDirs; // klasor sayisi
sem_t *sem_named=NULL; // named semafor -> fifo icin kullanilacak
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
	sigemptyset(&(sigact.sa_mask));
	sigact.sa_handler = sighandler;
	sigaction(SIGINT,&sigact,NULL);

	strWord = argv[2]; // aranacak kelime
	if(!doneflag) // sinyal yoksa aramaya basla
		total= findOccurence(argv[1],argv[2]);

	gettimeofday(&endTime,NULL);

	printf("SEARCH COMPLETED IN %ld ms.\n",getTimeDif(startTime,endTime));
	printf("Found %d occurances.\n",total);

	unlink(FIFO_NAME);
	return 0;
}

// signal handler
void sighandler(int signo){
	printf("#### SIGINT(^C) handled ####\n");
	doneflag=1;
}

// klasor icinde recursive olarak kelime ariyacak
int findOccurence(const char *dirname,const char *word){
	// semafor ac
	sem_unlink(SEM_NAME);
	getnamed(SEM_NAME,&sem_named,1);

	if(!doneflag){
		pthread_t reader;

		// thread fifodan bilgileri loga aktaracak
		pthread_create(&reader,NULL,threadRemoveFifo,NULL);

		// recursive aramaya basla
		findRec(dirname,word);

		// fifodan okuma bekleyen threade -1 yolla
		// thread sona geldigini anlicak ve join olacak
		int fdFifoWrite = open(FIFO_NAME,O_WRONLY | O_APPEND,0666);
		int endOfFifo = -1;
		write(fdFifoWrite,&endOfFifo,sizeof(int));

		pthread_join(reader,NULL);
	}

	// semaforu kapat
	destroynamed(SEM_NAME,sem_named);
	return totalOccNum;
}


int findRec(const char *dirPath,const char *word){

	int i;
	pid_t pidChild=-1;

	// pipelarin karsimamami icin pipe ac
	sem_init(&sem_mutex,0,1);
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
		pipe(fdPipe); //TODO : PIPE HATA KONTROL
		ths = (hmThread_t *)calloc(sizeof(hmThread_t),inumFiles);
		
		if(!doneflag){

			// bu thread pipe read ucunda kalacak ve gelen herseyi fifoya yonlendirecek
			pthread_create(&pipeReadThread,NULL,threadRemovePipe,NULL);

			for(i=0;i<inumFiles;++i){
				if(!doneflag){
					ths[i].strFilePath = strFiles[i];
					ths[i].word = word;
					pthread_create(&(ths[i].th),NULL,threadFindOcc,(void *)&ths[i]);
				}else break;
			}


			int j=0;
			for(j=0;j<i;++j){
				pthread_join((ths[j].th),NULL);
			}

			//tum threadler oldugune gore artık dinleyici pipe a tid -1 veer ve pipe threadi daha fazla beklemesin
			pid_t endOfPipe=-1;
			write(fdPipe[1],&endOfPipe,sizeof(pid_t));
			pthread_join(pipeReadThread,NULL);
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

	freeAll();
	return 0;
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

	// pipe icin kullanilan mutexi sil
	sem_destroy(&sem_mutex);

	if(ths != NULL){
		free(ths);
		ths=NULL;
	}

	//unlink(FIFO_NAME);
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


// thread kullanacak ve surekli fifodan bilgi alip loga basicak
void *threadRemoveFifo(void *arg){

	pid_t tid;
	int totalReaded=0;
	int fdFifoWrite;
	int strSize;
	char strPath[PATH_MAX];
	char strMessage[MESSAGE_MAX];
	int total=0;

	int fdLog = open(LOG_FILE_NAME,(O_WRONLY  | O_CREAT | O_TRUNC),0666);

	mkfifo(FIFO_NAME,0666);
	int fdFifoRead = open(FIFO_NAME,O_RDWR);

	sprintf(strMessage,"##### Search start for '%s'. #####\n",strWord);
	write(fdLog,strMessage,strlen(strMessage)*sizeof(char));

	// bir tane tid oku eger gecerli ise 2tane olacak sekilde koordinatlar okicak

  	// readler kac byte okudu kontrol et
	int a=0,b=0;
	while(!doneflag && (a=read(fdFifoRead,&strSize,sizeof(int)))>0 && strSize!=-1){
		(b=read(fdFifoRead,strPath,sizeof(char)*strSize));
		strPath[strSize]='\0';
		//printf("StrSize : %d a%d b%d - %s-\n",strSize,a,b,strPath);

		sprintf(strMessage,"\n## Path : %s. ##\n",strPath);
		write(fdLog,strMessage,sizeof(char)*strlen(strMessage));

		int row=0,col=0,i=0;

		// koordinatlari oku
		while(!doneflag && read(fdFifoRead,&row,sizeof(int))>0 && read(fdFifoRead,&col,sizeof(int))>0 ){
			++i;
			sprintf(strMessage,"%d -> Row : %d - Col : %d\n",i,row,col);

			if(row==-1){ // koorninat -1 ise o zaman farkli dosyanin inputu gelecek demektir
				--i;
				break;
			}
			write(fdLog,strMessage,sizeof(char)*strlen(strMessage));
		}
		sprintf(strMessage,"## Path Total : %d. ##\n",i);
		write(fdLog,strMessage,strlen(strMessage)*sizeof(char));
		total+=i;
	}

	totalOccNum=total;
	sprintf(strMessage,"\n##### TOTAL OCCURENCE NUMBER : %d. #####\n",totalOccNum);
	write(fdLog,strMessage,strlen(strMessage)*sizeof(char));

	if(doneflag==1){
		sprintf(strMessage,"\n##### SIGINT(^C) HANDLED #####\n");
		write(fdLog,strMessage,strlen(strMessage)*sizeof(char));
	}

	close(fdLog);
}

/*
** Bu fonksiyonu thread kullanacak ve kendisne verilen file icinde kelie ariyacak
** buldugu sonuclari ise yine kendine verilen pipe uzerinde kendi parent threadine yollicak
*/
void *threadFindOcc(void *args){

	hmThread_t *pArgs = (hmThread_t *)args;
	pArgs->tid = syscall(SYS_gettid);

	// tek pipe oldugu icin lock karismamasii sagliyalim
	sem_wait(&sem_mutex);
	#ifdef DEBUG
		//fprintf(stdout,"Thread[%ld] in Mutex - treadFindOcc.\n",(long)pArgs->tid);
	#endif

	if(!doneflag)
		write(fdPipe[1],&(pArgs->tid),sizeof(pid_t));

	int total=0;
	if(!doneflag) // aramayi baslat
		total = findOccurenceInRegular(fdPipe[1],pArgs->strFilePath,pArgs->word);

	int endOfPipe = -1;

	// burayi kesin yazsinki en azindan threadin oldugunu diger thread bilsin
	write(fdPipe[1],&endOfPipe,sizeof(int));
	write(fdPipe[1],&total,sizeof(int));

	sem_post(&sem_mutex); // unlock mutex

	// dosyada koordinatlari ara
	// kendi tid si ve daha sonra satir sutun olarak yaz
	// bittiyse -1 toplam olarak yaz ve join ol

	// tid row col
	//	   row col
	//     -1  total // bitis durumu

}

// thread bilgi dizisinden threadin indesini bul
int findThreadIndex(pid_t tid){

	int i=0;
	for(i=0;i<inumFiles;++i){
		if(ths[i].tid == tid)
			return i;
	}
	return -1;
}

// kendisine gelen toplam thread sayisi kadar tid -1 okuyana kadar pipe i bosalt
// tid -1 i threadler oldukten sonra main process basacak fifoya sona geldigini bildirmek icin
// okuduklarini fifoya yaz

// read : tid row col
//			  -1 total

// write : pathlen path row col
//                      -1 total
void *threadRemovePipe(void *arg){

	pid_t tid;
	int totalReaded=0;
	int fdFifoWrite;


	fdFifoWrite = open(FIFO_NAME,O_WRONLY,O_APPEND);

	// bir tane tid oku eger gecerli ise 2tane olacak sekilde koordinatlar okicak
	
	while(!doneflag && read(fdPipe[0],&tid,sizeof(pid_t))>0 && tid!=-1){

		int row=0,col=0;
		int index = findThreadIndex(tid);
		#ifdef DEBUG
			//fprintf(stdout,"Thread[%d][%ld] :  %s\n",index,(long)tid,ths[index].strFilePath);
		#endif


		// eger aranan filede bulunmadiysa bosuna loga basmayalim
		int sizeOfFileName = strlen(ths[index].strFilePath);

		read(fdPipe[0],&row,sizeof(int));
		read(fdPipe[0],&col,sizeof(int));

		sem_wait(sem_named);
		if(!doneflag && row!=-1){
			
			// fifoya bilgileri gonder
			int a = write(fdFifoWrite,&sizeOfFileName,sizeof(int));

			int b = write(fdFifoWrite,ths[index].strFilePath,strlen(ths[index].strFilePath));
			//printf("Size : %d - a%d - b%d %s-\n",sizeOfFileName,a,b,ths[index].strFilePath);
			write(fdFifoWrite,&row,sizeof(int));
			write(fdFifoWrite,&col,sizeof(int));

			while(!doneflag && read(fdPipe[0],&row,sizeof(int))>0 && read(fdPipe[0],&col,sizeof(int))>0){
				#ifdef DEBUG
					fprintf(stdout,"Pipe-Row: %d - Col: %d\n",row,col);
				#endif

				// fifoya yonlendirme yapalımm

				write(fdFifoWrite,&row,sizeof(int));
				write(fdFifoWrite,&col,sizeof(int));

				// bir tanesi yazildi demekki
				// yeni pid li veri cekmeye gec
				if(row==-1){
					totalReaded=col;
					break;
				}
			}if(doneflag){ // eger sinyal gelirse bitis mesaji yolla
				// sinyal gelirse koordinatlari -1 yolliyalimki bittigini anlasin
				int end=-1;
				write(fdFifoWrite,&end,sizeof(int));
				write(fdFifoWrite,&end,sizeof(int));
			}
		}
		sem_post(sem_named);
	}
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
int findOccurenceInRegular(int fd,const char* fileName,const char *word){

	char str[30];
	int fdFileToRead; /* okunacak dosya fildesi */
	char buf; /* tek karakter okumalik buffer */
	int i=0;
	int column=-1;
	int row=0;
	int found=0;
	int logCreated = 0;
	pid_t tid = syscall(SYS_gettid);


	if((fdFileToRead = open(fileName,O_RDONLY)) == -1){
		fprintf(stderr," Failed open \"%s\" : %s ",fileName,strerror(errno));
		return -1;
	}

	#ifdef DEBUG_FILE_READ
		fprintf(stdout,"[s%ld] thread searches in :%s\n",(long)tid,fileName);
	#endif
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
        write(fd,&row,sizeof(int));
        write(fd,&column,sizeof(int));
        i=0;
      }
    }else{
      i=0;
    }
  }

  /* dosyalarin kapatilmasi*/
  #ifdef DEBUG_FILE_READ
  	fprintf(stdout,"[e%ld] thread end of %s\n",(long)tid,fileName);
  #endif
  close(fdFileToRead);
  return found;
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
