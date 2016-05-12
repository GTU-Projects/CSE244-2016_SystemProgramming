#include "occurance_list.h"
#include <stdlib.h>
void addLastOccurance(occurance_t *hm,int row,int col){

	node_t *newNode; // yeni nod olustur
	newNode=(node_t*)malloc(sizeof(node_t));
	newNode->row = row;
	newNode->column=col;
	newNode->next=NULL;

	if(hm->head==NULL){ // bos ise basina ekle
		hm->total=0;
		hm->head = newNode;
		hm->last = newNode;
	}else{ // degilse sonuna ekle

		hm->last->next = newNode;
		hm->last = hm->last->next;
	}
	hm->total +=1;
}


void printOccurance(occurance_t * occ){

	if(occ ==NULL)
		perror("Null parameter");
	else{
		if(occ->head==NULL)
			printf("empty list\n");
		else{
			node_t *ref = occ->head;
			while(ref!=NULL){
				printf("R: %d - C: %d\n",ref->row,ref->column);
				ref = ref->next;
			}
			printf("Total %d\n",occ->total);
		}
	}
}


void printOccurancesToLog(const char *fname,occurance_t * occ){


	FILE * fpLog = fopen(fname,"a");

	if(occ ==NULL)
		perror("Null parameter");
	else{
		if(occ->head==NULL)
			fprintf(fpLog,"empty list\n");
		else{
			fprintf(fpLog, "File %s\n",occ->fileName);
			node_t *ref = occ->head;
			while(ref!=NULL){
				fprintf(fpLog,"R: %d - C: %d\n",ref->row,ref->column);
				ref = ref->next;
			}
			fprintf(fpLog,"Total %d\n",occ->total);
		}
	}
	fflush(fpLog);
	fclose(fpLog);
}


void deleteOccurance(occurance_t *occ){
	if(occ!=NULL){
		if(occ->head !=NULL){
			node_t *ref;
			while(occ->head != NULL){
				ref = occ->head;
				occ->head = ref->next;
				free(ref);
			}
		}
	}
}


int test(){

	occurance_t *hm = malloc(sizeof(occurance_t));

	hm->total=0;
	hm->head=NULL;
	hm->last=NULL;


	addLastOccurance(hm,3,4);
	addLastOccurance(hm,3,4);
	addLastOccurance(hm,3,4);
	addLastOccurance(hm,3,4);
	printOccurance(hm);
	deleteOccurance(hm);
	
	free(hm);

}