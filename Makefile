all: server1-COM_SUCKET server2-COM_SUCKET server3-COM_SUCKET

server1-COM_SUCKET: server.o
	gcc server.o -o server1-COM_SUCKET

server2-COM_SUCKET: server2.o
	gcc server2.o -o server2-COM_SUCKET

server3-COM_SUCKET: server.o
	gcc server.o -o server3-COM_SUCKET

server.o: server.c tools.h
	gcc -c server.c -o server.o -Wall

server2.o: server2.c tools.h
	gcc -c server2.c -o server2.o -Wall

clean:
	rm -rf *.o  client server1-COM_SUCKET server2-COM_SUCKET server3-COM_SUCKET
	rm *copy_*
