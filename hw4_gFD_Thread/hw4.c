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

/* global variables */
pid_t *pPid_childs=NULL;
FILE *pFile_log=NULL;
DIR *pDir_current=NULL;
struct dirent * pDirent_current=NULL;

pthread_t * pTh_thread=NULL;
t_search *allSearch=NULL;

char **strDirectories=NULL;
char **strFiles=NULL;

int iFile_num=0;
int iDir_num=0;




int main(int argc,char *argv[]){


	if(argc!=3){
		fprintf(stderr,"Main arguments error.\n");
		fprintf(stderr,"USAGE : ./gfdT dirname word");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT,sigint_handler);

	// open log file and check error
	
	struct sigaction action;


	int iTotal_wordnum = search_dir(argv[1],argv[2]);

	printf("------------------------------------------\n");
	printf("Found %d times %s is %s dir.\n",iTotal_wordnum,argv[2],argv[1]);



	return 0;
}


// sigint handler
void sigint_handler(int signum){

	if(NULL != pFile_log){ // open log file
		pFile_log = fopen(LOG_FILE_NAME,"w");
		if(NULL == pFile_log){ // file opening error
			fprintf(stderr, "%s\n",FOPEN_LOG_ERROR);
			exit(EXIT_FAILURE);
		}
	}

	fprintf(pFile_log,"SIGINT (ctrl + c) handled. Program aborted.\n");
	fclose(pFile_log);
	pFile_log=NULL;

	// free child pid array
	if(NULL != pPid_childs){
		free(pPid_childs);
		pPid_childs=NULL;
	}




	exit(EXIT_FAILURE);
}


int search_dir( char *dir_path, char *word){

	int fildes_man;
	int total=0;

	pFile_log = fopen(LOG_FILE_NAME,"w");


	total = search_dir_recursive(dir_path,word);

	fprintf(pFile_log,"------------------------------------------\n");
	fprintf(pFile_log,"Found %d times %s is %s dir.\n",total,dir_path,word);
	freeAll();
	return total;
}




int search_dir_recursive( char *dir_path, char *word){

	pid_t pid_child=-1;
	pid_t pid_dead_child=-1;
	int iTotal_wordnum=0;
	int fdStatus = -1; // current dirrectory and file status
	int drStatus = -1;
	char strPath[PATH_MAX];



	if(NULL == (pDir_current = opendir(dir_path))){
		fprintf(stderr, "[%ld] %s : %s\n",(long)getpid(),DIROPEN_LOG_ERROR,strerror(errno));
		fprintf(pFile_log, "[%ld] %s : %s\n",(long)getpid(),DIROPEN_LOG_ERROR,strerror(errno));
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

	pPid_childs = calloc(sizeof(pid_t),iDir_num);
	allSearch = (t_search *)calloc(sizeof(t_search),iFile_num);

	open_pipe_connection(allSearch,iFile_num);
	rewinddir(pDir_current);

	while(NULL != (pDirent_current = readdir(pDir_current))){
		if(strcmp(pDirent_current->d_name,".")!=0 && strcmp(pDirent_current->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dir_path,pDirent_current->d_name);

			if(is_regfile(strPath)){
				++fdStatus;
				allSearch[fdStatus].filename = strFiles[fdStatus];
				allSearch[fdStatus].word = word;
				allSearch[fdStatus].status = fdStatus;
				printf("File: %s \n",allSearch[fdStatus].filename);
				
				pthread_create(&(allSearch[fdStatus].tid),NULL,search_with_thread,&allSearch[fdStatus]);

				// TODO : THREAD CONTINUE
				
			}else if(is_directory(strPath)){
				continue;

				if(-1 == (pid_child = fork())){
					fprintf(stderr,"Fork error.");
					exit_hmenn(0);
				}

				if(0 == pid_child){
					break;
				}
				else{
					// TODO : OPEN FIFO CONNECTIONS FOR PROCESS

				}

			}
		}
		//break;
	}


	// child here
	if(pid_child == 0){


	}else // parent here
	{
		// first wait for threads and read their outputs to global log file
		int i=0;
		for(i=0;i<=fdStatus;++i){
			pthread_join(allSearch[i].tid,NULL);
			close(allSearch[i].fd[1]);
			fprintf(pFile_log,"\nFILE : %s    // WORD : %s \n",allSearch[i].filename,allSearch[i].word);
	        iTotal_wordnum += copyfile(allSearch[i].fd[0],pFile_log);
	        close(allSearch[i].fd[0]);
			printf("Thread %d dead.\n",i);
		}

	}










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
        #ifdef DEBUG_FILE_READ
        printf("%d. %d %d\n",coord.found,coord.row,coord.column);
        #endif
        write(fd,&coord,sizeof(coord));
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


	if(NULL != pPid_childs){
		free(pPid_childs);
		pPid_childs=NULL;
	}

	if(NULL != pFile_log){
		fclose(pFile_log);
		pFile_log=NULL;
	}


	if(NULL != pDir_current){
		closedir(pDir_current);
		pDir_current=NULL;
	}

	pDirent_current=NULL;

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
      fprintf(out, "%d.  %d  %d\n",coord.found,coord.row,coord.column);
   }
   return totalnum;
}

