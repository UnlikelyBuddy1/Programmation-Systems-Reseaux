all: server server2

server: server.o
	gcc server.o -o server

server.o: server.c tools.h
	gcc -c server.c -o server.o -Wall

clean:
	rm -rf *.o  client server
	rm *copy_*
