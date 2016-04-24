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
#include "hw4.h"

/* global degiskenler */
t_child *pTChilds=NULL;  // fork ile olusturulalar
DIR *pDir_current=NULL; // aktif klasor pointeri
struct dirent * pDirent_current=NULL;

pthread_t * pTh_thread=NULL; // olsuturulan threadler
t_search *allSearch=NULL; // her dosya icin bilgi saklar

char **strDirectories=NULL; // tum klasor adresleri store edildi threadler icin
char **strFiles=NULL; // tum dosya sayilari threadler icin store edildi

int iFile_num=0; // bir klasordeki dosya sayisi
int iDir_num=0; // bir klasordeki ic klasor sayisi

// dizi icindeki id lere gore pipe olusturur
int openFifoConnection(t_child *arr,int size,int drStatus){

  char fifoName[MAX_FILE_NAME];

  if(NULL == arr || size<=0 || drStatus<0 || drStatus >=size )
    return 0;

    /* childin pid si ve status ile acacak*/
 	pid_t pid = getpid();
 	sprintf(fifoName,"%ld-%d.ff",(long)pid,drStatus);

	#ifdef DEBUG
	 printf("Fifo : %s created.\n",fifoName);
	#endif
	  if(-1 == mkfifo(fifoName,FIFO_PERMS)){
	    if(errno != EEXIST){
	      fprintf(stderr,"FIFO ERROR : %s",strerror(errno));
	      exit(1);
	    }
	  }
  return 1;
}




int main(int argc,char *argv[]){

	// arguman kontrol
	if(argc!=3){
		fprintf(stderr,"Main arguments error.\n");
		fprintf(stderr,"USAGE : ./gfdT dirname word");
		exit(EXIT_FAILURE);
	}


	int iTotal_wordnum = search_dir(argv[1],argv[2]);

	printf("------------------------------------------\n");
	printf("Found %d times %s is %s dir.\n",iTotal_wordnum,argv[2],argv[1]);



	return 0;
}


int search_dir( char *dir_path, char *word){

	int fildes_man;
	int total=0;
	int fd;

	//pFile_log = fopen(LOG_FILE_NAME,"w");

	
	fd = open("hmenn.log",(O_WRONLY | O_CREAT),FD_MODE);
	total = search_dir_recursive(dir_path,word,fd);
	close(fd);
	

	freeAll();
	return total;
}




int search_dir_recursive( char *dir_path, char *word,int fd){

	pid_t pid_child=-1;
	pid_t pid_dead_child=-1;
	int iTotal_wordnum=0; // toplam bulunan iterasyon sayisi
	int fdStatus = -1; // current dirrectory and file status
	int drStatus = -1;
	char strPath[PATH_MAX];



	if(NULL == (pDir_current = opendir(dir_path))){
		fprintf(stderr, "[%ld] %s : %s\n",(long)getpid(),DIROPEN_LOG_ERROR,strerror(errno));
		exit_hmenn(1);
	}

#ifdef DEBUG
	fprintf(stdout,"[%ld] opened Directory : %s\n",(long)getpid(),dir_path);
#endif

	if(-1 == find_numof_elems_in_dir(dir_path,&iFile_num,&iDir_num)){
		// TODO: ADD ERROR CHECK
		exit_hmenn(1);
	}
#ifdef DEBUG
		fprintf(stdout,"[%ld] found %d dir and %d file in %s\n",(long)getpid(),
			iDir_num,iFile_num,dir_path);
#endif

	pTChilds = calloc(sizeof(t_child),iDir_num);
	allSearch = (t_search *)calloc(sizeof(t_search),iFile_num);

	//pipe between process - thread
	open_pipe_connection(allSearch,iFile_num);
	rewinddir(pDir_current);

	char strFifoName[MAX_FILE_NAME];
	int tempFD;
	while(NULL != (pDirent_current = readdir(pDir_current))){
		if(strcmp(pDirent_current->d_name,".")!=0 && strcmp(pDirent_current->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dir_path,pDirent_current->d_name);

			// HER DOSYAYI BIRER THREAD OKUYACAK VE KAYDEDECEK LOGA
			if(is_regfile(strPath)){
				++fdStatus;
				allSearch[fdStatus].filename = strFiles[fdStatus];
				allSearch[fdStatus].word = word;
				allSearch[fdStatus].status = fdStatus;
				//printf("File: %s \n",allSearch[fdStatus].filename);
				pthread_create(&(allSearch[fdStatus].tid),NULL,search_with_thread,&allSearch[fdStatus]);

				//  THREAD PARENTE KATILACAK
			}else if(is_directory(strPath)){
				
				++drStatus;
				openFifoConnection(pTChilds,iDir_num,drStatus);
				// dosyalar icin fork et
				if(-1 == (pid_child = fork())){
					fprintf(stderr,"Fork error.");
					exit_hmenn(0);
				}

				if(0 == pid_child){ // asagindan devammmm
					break;
				}
				else{
					// read icin parent fifoyu acarr
					sprintf(strFifoName,"%ld-%d.ff",(long)getpid(),drStatus);
					pTChilds[drStatus].fd[0]= open(strFifoName,O_RDONLY);
					pTChilds[drStatus].pid=pid_child;
					pTChilds[drStatus].status=drStatus;
				}
			}
		}
	}

	// child devamm
	if(pid_child == 0){

		pid_t temp;
		// fifoyu write modunda ac
		sprintf(strFifoName,"%ld-%d.ff",(long)getppid(),drStatus);
		pTChilds[drStatus].fd[1] = open(strFifoName,O_WRONLY);
		if(pTChilds[drStatus].fd[1] == -1) { // hata kontrol
	      	fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
	             (long)getpid(), strFifoName, strerror(errno));
	     	exit(1);
     	}
     	// recursive cagri icin directory processin fd sini kaydet
     	// yeni recursive parentin fd sine yazacak
		temp = pTChilds[drStatus].fd[1];
		freeAll(); // kullanılanları geri ve
		search_dir_recursive(strPath,word,temp); // recursive cagri
		close(temp);
		unlink(strFifoName);	
		exit_hmenn(0); //
	}else // parent here
	{
		pid_t pid_dead;
		// once kendi threadlerinde veri oku
		int i=0;
		for(i=0;i<=fdStatus;++i){	
			pthread_join(allSearch[i].tid,NULL);
			close(allSearch[i].fd[1]);
			char str[255];
			sprintf(str,"FILE : %s - Total :%d\n",allSearch[i].filename,allSearch[i].total);
	        iTotal_wordnum += allSearch[i].total;
	        write(fd,&str,strlen(str)*sizeof(char));
	        copyfile3(allSearch[i].fd[0],fd);
	        close(allSearch[i].fd[0]);	        
			printf("Thread %d dead.\n",i);
		}


		while(-1 != (pid_dead = wait(NULL))){
			char strFifoName[MAX_FILE_NAME];
			int id = getID(pTChilds,iDir_num,pid_dead);
			
			printf("Child[%ld - %d] dead.",(long)pid_dead,id);
			printf("%d",pTChilds[id].fd[0]);
			copyfile3(pTChilds[id].fd[0],fd);
			close(pTChilds[id].fd[0]);
		}

	}


	freeAll();

	return iTotal_wordnum;

}


int find_numof_elems_in_dir( char *dir_path,int *filenum, int *dirnum){

	char strPath[PATH_MAX];
	int iFile_num=0;
	int iDir_num=0;
	pid_t pid = getpid();

	*filenum=0;
	*dirnum=0;

	while(NULL != (pDirent_current = readdir(pDir_current))){
		sprintf(strPath,"%s/%s",dir_path,pDirent_current->d_name);
		if(strcmp(pDirent_current->d_name,".")!=0 && strcmp(pDirent_current->d_name,"..")!=0){
			#ifdef DEBUG
				fprintf(stdout, "[%ld] %d. Elem is %s\n",(long)pid,iFile_num+iDir_num,strPath);
			#endif
			if(is_directory(strPath)){
				++iDir_num;
			}else // if not directory 
			if(is_regfile(strPath)){
				++iFile_num;
			}
		}
	}
	
	rewinddir(pDir_current);

	strDirectories = (char **)calloc(sizeof(char*),iDir_num);
	strFiles = (char **)calloc(sizeof(char *),iFile_num);

	iFile_num=0;
	iDir_num=0;

	while(NULL != (pDirent_current = readdir(pDir_current))){
			sprintf(strPath,"%s/%s",dir_path,pDirent_current->d_name);
			if(strcmp(pDirent_current->d_name,".")!=0 && strcmp(pDirent_current->d_name,"..")!=0){
				if(is_directory(strPath)){
					strDirectories[iDir_num] = strdup(strPath);
					++iDir_num;
				}else // if not directory 
				if(is_regfile(strPath)){
					strFiles[iFile_num]=strdup(strPath);
					++iFile_num;
				}
			}
		}


	pDirent_current=NULL;
	*filenum = iFile_num;
	*dirnum=iDir_num;

}







// special exit method
void exit_hmenn(int status){
	freeAll();
	exit(status);
}


int is_regfile(const char * fileName){
  struct stat statbuf;

  if(stat(fileName,&statbuf) == -1){
    return 0;
  }else{
    return S_ISREG(statbuf.st_mode);
  }
}

int is_directory(const char *dirName){
  struct stat statbuf;
      if(stat(dirName   ,& statbuf) == -1){
    return 0;
  }
  else {
    return S_ISDIR(statbuf.st_mode);
  }
}


void * search_with_thread(void *args){

	
	if(NULL == args){
		printf("dasd\n");
		return NULL;
	}
	int total =0;


	t_search *search= (t_search *)args;
	printf("Thread File : %ld\n",search->tid);
	total = findOccurencesInFile(search->fd[1],search->filename,search->word);
	search->total = total;
	printf("Thered found %d\n",total);
	return NULL;
}

int findOccurencesInFile(int fd,const char* fileName,const char *word){

	char str[30];
  int fdFileToRead; /* okunacak dosya fildesi */
  char buf; /* tek karakter okumalik buffer */
  int i=0;
  int column=-1;
  int row=0;
  int found=0;
  int logCreated = 0;
  t_coordinat coord;

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
        coord.found=found;
        coord.row=row;
        coord.column=column;
        sprintf(str,"%d. %d %d\n",coord.found,coord.row,coord.column);
        #ifdef DEBUG_FILE_READ
        printf("%d. %d %d\n",coord.found,coord.row,coord.column);
        #endif
        write(fd,&str,sizeof(char)*(strlen(str)));
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
void open_pipe_connection(t_search * arr, int size){

	int i=0;

	for(i=0;i<size;++i){
		if(-1 == pipe(arr[i].fd)){
			perror("Pipe");
			exit_hmenn(0);

		}
		#ifdef DEBUG
			fprintf(stdout,"Pipe %d opened.\n",i);
		#endif
	}

}


void freeAll(){


	if(NULL != pTChilds){
		free(pTChilds);
		pTChilds=NULL;
	}

	


	if(NULL != pDir_current){
		closedir(pDir_current);
		pDir_current=NULL;
	}

	//pDirent_current=NULL;

	if(NULL != pTh_thread){
		free(pTh_thread);
		pTh_thread=NULL;
	}

	if(NULL != allSearch){
		free(allSearch);
		allSearch=NULL;
	}

	int i=0;
	if(NULL != strDirectories){
		for(i=0;i<iDir_num;++i){
			if(strDirectories[i]!=NULL){
				free(strDirectories[i]);
				strDirectories[i]=NULL;
			}
		}
		free(strDirectories);
		strDirectories=NULL;
	}


	if(NULL != strFiles){
		for(i=0;i<iFile_num;++i){
			if(strFiles[i]!=NULL){
				free(strFiles[i]);
				strFiles[i]=NULL;
			}
		}
		free(strFiles);
		strFiles=NULL;
	}


}



int copyfile(int fromfd, FILE* out) {
   	t_coordinat coord;
   	int totalnum=0;

   	while (read(fromfd,&coord,sizeof(coord)) > 0){
   		++totalnum;
   		printf("%d %d %d \n",coord.found,coord.row,coord.column);
      fprintf(out, "%d.  %d  %d\n",coord.found,coord.row,coord.column);
   }
   return totalnum;
}

int getID(t_child *arr,int size,pid_t pid){
  int i=0;
  if(NULL == arr || size <=0)
    return -1;

  for(i=0;i<size;++i)
    if(arr[i].pid==pid)
      return i;
  return -1;
}

ssize_t r_read(int fd, void *buf, size_t size) {
   ssize_t retval;
   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
   return retval;
}

ssize_t r_write(int fd, void *buf, size_t size) {
  char *bufp;
   size_t bytestowrite;
   ssize_t byteswritten;
   size_t totalbytes;

   for (bufp = buf, bytestowrite = size, totalbytes = 0;
        bytestowrite > 0;
        bufp += byteswritten, bytestowrite -= byteswritten) {
      byteswritten = write(fd, bufp, bytestowrite);
      if ((byteswritten) == -1 && (errno != EINTR))
         return -1;
      if (byteswritten == -1)
         byteswritten = 0;
      totalbytes += byteswritten;
   }
   return totalbytes;
}

int copyfile3(int fromfd, int tofd) {
   int bytesread;
   int totalbytes = 0;

   while ((bytesread = readwrite(fromfd, tofd)) > 0)
      totalbytes += bytesread;
   return totalbytes;
}

int readwrite(int fromfd, int tofd) {
   char buf[1024];
   int bytesread;

   if ((bytesread = r_read(fromfd, buf, 1024)) < 0)
      return -1;
   if (bytesread == 0)
      return 0;
   if (r_write(tofd, buf, bytesread) < 0)
      return -1;
   return bytesread;
}

int copyfile2(int fromfd, int tofd) {
   	t_coordinat coord;
   	int totalnum=0;

   	while (read(fromfd,&coord,sizeof(t_coordinat)) > 0){
   		++totalnum;
   		printf("%d %d %d \n",coord.found,coord.row,coord.column);
      write(tofd,&coord,sizeof(t_coordinat));
   }
   return totalnum;
}