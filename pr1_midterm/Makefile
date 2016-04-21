CC=gcc
CFLAGS=-c -D_POSIX_C_SOURCE

## -ansi for standart c bu signal.h is not in standart c, it's in posix

all: integralGen client  clean



integralGen : integralGen.o tinyexpr.o
	$(CC) integralGen.o tinyexpr.o -o integralGen -lm

tinyexpr.o : tinyexpr.c
	$(CC) $(CFLAGS) tinyexpr.c

integralGen.o : integralGen.c
	$(CC) $(CFLAGS) integralGen.c


client : client.o
	$(CC) client.o -o client

client.o : client.c
	$(CC) $(CFLAGS) client.c

clean:
	rm -rf *.o
