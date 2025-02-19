CC = gcc
CFLAGS = -Wall

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

run_server: server
	./server

run_client: client
	./client 127.0.0.1

clean:
	rm -f server client