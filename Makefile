CC=gcc

server: src/server.o src/sal_linux.o
	$(CC) -o server src/server.o src/sal_linux.o -std=c99 -pedantic -Wall -Werror

client: src/client.o
	$(CC) -o client src/client.o src/sal_linux.o -std=c99 -pedantic -Wall -Werror
