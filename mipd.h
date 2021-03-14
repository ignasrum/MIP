#ifndef MIPD_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#include <time.h>

#include <malloc.h>

#include "util.h"
#include "linked_list.h"

#define HELP_MESSAGE "Usage: \n" \
			"    mipd [-h] [-d] <socket_application> <identifier> [MIP addresses]\n" \
			"    <socket_application>: unix domain socket for communication with applications\n" \
			"    <identifier>: string used as identifier between MIP daemon and routing daemon\n" \
			"    [MIP addresses]: mip addresses to bind to network interfaces\n"
#define ETH_P_MIP 0x88B5
#define ETH_BROADCAST_ADDRESS {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

/*
 * interface struct used for storing network interfaces with their mac and mip addresses
 */
struct interface {
	char name[IF_NAMESIZE]; 
	struct sockaddr_ll mac;
	uint8_t mip;
};

/*
 * mip_arp struct used for storing arp table contents
 */
struct mip_arp {
	uint8_t mac[6];
	uint8_t dst_mip;
	uint8_t src_mip;
};	

/*
 * mip_header struct
 * 4 bytes = 32 bits
 */
struct mip_header {
	uint8_t t : 1;
	uint8_t r : 1;
	uint8_t a : 1;
	uint8_t ttl : 4;
	uint16_t plength : 9;
	uint8_t dest;    /* destination MIP address */
	uint8_t src;     /* source mip address */
};

/*
 * Used to store mip packets in a message queue
 */
struct packet {
	uint8_t dst_mip;
	uint8_t next_hop;
	uint8_t src_mip;
	uint8_t ttl;
	char message[1500 - sizeof(struct mip_header)];
	clock_t timer;
};

/* Global variables */
uint8_t debug_mode = 0;
struct interface **interfaces;
struct mip_arp **arp_cache;
int arp_size = 0;

/*
 * Gets all network interfaces
 * @counter: pointer to interface counter 
 * Return: returns pointer to pointer array of interfaces
 */
struct interface** get_network_interfaces(uint8_t *counter);

/*
 * Assigns mip addresses to network interfaces on current host
 * @interf_list: pointer to list of interfaces - this is where interfaces (with mip addresses) are added
 * @mip_addrs: pointer to mip addresses
 * @mip_count: mip address count
 * Return: returns assigned interface count
 */

uint8_t assign_mip_addresses(struct interface ***interf_list, uint8_t *mip_addrs, int mip_count);

/*
 * Creates ethernet socket on mip protocol
 * Return: returns file descriptor to socket
 */
int ethernet_socket();

/*
 * Creates unix domain socket and sets it to listen
 * @path: file path string
 * Return: returns file descriptor to socket
 */
int stream_domain_socket(char *path);

/*
 * Contructs an ethernet header
 * @ether_header: struct ether_header pointer where final header is stored
 * @dst_mac: destination mac address
 * @interface: interface to send on
 * Return: return 1 if unsuccessful, 0 if successful
 */
int construct_ether_header(struct ether_header *ether_header, uint8_t *dst_mac, struct interface *interface);

/*
 * Sends ethernet frame 
 * @msg: message to be sent
 * @address: mac address to send to
 * @interface: interface to send from
 * Return: returns sent bytes if successful, 1 if unsuccessful
 */
int send_ethernet_frame(char *msg, uint8_t *address, struct interface *interface);

/*
 * Deconstructs ethernet frame
 * @buf: pointer to received buffer
 * @eth_hdr: pointer to empty ether_header - this is where ethernet information is stored
 * Return: return char array with message
 */
char* deconstruct_ethernet_frame(char *buf, struct ether_header *eth_hdr); 

/*
 * Broadcast a message on ethernet
 * @message: pointer to message string
 * @interface: interface to broadcast from
 * Return: 0 if successful, 1 if unsuccessful
 */
int broadcast_ethernet_frame(char *message, struct interface *interface); 

/*
 * Checks arp cache for mip address
 * Depends on: global variable arp_cache
 * @mip_address: mip address to look for
 * Return: struct mip_arp with entry if found, NULL if not found
 */
struct mip_arp* check_arp_cache(uint8_t mip_address); 

/*
 * Check interfaces for interface with supplied mip address
 * @mip: mip address to look for
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * Return: struct interface with interface if found, NULL if not found
 */
struct interface* check_interfaces(uint8_t mip, struct interface **interfaces, int interf_count);

/*
 * Check interfaces for interface with supplied mip address
 * @name: interface name to look for
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * Return: struct interface with interface if found, NULL if not found
 */
struct interface* name_to_interface(char* name, struct interface **interfaces, int interf_count);

/*
 * Prints arp cache
 * Depends on: global variable @arp_cache
 */
void print_arp_cache();

/*
 * Adds entry to arp cache table
 * Depends on: global variable @arp_cache
 * @mac_addr: mac address to add
 * @dst_mip: destination mip address to add
 * @src_mip: source mip address to add
 * Return: return struct mip_arp with entry
 */
struct mip_arp* add_to_arp_cache(uint8_t mac_addr[], uint8_t dst_mip, uint8_t src_mip); 

/*
 * Constructs mip header
 * @header: pointer to a struct mip_header, where final header is stored
 * @t: transport setting
 * @r: routing setting
 * @a: arp setting
 * @ttl: Time to Live
 * @dest: destination mip address
 * @src: source mip address
 * @message: message for which header is constructed
 * Return: return 1 if unsuccessful, 0 if successful
 */
int construct_mip_header(struct mip_header *header, int t, int r, int a, uint8_t ttl, 
			 uint8_t dest, uint8_t src, char *message);

/*
 * Construct mip packet
 * @header: mip header
 * @message: message to send
 * @msg_size: size of message
 * Return: char buffer with packet, NULL if unsuccessful
 */
char* construct_mip_packet(struct mip_header header, char *message, size_t msg_size);

/*
 * Prints information about communication on host, called when communication has occurred
 * @eth_hdr: struct ether_header from packet
 * @mip_header: struct mip_header from packet
 */
void print_communication_info(struct ether_header eth_hdr, struct mip_header mip_header);

/*
 * Deconstructs mip packet
 * @packet: mip packet
 * @mip_header: pointer to struct mip_header - header info is stored here
 * Return: message string
 */
char* deconstruct_mip_packet(char *packet, struct mip_header *mip_header); 

/*
 * Executes ARP resolution
 * Depends on: global variable @arp_cache
 * @mip_address: mip address to look for
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * @socket: socket to send on 
 * Return: struct mip_arp if successful, NULL if unsuccessful
 */
struct mip_arp* arp_resolution(uint8_t mip_address, struct interface **interfaces, int interf_count, int socket);

/*
 * Adds packet to message queue with @root
 * @root: root of linked list (message queue)
 * @dst_mip: destination mip address
 * @next_hop: next hop mip address
 * @src_mip: source mip address
 * @ttl: Time to Live
 * @message: message
 * Return: return 1 if unsuccessful, 0 if successful
 */
int add_packet_to_queue(struct node *root, uint8_t dst_mip, uint8_t next_hop, uint8_t src_mip, 
			uint8_t ttl, char* message);

/*
 * Send mip packet 
 * @message: message to send
 * @msg_size: size of message
 * @dst_mip: destination mip address
 * @next_hop: next hop mip address
 * @src_mip: source mip address
 * @ttl: Time to Live
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * @socket: socket to send on
 * Return: bytes sent if successful, 1 if unsuccessful
 */
ssize_t send_mip_packet(char *message, size_t msg_size, uint8_t dst_mip, uint8_t next_hop, uint8_t src_mip, 
			uint8_t ttl, struct interface **interfaces, int interf_count, int socket);

/*
 * Broadcasts a mip packet
 * @message: message to send
 * @msg_size: size of message
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * @t: transport setting in mip header
 * @r: routing setting in mip header
 */
void broadcast_mip_packet(char *message, size_t msg_size, struct interface **interfaces, 
			  int interf_count, int t, int r);

/*
 * Send ARP response
 * @mac_addr: destination mac address
 * @dest_addr: destination mip address
 * @src_addr: source mip address
 * @interface: interface to send on
 * Return: bytes sent if successful, 1 if unsuccessful
 */
ssize_t send_arp_response(uint8_t *mac_addr, uint8_t dest_addr, uint8_t src_addr, struct interface *interface);

/*
 * Sends packets in message queue
 * Checks if timer has elapsed (1 sec), and whether time to live < 0
 * @root: root of linked list (message queue)
 * @dst_mip: destination mip address
 * @next_hop: next hop mip address
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * @socket: socket to send on
 * Return: returns new root of message queue
 */
struct node* send_packets(struct node *root, uint8_t dst_mip, uint8_t next_hop, 
		  	  struct interface **interfaces, int interf_count, int socket);

/*
 * Sends local mip addresses to routing daemon
 * @interfaces: pointer to list of interfaces
 * @interf_count: interface count
 * @route_info_sock: info socket between mip daemon and routing daemon
 * Return: return 1 if unsuccessful, 0 if successful
 */
int send_mip_addrs_rd(struct interface **interfaces, int interf_count, int route_info_sock);

/*
 * Epoll loop which accepts connection and ethernet packets
 * @interfaces: list with network interfaces
 * @interf_count: network interface count
 * @application_socket: unix domain socket file descriptor
 * Return: 1 if unsuccessful
 */
int accept_packets(struct interface **interfaces, int interf_count, int application_socket);

#endif
