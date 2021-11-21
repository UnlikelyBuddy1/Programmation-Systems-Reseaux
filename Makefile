all: server  client

client: client.o
	gcc client.o -o client

client.o: client.c
	gcc -c client.c -o client.o -Wall

server: server.o
	gcc server.o -o server

server.o: server.c tools.h
	gcc -c server.c -o server.o -Wall

clean:
	rm -rf *.o  client server
	rm -rf client server
	rm -rf client server
