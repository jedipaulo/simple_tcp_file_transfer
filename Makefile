CC = gcc
CFLAGS = -g3 -Werror -O0
LDLIBS = -lcrypto

server: src/server.o src/common.o src/tlv.o src/sal.o src/sal_linux.o
	$(CC) -o server src/server.o src/common.o src/tlv.o src/sal.o src/sal_linux.o -std=c99 -pedantic -Wall -Werror $(LDLIBS)

client: src/client.o src/common.o src/tlv.o src/sal.o src/sal_linux.o
	$(CC) -o client src/client.o src/common.o src/tlv.o src/sal.o src/sal_linux.o -std=c99 -pedantic -Wall -Werror $(LDLIBS)

clean:
	rm -f src/client.o src/server.o src/common.o src/tlv.o src/sal.o src/sal_linux.o

docs:
	doxygen doxygen.cfg

.PHONY: clean docs
