#ifndef HW4_131044009
 #define HW4_131044009 1

#define DEBUG_FILE_READ



#define LOG_FILE_NAME "hmenn.log"
#define FOPEN_LOG_ERROR "An error occurred while opening the file"
#define DIROPEN_LOG_ERROR "An error occured while opening the directory"


typedef struct {
	int fd[2];
}t_fildes;

typedef struct {
	const char *filename;
	const char *word;
	int status;
	int total;
}t_search;


// my special exit method to clear traces
void exit_hmenn(int status);


// interrupt / ctrl +c signal handler
void sigint_handler(int signum);

// recursive wrapper method to search directory
int search_dir(const char * dir_path, const char * word);


//recursive directory searcher
// it is PRIVATE function
int search_dir_recursive(const char *dir_path, const char *word, int *total);


// counst number of file and directory in dir_path
int find_numof_elems_in_dir(const char *dir_path, int *filenum, int *dirnum);


// thread search
void *search_with_thread(void *args);




#endif