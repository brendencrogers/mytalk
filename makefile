CC = gcc
CFLAGS = -Wall -pedantic -g
LD = gcc
LDFLAGS = 

all: mytalk
	$(LD) $(LFLAGS) -o mytalk mytalk.o

mytalk: mytalk.o
	$(LD) $(LFLAGS) -o mytalk mytalk.o

mytalk.o: mytalk.c
	$(CC) $(CFLAGS) -c -o mytalk.o mytalk.c

clean:
	rm *.o
