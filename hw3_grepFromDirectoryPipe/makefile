CC=gcc
CFLAGS=-c  -Wall -ansi -pedantic-errors

all: GrepFromDirectory clean

GrepFromDirectory : main.o HW3_131044009.o
	$(CC) main.o HW3_131044009.o -o exec

main.o: main.c
		$(CC) $(CFLAGS) main.c

HW3_131044009.o: HW3_131044009.c
		$(CC) $(CFLAGS) HW3_131044009.c

clean:
	rm -rf *.o
	rm -rf *~
	rm -rf *.fifo
	rm -rf *.mercan
