CC=gcc
CCFLAGS=-Wall -Wextra -Wconversion -g

all: mipd client server routing_d

mipd: mipd.c mipd.h util.c util.h linked_list.c linked_list.h
	$(CC) mipd.c util.c linked_list.c $(CCFLAGS) -o mipd

client: client.c util.c util.h
	$(CC) client.c util.c $(CCFLAGS) -o client

server: server.c
	$(CC) server.c $(CCFLAGS) -o server

routing_d: routing_d.c routing_d.h util.c util.h linked_list.c linked_list.h
	$(CC) -pthread routing_d.c util.c linked_list.c $(CCFLAGS) -o routing_d

run_mipd: mipd
	valgrind --track-origins=yes --leak-check=full ./mipd $(ARGS)

run_client: client
	valgrind --track-origins=yes --leak-check=full ./client $(ARGS)

run_server: server
	valgrind --track-origins=yes --leak-check=full ./server $(ARGS)

run_routing_d: routing_d
	valgrind --track-origins=yes --leak-check=full ./routing_d $(ARGS)

clean:
	rm -f mipd
	rm -f client
	rm -f server
	rm -f routing_d
