CC=gcc
CFLAGS=-c -Wall -ansi -pedantic-errors

all: GrepFromDirectory clean

GrepFromDirectory: HW2_131044009_HASAN_MEN.o
	$(CC) HW2_131044009_HASAN_MEN.o -o exec

## hocam burayi eskisi gibi birakmisim
##main.o: HW2_131044009_HASAN_MEN.c
HW2_131044009_HASAN_MEN.o: HW2_131044009_HASAN_MEN.c
	$(CC) $(CFLAGS) HW2_131044009_HASAN_MEN.c

clean:
	rm -rf *.o
	rm -rf *~
