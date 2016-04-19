CC=gcc
CFLAGS=-c   -D_POSIX_C_SOURCE

## -ansi for standart c bu signal.h is not in standart c, it's in posix

all: integralGen client clean

integralGen : integralGen.o
	$(CC) integralGen.o -o integralGen

integralGen.o : clean
	$(CC) $(CFLAGS) integralGen.c


client : client.o
	$(CC) client.o -o client

client.o : clean
	$(CC) $(CFLAGS) client.c

clean:
	rm -rf *.o
