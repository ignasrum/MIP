#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define HELP_MESSAGE "Usage: \n" \
	"    client [-h] <destination_host> <message> <socket_application>\n" \
	"    <destination_host> - mip address of destination host\n" \
	"    <message> - message to send\n" \
	"    <socket_application> - pathname of unix socket\n"

int main(int argc, char *argv[]) {
	char *message;
	char *sock_path;
	uint8_t dest_host;
	if(argc < 4) {
		printf(HELP_MESSAGE);
		exit(1);
	} else if(strcmp(argv[1], "-h") == 0) {
		printf(HELP_MESSAGE);
		exit(1);
	} else {
		dest_host = (uint8_t)atoi(argv[1]);
		message = argv[2];
		sock_path = argv[3];
	}
	/* create socket */
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sock_path, strlen(sock_path));
	/* set timeout of 1 second */
	struct timeval timeout;
	timeout.tv_usec = 0;
	timeout.tv_sec = 1;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	char buf[strlen(message) + 1];
	buf[0] = dest_host;
	memcpy(&buf[1], message, strlen(message));
	/* connect on socket */
	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    		perror("connect error");
    		exit(-1);
  	}
	/* send message and start timer */
	clock_t pre = clock();
	if(send(fd, buf, strlen(message) + 1, 0) == -1) {
		perror("send error");
		exit(-1);
	}
	/* wait for message */
	char buffer[1500];
	while(1) {
		int result = recv(fd, buffer, 1500, 0);
		if(result == -1) {
			perror("Failed to receive packet");
		}
		/* if message is pong*/
		if(strcmp(&buffer[1], "PONG") == 0) {
			printf("%s\n", &buffer[1]);
			clock_t diff = clock() - pre;
			double m_seconds = (diff * 1000.0) / CLOCKS_PER_SEC;
			/* print time */
			printf("Spent %.3f millliseconds\n", m_seconds);
		}
		break;
	}
	return 0;
}
