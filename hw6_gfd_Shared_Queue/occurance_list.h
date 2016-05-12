#ifndef _131044009_OCCURANCE_LIST
 #define _131044009_OCCURANCE_LIST
#include <stdio.h>
#include <stdlib.h>

// genel olarak tekrarlama yapisini tutacak
typedef struct node_s{
	int row;
	int column;
	struct node_s *next;
}node_t;

// koordinatlar linked list olarak tutulacak
typedef struct{
	int total;
	node_t *last; // eklemeyi constant zamana indirmek icin
	node_t *head;
	const char *fileName;
}occurance_t;



// listenin sonuna ekleme yapar
void addLastOccurance(occurance_t *hm,int row,int col);

// listeyi ekrana basar 
void printOccurance(occurance_t * occ);

// listeyi temizler
void deleteOccurance(occurance_t *occ);

// yapiyi dosyaya basar
void printOccurancesToLog(const char *fname,occurance_t * occ,long totalTime);


#endif