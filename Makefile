#Sample Makefile. You can make changes to this file according to your need
# The executable must be named proxy

CC = gcc
LDFLAGS = -lpthread -g

OBJS = client.o
OBJS2 = server.o

all: client server

chat_client:	$(OBJS)

server: $(OBJS2)

chat_client.o: client.c
	$(CC) $(CFLAGS) -c client.c

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f *~ *.o client
	rm -f *~ *.o server 

