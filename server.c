#include <sys/socket.h>
#include <sys/un.h>

#include "util.h"

#define HELP_MESSAGE "Usage: \n" \
	"    server [-h] <socket_application>\n" \
	"    <socket_application> - pathname of unix socket\n"

int main(int argc, char *argv[]) {
	char *sock_path;
	if(argc < 2) {
		printf(HELP_MESSAGE);
		exit(1);
	} else if(strcmp(argv[1], "-h") == 0) {
		printf(HELP_MESSAGE);
		exit(1);
	} else {
		sock_path = argv[1];
	}
	/* create socket */
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sock_path, strlen(sock_path));
	/* connect on socket */
	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    		perror("connect error");
    		exit(-1);
  	}
	/* receive message */
	char buf[1500];
	while(1) {
		int result = recv(fd, buf, 1500, 0);
		if(result == -1) {
			perror("Failed to receive packet");
		}
		break;
	}
	uint8_t dst_mip = buf[0];
	printf("%s \n", &buf[1]);
	/* if message is ping, send pong */
	if(strncmp(&buf[1], "PING", 4) == 0) {
		char *pong = "PONG";
		memcpy(&buf[1], pong, sizeof(pong));
		if(send(fd, buf, strlen(buf), 0) == -1) {
			perror("send error");
			exit(-1);
		}
	}
	return 0;
}
