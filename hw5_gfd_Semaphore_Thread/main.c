#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include "hm.h"


#define PATH_MAX 255;



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

	// fifo ac 
	// fifoyu read edecek threadi olustur
	// read edip loga yazacak :D


	//recursive metodu cagir
	// metod bittikten sonra fifoya bittigini belirtmek icin path = -1 yaz



	return 0;
}



int findRec(const char *dirname,const char *word){

	// klasordeki tum txt leri bul
	// hepsinin pathini kaydet
	// daha sonra hepsi icin ayrı thread ve mutex pipe ac
	// bir thread ise mutexi habire okumasi icin


	return 0;
}


void threadFindOcc(void *arg){

	// kendisine gelen argumandan pipe a yazacak ama mutex yapacakki karismasin
	// dosyada koordinatlari ara
	// kendi tid si ve daha sonra satir sutun olarak yaz
	// bittiyse -1 toplam olarak yaz ve join ol


	// tid row col
	//	   row col
	//     -1  total

}

void threadRemovePipe(void *arg){

	// kendisine gelen toplam thread sayisi kadar tid -1 okuyana kadar pipe i bosalt
	// okuduklarini fifoya yaz

	// read : tid row col
	//			  -1 total

	// write : pathlen path row col
	//                      -1 total
}