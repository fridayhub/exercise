cc=gcc
DEBUGG = -Os -s -Wall

all:server_epoll client

server_epoll:server_epoll.c
	$(cc) $(DEBUGG) server_epoll.c -o server_epoll

client:client.c
	$(cc) $(DEBUGG) client.c -o client

.PHONY:clean
clean:
	rm -f client server_epoll
