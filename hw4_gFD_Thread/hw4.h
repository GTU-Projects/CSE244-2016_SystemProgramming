#ifndef HW4_131044009
 #define HW4_131044009 1

//#define DEBUG_FILE_READ 



#define FD_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MAX_FILE_NAME 20
#define LOG_FILE_NAME "hmenn.log"
#define FOPEN_LOG_ERROR "An error occurred while opening the file"
#define DIROPEN_LOG_ERROR "An error occured while opening the directory"


typedef struct {
	pthread_t tid;
	int fd[2];
	char *filename;
	char *word;
	int status;
	int total;
}t_search;

typedef struct{
	pid_t pid;
	int status;
	int fd[2]; // fifo 
}t_child;

typedef struct {
	int found;
	int row;
	int column;
}t_coordinat;


// my special exit method to clear traces
void exit_hmenn(int status);


// interrupt / ctrl +c signal handler
void sigint_handler(int signum);

// recursive wrapper method to search directory
int search_dir( char * dir_path,  char * word);


//recursive directory searcher
// it is PRIVATE function
int search_dir_recursive( char *dir_path,  char *word,int fd);


// counst number of file and directory in dir_path
int find_numof_elems_in_dir( char *dir_path, int *filenum, int *dirnum);


// thread search
void *search_with_thread(void *args);

void open_pipe_connection(t_search * arr,int size);

void freeAll();



int getID(t_child *arr,int size,pid_t pid);

// from course book
int copyfile(int fromfd, FILE *out);




int copyfile2(int fromfd, int tofd);
int readwrite(int fromfd, int tofd);
ssize_t r_write(int fd, void *buf, size_t size);
ssize_t r_read(int fd, void *buf, size_t size);

#endif