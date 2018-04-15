CC = gcc -std=gnu99

server: server.c sha256.c sha256.h uint256.h server.h
	$(CC) -g -o server server.c sha256.c -lpthread

clean:
	rm -rf server
