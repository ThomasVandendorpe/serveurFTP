CC=gcc
CFLAGS=-Wall -Werror -Wextra -g
LDFLAGS=-lpthread

EXECUTABLES=clientSR serverSR

all: $(EXECUTABLES)

clientSR: clientSR.o csapp.o utils.o
	$(CC) $^ -o $@ $(LDFLAGS)

serverSR: serverSR.o csapp.o utils.o
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

csapp.o: csapp.h
clientSR.o: csapp.h utils.h
serverSR.o: csapp.h utils.h

clean:
	rm -rf *.o
	rm -rf $(EXECUTABLES)
