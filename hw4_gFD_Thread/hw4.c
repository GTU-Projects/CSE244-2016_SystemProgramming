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
t_fildes *pFD_pipe=NULL;
FILE *pFile_log=NULL;
DIR *pDir_current=NULL;
struct dirent * pDirent_current=NULL;

pthread_t * pTh_thread=NULL;




int main(int argc,char *argv[]){


	if(argc!=3){
		fprintf(stderr,"Main arguments error.\n");
		fprintf(stderr,"USAGE : ./gfdT dirname word");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT,sigint_handler);

	// open log file and check error
	pFile_log = fopen(LOG_FILE_NAME,"w");
	if(NULL == pFile_log){
		fprintf(stderr, "%s\n",FOPEN_LOG_ERROR);
		exit(EXIT_FAILURE);
	}

	struct sigaction action;



	int iTotal_wordnum = search_dir(argv[1],argv[2]);




	fprintf(pFile_log,"------------------------------------------\n");
	fprintf(pFile_log,"Found %d times %s is %s dir.\n",iTotal_wordnum,argv[2],argv[1]);
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

	// free thread pipe array
	if(NULL != pFD_pipe){
		free(pFD_pipe);
		pFD_pipe=NULL;
	}


	exit(EXIT_FAILURE);
}


int search_dir(const char *dir_path,const char *word){

	int fildes_man;
	int total=0;

	search_dir_recursive(dir_path,word,&total);
	return total;
}




int search_dir_recursive(const char *dir_path,const char *word,int *total){

	pid_t pid_child=-1;
	pid_t pid_dead_child=-1;
	int iTotal_wordnum=0;
	int iFile_num=0;
	int iDir_num=0;
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


	pFD_pipe = calloc(sizeof(t_fildes),iFile_num);
	pPid_childs = calloc(sizeof(pid_t),iDir_num);
	pTh_thread = calloc(sizeof(pthread_t),iFile_num);


	rewinddir(pDir_current);

	while(NULL != (pDirent_current = readdir(pDir_current))){
		if(strcmp(pDirent_current->d_name,".")!=0 && strcmp(pDirent_current->d_name,"..")!=0){
			sprintf(strPath,"%s/%s",dir_path,pDirent_current->d_name);

			if(is_regfile(strPath)){
				t_search tS_send;;
				++fdStatus;

				tS_send.filename = strPath;
				tS_send.word = word;
				tS_send.status = fdStatus;
				
				open_pipe_connection(pFD_pipe,fdStatus);

				pthread_create(&pTh_thread[fdStatus],NULL,search_with_thread,(void *)&tS_send);
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

	}

	int i=0;
	for(i=0;i<fdStatus;++i)
		pthread_join(pTh_thread[i],NULL);

	exit_hmenn(0);
}


int open_pipe_connection(t_fildes * arr, int index){

	if(NULL == arr || index <0)
		return 0;

	if(-1 == (pipe(arr[index].fd))){
		fprintf(stderr,"Failed to openinin pipe\n");
		exit_hmenn(1);
	}

	#ifdef DEBUG
		fprintf(stdout,"Pipe opened. Index : %d\n",index);
	#endif
	return 1;
}


int find_numof_elems_in_dir(const char *dir_path,int *filenum, int *dirnum){

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

	pDirent_current=NULL;
	*filenum = iFile_num;
	*dirnum=iDir_num;

}







// special exit method
void exit_hmenn(int status){
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
		return NULL;
	}
	int total =0;


	t_search *tS_send = (t_search *)args;

	total = findOccurencesInFile(pFD_pipe[tS_send->status].fd,tS_send->filename,tS_send->word);
	tS_send->total = total;
	return NULL;
}

int findOccurencesInFile(int fd,const char* fileName,const char *word){

  char wordCoordinats[30];/* dosyaya koordinatlari basmak icin string yuvasi */
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
         /*  EGER KELIME VARSA LOG FILE AC VE YAZ YOKSA ELLEME */
        if(logCreated == 1){
          /* log dosyasinin basina bilgilendirme olarak path basildi */
          write(fd,fileName,strlen(fileName));
          write(fd,"\n",1);
          logCreated =1;
        }
        lseek(fdFileToRead,-i+1,SEEK_CUR);
        column =column - i+1;
        #ifdef DEBUG_FILE_READ
        printf("%d. %d %d\n",found,row,column);
        #endif
        sprintf(wordCoordinats,"%d%c%d%c%d%c",found,'.',row,' ',column,'\n');
        write(fd,wordCoordinats,strlen(wordCoordinats));
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