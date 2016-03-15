CC=gcc
CFLAGS=-c -Wall

all: GrepFromDirectory clean

GrepFromDirectory: 131044009_HW2.o main.o
	$(CC) 131044009_HW2.o main.o -o exec

main.o: main.c
	$(CC) $(CFLAGS) main.c

131044009.o : 131044009_HW2.c
	$(CC) $(CFLAGS) 131044009_HW2.c

clean:
	rm *.o
