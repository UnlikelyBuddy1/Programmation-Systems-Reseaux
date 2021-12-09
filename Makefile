all: server server2

server: server.o
	gcc server.o -o server

server2: server2.o
	gcc -o server2 server2.c -lpthread

server.o: server.c tools.h
	gcc -c server.c -o server.o -Wall

clean:
	rm -rf *.o  client server
	rm -rf client server server2
	rm *copy_*
