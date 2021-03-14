#ifndef UTIL_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* CONSTANTS */
#define ROUTE_INFO_MIPD_PATH "route_info_sock_mipd"
#define ROUTE_REQUEST_MIPD_PATH "route_request_sock_mipd"
#define ROUTE_INFO_RD_PATH "route_info_sock_rd"
#define ROUTE_REQUEST_RD_PATH "route_request_sock_rd"
#define MAX_PATH_LENGTH 150

/* Global variables */
char rd_info_path[MAX_PATH_LENGTH];
char rd_request_path[MAX_PATH_LENGTH];
char mipd_info_path[MAX_PATH_LENGTH];
char mipd_request_path[MAX_PATH_LENGTH];

/*
 * Constructs paths from @identifier
 * Depends on global variables: @rd_info_path, @rd_request_path, @mipd_info_path, @mipd_request_path
 * @identifier: identifier which helps produce unique paths
 * Return: 1 if unsuccessful, else 0
 */
int initialize_paths(char* identifier);

/*
 * Creates datagram unix domain socket from path
 * @client_path: path
 * Return: file descriptor of created socket
 */
int dgram_domain_socket(char* client_path);

/*
 * Send on datagram unix domain socket
 * @sock: unix domain socket to send on
 * @buf: buffer to send
 * @buf_size: size of buffer
 * @server_path: path of server to send to
 * Return: bytes sent, else -1 if unsuccessful
 */
int send_on_dgram_domain_sock(int sock, char* buf, size_t buf_size, char* server_path);

/*
 * Free 2 dimensional pointer array
 * @arr: array to free
 * @size: element count in array
 * Return: 1 if unsuccessful, else 0
 */
int free_ptr_arr(void **arr, uint8_t size);

/*
 * Converts string to number
 * @message: message to convert
 * Return: integer if successful, else -1
 */
long int str_int(char *message);

#endif
