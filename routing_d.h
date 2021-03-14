#ifndef ROUTING_D_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>

#include "util.h"
#include "linked_list.h"

#define HELP_MESSAGE "Usage: \n" \
	"    routing_daemon [-h] <identifier>\n" \
	"    <identifier>: string used as identifier between MIP daemon and routing daemon\n"

/*
 * Routing table entry
 */
struct r_table_entry {
	uint8_t dst_mip;
	uint8_t next_hop;
	uint8_t src_mip;
	uint8_t weight;
};

/* Global variables */
struct node *routing_table;

/*
 * Prints routing table in terminal
 * Depends on global variable @routing_table
 */
void print_routing_table();

/*
 * Finds next hop for destination mip @dest_mip
 * Depends on global variable @routing_table
 * Return: next hop mip if found, else 255
 */
uint8_t find_next_hop(uint8_t dest_mip);

/*
 * Adds routing table entry to @routing_table
 * Depends on global variable @routing_table
 * @entry: entry to add to routing table
 * Return: 1 if unsuccessful, else 0
 */
int add_routing_table_entry(struct r_table_entry *entry);

/*
 * Update routing table
 * Depends on global variable @routing_table
 * @buf: buffer received from another routing daemon or mip daemon
 * @buf_size: buffer size
 * @type: type of content (1 if local addresses from mipd, 0 if another routing daemon)
 * Return: 1 if unsuccessful, else 0
 */
int update_routing_table(char* buf, int buf_size, uint8_t type);

/*
 * Send routing table to neighboring routing daemons
 * Depends on global variable @routing_table
 * @route_info_sock: info socket in interface between mip daemon and routing daemon
 * Return: 1 if unsuccessful, else 0
 */
int send_routing_table(int route_info_sock);

/*
 * Epoll loop which manages connections
 * Depends on global variable @routing_table
 * Return: 1 if unsuccessful
 */
int accept_packets(); 

#endif
