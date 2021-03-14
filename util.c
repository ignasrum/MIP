#include "util.h"

int initialize_paths(char* identifier) {
	/* Initializing socket paths */
	sprintf(rd_info_path, "%s_%s", identifier, ROUTE_INFO_RD_PATH);
	sprintf(rd_request_path, "%s_%s", identifier, ROUTE_REQUEST_RD_PATH);
	sprintf(mipd_info_path, "%s_%s", identifier, ROUTE_INFO_MIPD_PATH);
	sprintf(mipd_request_path, "%s_%s", identifier, ROUTE_REQUEST_MIPD_PATH);
	/*****************************/
	return 0;
}

int dgram_domain_socket(char* client_path) {
	/* create socket */
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sun_family = AF_UNIX;
	strncpy(client_addr.sun_path, client_path, strlen(client_path));
	unlink(client_path);
	/* bind to address */
	bind(fd, (struct sockaddr*)&client_addr, sizeof(client_addr));
	return fd;
}

int send_on_dgram_domain_sock(int sock, char* buf, size_t buf_size, char* server_path) {
	struct sockaddr_un server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, server_path, strlen(server_path));
	int rc;
	if(rc = sendto(sock, buf, buf_size, 0, (struct sockaddr*)&server_addr, 
		  sizeof(server_addr)) == -1) {
		perror("send error");
	}
	return rc;
}

int free_ptr_arr(void **arr, uint8_t size) {
	/* frees two dimensional array */
	uint8_t i = 0;
	while(i < size && arr[i] != NULL) {
		free(arr[i]);
		i++;
	}
	free(arr);
	return 0;
}

long int str_int(char *message) {
	/* converts string to number */
	long int result = -1;
	char *endptr;
	result = strtol(message, &endptr, 0);
	if(endptr == message) {
		printf("No digits.\n");
	}
	return result;
}
