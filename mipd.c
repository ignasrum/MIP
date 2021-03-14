#include "mipd.h"

struct interface** get_network_interfaces(uint8_t *counter) {
	(*counter) = 0;
	struct interface **interf_list = calloc(1, sizeof(struct interface*));
  	struct ifaddrs *interfaces, *ifp;
	
	if (getifaddrs(&interfaces)) {
    		perror("getifaddrs");
    		return NULL;
  	}

	/* go through all interfaces */
  	for (ifp = interfaces; ifp != NULL; ifp = ifp->ifa_next) {
    		if (ifp->ifa_addr != NULL && ifp->ifa_addr->sa_family == AF_PACKET && strcmp(ifp->ifa_name, "lo") != 0) {
			/* store target interfaces in @interf_list */
			(*counter)++;
			interf_list = realloc(interf_list, sizeof(struct interface*) * (*counter));
			interf_list[(*counter) - 1] = calloc(1, sizeof(struct interface));
      			memcpy(&interf_list[(*counter) - 1]->mac, (struct sockaddr_ll*)ifp->ifa_addr, 
					sizeof(struct sockaddr_ll));
      			memcpy(&interf_list[(*counter) - 1]->name, ifp->ifa_name, 
					strlen(ifp->ifa_name));
    		}
  	}

  	freeifaddrs(interfaces);
	return interf_list;
}

uint8_t assign_mip_addresses(struct interface ***interf_list, uint8_t *mip_addrs, int mip_count) {
	/* assign MIP addresses to network interfaces on current machine */
	uint8_t mac_count = 0;
	struct interface** interf_temp = get_network_interfaces(&mac_count);
	if(interf_list == NULL) {
		return 1;
	}
	uint8_t i = 0;
	if(mac_count > mip_count) {
		printf("Not enough MIP addresses were supplied.\n");
	}
	while(i < mac_count && i < mip_count) {
		memcpy(&interf_temp[i]->mip, &mip_addrs[i], sizeof(uint8_t));
		printf("Interface - MIP: %d, MAC: %x:%x:%x:%x:%x:%x \n", interf_temp[i]->mip,
			       	interf_temp[i]->mac.sll_addr[0],
				interf_temp[i]->mac.sll_addr[1],
				interf_temp[i]->mac.sll_addr[2],
				interf_temp[i]->mac.sll_addr[3],
				interf_temp[i]->mac.sll_addr[4],
				interf_temp[i]->mac.sll_addr[5]);
		i++;
	}
	*interf_list = interf_temp;
	return mac_count;
}

int ethernet_socket() {
	/* create raw socket on mip protocol */
	int sockfd = 0;
	if((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_MIP))) == -1) {
		perror("Failed to create socket");
		return 1;
	}	
	return sockfd;
}

int stream_domain_socket(char *path) {
	/* create stream unix domain socket */
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, strlen(path));
	unlink(path);
	/* bind and listen */
	bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(fd, 10);
	return fd;
}

int construct_ether_header(struct ether_header *ether_header, uint8_t *dst_mac, struct interface *interface) {
	/* copy addresses into header */
  	memcpy(ether_header->ether_dhost, dst_mac, 6);
  	memcpy(ether_header->ether_shost, interface->mac.sll_addr, 6);
	/* set ethernet protocol type */
  	ether_header->ether_type = htons(ETH_P_MIP);
	return 0;
}

int send_ethernet_frame(char *msg, uint8_t *address, struct interface *interface) {
	struct ether_header ether_header;
	struct sockaddr_ll target_address = interface->mac;
	struct msghdr *message;
	struct iovec message_vector[2];

	/* copy addresses into header */
  	memcpy(ether_header.ether_dhost, address, 6);
  	memcpy(ether_header.ether_shost, interface->mac.sll_addr, 6);
	/* set ethernet protocol type */
  	ether_header.ether_type = htons(ETH_P_MIP);

	/* setup message to be sent */
 	memcpy(target_address.sll_addr, address, 6);
	message_vector[0].iov_base = &ether_header;
	message_vector[0].iov_len = sizeof(struct ether_header);
	message_vector[1].iov_base = msg;
	message_vector[1].iov_len = malloc_usable_size(msg);
	printf("send ethernet frame of size: %d \n", malloc_usable_size(msg));

	/* setup message header */
	message = calloc(1, sizeof(struct msghdr));
  	message->msg_name = &target_address;
  	message->msg_namelen = sizeof(struct sockaddr_ll);
  	message->msg_iovlen = 2;
  	message->msg_iov = message_vector;

	/* create ethernet socket and send message */
	int socket = ethernet_socket();
	ssize_t result = sendmsg(socket, message, 0);
	if(result == -1) {
		perror("Failed to send message");
		free(message);
		return 1;
	}
	free(message);
	return result;
}

char* deconstruct_ethernet_frame(char *buf, struct ether_header *eth_hdr) {
    	char *p = buf;
	memcpy(eth_hdr, (struct ether_header*)buf, sizeof(struct ether_header));
	p = buf + sizeof(struct ether_header);
	return p;
}

int broadcast_ethernet_frame(char *message, struct interface *interface) {
	uint8_t broadcast_address[] = ETH_BROADCAST_ADDRESS;
	struct ether_header eth_hdr;
	construct_ether_header(&eth_hdr, broadcast_address, interface);
	return send_ethernet_frame(message, broadcast_address, interface);
}

struct mip_arp* check_arp_cache(uint8_t mip_address) {
	int i = 0;
	while(i < arp_size) {
		if(arp_cache[i]->dst_mip == mip_address) {
			return arp_cache[i];
		}
		i++;
	}
	return NULL;	
}

struct interface* check_interfaces(uint8_t mip, struct interface **interfaces, int interf_count) {
	int i = 0;
	while(i < interf_count) {
		if(interfaces[i]->mip == mip) {
			return interfaces[i];
		}
		i++;
	}
	return NULL;	
}

struct interface* name_to_interface(char* name, struct interface **interfaces, int interf_count) {
	int i = 0;
	while(i < interf_count) {
		if(strcmp(interfaces[i]->name, name) == 0) {
			return interfaces[i];
		}
		i++;
	}
	return NULL;	
}

void print_arp_cache() {
	int i = 0;
	if(arp_size == 0) {
		printf("ARP Cache is currently empty. \n");
		return;
	}
	printf("ARP TABLE: \n");
	while(i < arp_size) {
		printf("	ARP element - MIP: %d, MAC: %x:%x:%x:%x:%x:%x \n", arp_cache[i]->dst_mip,
			       	arp_cache[i]->mac[0],
				arp_cache[i]->mac[1],
				arp_cache[i]->mac[2],
				arp_cache[i]->mac[3],
				arp_cache[i]->mac[4],
				arp_cache[i]->mac[5]);
		i++;
	}
}

struct mip_arp* add_to_arp_cache(uint8_t mac_addr[], uint8_t dst_mip, uint8_t src_mip) {
	struct mip_arp* rc = check_arp_cache(dst_mip);
	if(rc != NULL) {
		return rc;
	}
	if(arp_size == 0) {
		arp_cache = calloc(1, sizeof(struct mip_arp*) * 1);
	} else {
		arp_cache = realloc(arp_cache, sizeof(struct mip_arp*) * (arp_size + 1));
	}
	struct mip_arp *mip_arp = calloc(1, sizeof(struct mip_arp));
	memcpy(&mip_arp->mac, mac_addr, sizeof(uint8_t) * 6);
	memcpy(&mip_arp->dst_mip, &dst_mip, sizeof(uint8_t));
	memcpy(&mip_arp->src_mip, &src_mip, sizeof(uint8_t));
	arp_cache[arp_size] = mip_arp;
	arp_size++;
	return arp_cache[arp_size];

}

int construct_mip_header(struct mip_header *header, int t, int r, int a, uint8_t ttl, 
			 uint8_t dest, uint8_t src, char *message) {
	/* construct mip header */
	header->t = 0;
	header->r = 0;
	header->a = 0;
	if(t == 1) {
		header->t = 1;
	}
	if(r == 1) {
		header->r = 1;
	}
	if(a == 1) {
		header->a = 1;
	}
	header->ttl = ttl;
	header->plength = (strlen(message) / 4) + 2;  /* weird bug, plength HAS to be set to 2 or more at all times */
	header->dest = dest;
	header->src = src;
	return 0;
}

char* construct_mip_packet(struct mip_header header, char *message, size_t msg_size) {
	/* check for message constraints */
	if((strlen(message) + sizeof(struct mip_header)) > 1500) {
		perror("Unable to create mip packet - exceeds 1500 bytes");
		return NULL;
	}
	if((strlen(message) + sizeof(struct mip_header)) % 4 != 0) {
		perror("Unable to create mip packet - not multiple of 4");
		return NULL;
	}
	/* construct mip packet - header first, then message right after */
	char *buffer = calloc(1, sizeof(struct mip_header) + msg_size);
	memcpy(buffer, &header, sizeof(struct mip_header));
	char *p = buffer + sizeof(struct mip_header);
	memcpy(p, message, msg_size);
	return buffer;
}

void print_communication_info(struct ether_header eth_hdr, struct mip_header mip_header) {
	printf("Packet received: \n");
	if(debug_mode) {
		printf("	T: %d, R %d, A %d \n", mip_header.t, mip_header.r, mip_header.a);
		printf("	Ethernet: %x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x \n", 
				eth_hdr.ether_shost[0],
				eth_hdr.ether_shost[1],
				eth_hdr.ether_shost[2],
				eth_hdr.ether_shost[3],
				eth_hdr.ether_shost[4],
				eth_hdr.ether_shost[5],
				eth_hdr.ether_dhost[0],
				eth_hdr.ether_dhost[1],
				eth_hdr.ether_dhost[2],
				eth_hdr.ether_dhost[3],
				eth_hdr.ether_dhost[4],
				eth_hdr.ether_dhost[5]);
		printf("	MIP: %d -> %d \n", mip_header.src, mip_header.dest);
		printf("	TTL: %d \n", mip_header.ttl);
		print_arp_cache();
	}	
}

char* deconstruct_mip_packet(char *packet, struct mip_header *mip_header) {
	struct ether_header eth_hdr;
	char *mip_packet = deconstruct_ethernet_frame(packet, &eth_hdr);
    	char *p = mip_packet;
	/* copy mip header into @mip_header */
	memcpy(mip_header, (struct mip_header*)mip_packet, sizeof(struct mip_header));
	/* rest is the message */
	p = mip_packet + sizeof(struct mip_header);
	return p;
}

struct mip_arp* arp_resolution(uint8_t mip_address, struct interface **interfaces, int interf_count, int socket) {
	printf("executing arp resolution\n");
	int result;
	/* check if mip address in arp cache */
	struct mip_arp *mip_arp = check_arp_cache(mip_address);
	if(mip_arp != NULL) {
		return mip_arp;
	}
	/* broadcast on ethernet */
	/* broadcast on every network interface */
	int i = 0;
	while(i < interf_count) {
		struct mip_header mip_hdr;
		construct_mip_header(&mip_hdr, 0, 0, 1, 15, mip_address, interfaces[i]->mip, "");
		char *packet = construct_mip_packet(mip_hdr, "", 0);
		result = broadcast_ethernet_frame(packet, interfaces[i]);
		free(packet);
		i++;
	}
	return NULL;
}

int add_packet_to_queue(struct node *root, uint8_t dst_mip, uint8_t next_hop, uint8_t src_mip, 
			uint8_t ttl, char* message) {
	/* add to queue */
	printf("added packet to queue \n");
	struct packet *packet = calloc(1, sizeof(struct packet));
	packet->dst_mip = dst_mip;
	packet->next_hop = next_hop;
	packet->src_mip = src_mip;
	packet->ttl = ttl;
	memcpy(packet->message, message, strlen(message));
	packet->timer = clock();
	add_data(root, packet);
}

ssize_t send_mip_packet(char *message, size_t msg_size, uint8_t dst_mip, uint8_t next_hop, 
			uint8_t src_mip, uint8_t ttl, struct interface **interfaces, int interf_count, int socket) {
	/* arp resolution */
	struct mip_header mip_hdr;
	struct ether_header eth_hdr;
	struct mip_arp *arp_entry = arp_resolution(next_hop, interfaces, interf_count, socket);
	if(arp_entry != NULL) {
		/* address exists on network - send message */
		struct interface* interface = check_interfaces(arp_entry->src_mip, interfaces, interf_count);
		/* send mip packet if interface/route is correct */
		if(interface != NULL) {
			if(src_mip == 0) {
				src_mip = interface->mip;
			}
			construct_mip_header(&mip_hdr, 1, 0, 0, ttl, dst_mip, src_mip, message);
			char *packet = construct_mip_packet(mip_hdr, message, msg_size);
			construct_ether_header(&eth_hdr, arp_entry->mac, interface);
			int result = send_ethernet_frame(packet, arp_entry->mac, interface);
			free(packet);
			return result;
		}
	}
	return 0;
}

void broadcast_mip_packet(char *message, size_t msg_size, 
			  struct interface **interfaces, int interf_count, int t, int r) {
	/* send out a mip packet on each interface with dest address 255 */
	int i = 0;
	while(i < interf_count) {
		uint8_t broadcast_address[] = ETH_BROADCAST_ADDRESS;
		struct mip_header mip_hdr;
		construct_mip_header(&mip_hdr, t, r, 0, 15, 255, interfaces[i]->mip, message);
		char *packet = construct_mip_packet(mip_hdr, message, msg_size);
		struct ether_header eth_hdr;
		construct_ether_header(&eth_hdr, broadcast_address, interfaces[i]);
		int result = send_ethernet_frame(packet, broadcast_address, interfaces[i]);
		free(packet);
		i++;
	}
}

ssize_t send_arp_response(uint8_t *mac_addr, uint8_t dest_addr, uint8_t src_addr, struct interface *interface) {
	if(debug_mode) {
		printf("Sending ARP response to MIP %d from %d \n", dest_addr, src_addr);
		printf("	%x:%x:%x:%x:%x:%x \n", 
				interface->mac.sll_addr[0],
				interface->mac.sll_addr[1],
				interface->mac.sll_addr[2],
				interface->mac.sll_addr[3],
				interface->mac.sll_addr[4],
				interface->mac.sll_addr[5]);
	}
	struct mip_header mip_hdr;
	construct_mip_header(&mip_hdr, 0, 0, 0, 15, dest_addr, src_addr, "");
	char *packet = construct_mip_packet(mip_hdr, "", 0);
	struct ether_header eth_hdr;
	construct_ether_header(&eth_hdr, mac_addr, interface);
	int result = send_ethernet_frame(packet, mac_addr, interface);
	free(packet);
	return result;
}

struct node* send_packets(struct node *root, uint8_t dst_mip, uint8_t next_hop, 
		  	  struct interface **interfaces, int interf_count, int socket) {
	/* send packets in message queue */
	struct node *current = root;
	while(current != NULL) {
		if(current->data != NULL) {
			struct packet *packet = (struct packet*)current->data;
			size_t size = sizeof(struct packet);
			clock_t diff = clock() - packet->timer;
			double sec = diff / CLOCKS_PER_SEC;
			/* check if timer timed out, or ttl is < 0*/
			if(sec >= 1 || packet->ttl < 0) {
				root = remove_node(root, packet, size);
			} else if(packet->dst_mip == dst_mip || packet->next_hop == next_hop) {
				int rc = send_mip_packet(packet->message, 
							 sizeof(packet->message), packet->dst_mip, 
							 next_hop, packet->src_mip, packet->ttl, 
							 interfaces, interf_count, socket);
				packet->next_hop = next_hop;
				if(rc != 0) {
					/* remove packet if it was sent */
					printf("size to remove: %d \n", size);
					root = remove_node(root, packet, size);
				}
			}
		}
		current = current->next_node;
	}
	return root;
}

int send_mip_addrs_rd(struct interface **interfaces, int interf_count, int route_info_sock) {
	/* send local mip addresses to routing daemon */
	uint8_t message[interf_count + 1];
	uint8_t type = 1;
	message[0] = type;
	int i = 1;
	while(i < interf_count + 1) {
		message[i] = interfaces[i-1]->mip;
		i++;
	}
	return send_on_dgram_domain_sock(route_info_sock, message, sizeof(message), rd_info_path);
}

int accept_packets(struct interface **interfaces, int interf_count, int application_socket) {
	int max_events = 10;
	struct epoll_event ev, events[max_events];
	int ethernet_sock, nfds, epollfd, route_request_sock, route_info_sock;

	/* setup sockets */
	ethernet_sock = ethernet_socket();

	route_info_sock = dgram_domain_socket(mipd_info_path);
	route_request_sock = dgram_domain_socket(mipd_request_path);

	/* create message queue */
	struct node *root = initialize_list();
	
	/* create epoll instance */
  	epollfd = epoll_create1(0);
  	if (epollfd == -1) {
  		perror("epoll_create1");
    		return 1;
  	}
	memset(&ev, 0, sizeof(struct epoll_event));
   	ev.events = EPOLLIN;
   	ev.data.fd = application_socket;
   	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, application_socket, &ev) == -1) {
       		perror("epoll_ctl: listen_sock");
       		exit(EXIT_FAILURE);
   	}
   	ev.data.fd = ethernet_sock;
   	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ethernet_sock, &ev) == -1) {
       		perror("epoll_ctl: ethernet_sock");
       		exit(EXIT_FAILURE);
   	}
   	ev.data.fd = route_request_sock;
   	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, route_request_sock, &ev) == -1) {
       		perror("epoll_ctl: route_request_sock");
       		exit(EXIT_FAILURE);
   	}
   	ev.data.fd = route_info_sock;
   	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, route_info_sock, &ev) == -1) {
       		perror("epoll_ctl: route_info_sock");
       		exit(EXIT_FAILURE);
   	}
	/* send local mip addresses to routing daemon */
	send_mip_addrs_rd(interfaces, interf_count, route_info_sock);
	while(1) {
		/* wait for events */
        	nfds = epoll_wait(epollfd, events, max_events, -1);
       		if (nfds == -1) {
                	perror("epoll_wait");
                	exit(EXIT_FAILURE);
       		}

		int i = 0;
               	while(i < nfds) {
			if(events[i].data.fd == application_socket) {
				/* if event on a domain socket - accept connection */
				int so = accept(events[i].data.fd, NULL, NULL);
				if (so == -1) {
					perror("accept connection on unix socket");
				  	break;
				}
				ev.data.fd = so;
				epoll_ctl(epollfd, EPOLL_CTL_ADD, so, &ev);
			} else if(events[i].data.fd == ethernet_sock) {
				/* if event on ethernet socket - deconstruct it */
				uint8_t buf[1500];
				memset(&buf, 0, 1500);
				struct sockaddr_ll sockaddr;
				int from = sizeof(sockaddr);
				int rc = recvfrom(events[i].data.fd, buf, 1500, 0, (struct sockaddr*)&sockaddr,
					          &from);
				int size_recv = rc - sizeof(struct ether_header) - sizeof(struct mip_header);
				char name[IF_NAMESIZE];
				if_indextoname(sockaddr.sll_ifindex, name);
				struct interface* rec_interf = name_to_interface(name, interfaces, interf_count);
				struct ether_header eth_hdr;
				char* eth_msg = deconstruct_ethernet_frame(buf, &eth_hdr);
				uint8_t broadcast_addr[] = ETH_BROADCAST_ADDRESS;
				int broadcast = memcmp(&eth_hdr.ether_dhost, &broadcast_addr, 
						sizeof(eth_hdr.ether_dhost)) == 0;
				struct mip_header mip_hdr;
				char *message = deconstruct_mip_packet(buf, &mip_hdr);
				struct interface* interface = check_interfaces(mip_hdr.dest, interfaces, 
							      		       interf_count);
				/* print communication information */
				print_communication_info(eth_hdr, mip_hdr);
				/* if broadcast packet received */
				if(broadcast == 1) {
					if(mip_hdr.t == 0 && mip_hdr.r == 1 && mip_hdr.a == 0) {
						printf("Received broadcast routing packet\n");
						printf("size received: %d \n", size_recv);
						send_on_dgram_domain_sock(route_info_sock, message, size_recv, 
									  rd_info_path);
					}
					if(mip_hdr.t == 0 && mip_hdr.r == 0 && mip_hdr.a == 1) {
						/* if arp packet - add to arp cache and send response */
						if(interface != NULL && interface->mip == mip_hdr.dest) {
							add_to_arp_cache(eth_hdr.ether_shost, mip_hdr.src, 
									 rec_interf->mip);
							send_arp_response(eth_hdr.ether_shost, mip_hdr.src,
								          mip_hdr.dest, rec_interf);
						}
					}
				/* if not broadcast packet received */
				} else {
					/* if targeted at local host */
					if(interface != NULL && interface->mip == mip_hdr.dest) {	
						/* if arp response */
						if(mip_hdr.t == 0 && mip_hdr.r == 0 && mip_hdr.a == 0) {
							printf("Received arp response \n");
							add_to_arp_cache(eth_hdr.ether_shost, mip_hdr.src, 
									 rec_interf->mip);
							root = send_packets(root, mip_hdr.src, mip_hdr.src, 
									    interfaces, interf_count, 
									    ethernet_sock);
						}
						/* if routing message */
						if(mip_hdr.t == 0 && mip_hdr.r == 1 && mip_hdr.a == 0) {
							printf("Received directed routing packet\n");
						}
						/* if transport packet */
						if(mip_hdr.t == 1 && mip_hdr.r == 0 && mip_hdr.a == 0) {
							/* forward to application */
							printf("Received transport message \n");
							printf("  message: %s \n", message);
							if(ev.data.fd > application_socket || 
							   ev.data.fd > ethernet_sock) {
								char buf[sizeof(message)];
								buf[0] = mip_hdr.src;
								memcpy(&buf[1], message, sizeof(message));
								if(send(ev.data.fd, buf, sizeof(message), 0) == -1) {
									perror("application send error");
									exit(-1);
								}
							}
						}
					/* if not targeted at local host */
					} else {
						/* forward packet */
						add_packet_to_queue(root, mip_hdr.dest, mip_hdr.dest, 
								    mip_hdr.src, mip_hdr.ttl-1, message);
						send_on_dgram_domain_sock(route_request_sock, &mip_hdr.dest, 
									  sizeof(uint8_t), rd_request_path);
						root = send_packets(root, mip_hdr.dest, 
								    mip_hdr.dest, interfaces, interf_count, 
								    ethernet_sock);
					}
				}
			} else {
				/* if event on unix domain socket */
				uint8_t buf[1500];
				memset(buf, 0, 1500);	
				int rc = read(events[i].data.fd, buf, 1500);

				if (rc > 0) {
					printf("Received message on unix socket \n");
					if(events[i].data.fd == route_request_sock) {
						printf("Message received on route request socket \n");
						printf("Received dest MIP %d \n", buf[0]);
						printf("Received next hop MIP %d \n", buf[1]);
						root = send_packets(root, buf[0], buf[1], 
								    interfaces, interf_count, ethernet_sock);
					} else if(events[i].data.fd == route_info_sock) {
						printf("Message received on route info socket \n");
						/* parse message */
						uint8_t type = buf[0];
						printf("received route table of size: %d \n", rc);
						if(type == 0) {
							broadcast_mip_packet(buf, rc, interfaces, interf_count, 0, 1);
						}
					} else if(events[i].data.fd > application_socket) {
						/* if event on an application connection */
						/* forward message to mip address */
						printf("Message received on application socket \n");
						uint8_t dst_addr = buf[0];
						char *message = &buf[1];
						add_packet_to_queue(root, dst_addr, dst_addr, 0, 15, message);
						send_on_dgram_domain_sock(route_request_sock, &buf[0], 
									  sizeof(uint8_t), rd_request_path);
						root = send_packets(root, dst_addr, dst_addr, interfaces, 
								    interf_count, ethernet_sock);
					}
				} else if (rc == 0) {
					/* disconnection */
					printf("Client disconnected\n");
					close(events[i].data.fd);
				} else {
					perror("read");
				}
			} 
			i++;
               	}
  	}	
	close(epollfd);
}

int main(int argc, char *argv[]) {
	if(argc < 4) {
		printf(HELP_MESSAGE);
		exit(1);
	} else if(strcmp(argv[1], "-h") == 0) {
		printf(HELP_MESSAGE);
		exit(1);
	} else {
		char* sock_path;
		char* identifier;
		uint8_t addrs[argc-3];
		uint8_t i;
		uint8_t c;
		if(strcmp(argv[1], "-d") == 0) {
			debug_mode = 1;
			sock_path = argv[2];
			identifier = argv[3];
			i = 4;
			c = 4;
			addrs[argc-c];
		} else { 
			/* -d was not supplied */
			sock_path = argv[1];
			identifier = argv[2];
			i = 3;
			c = 3;
			addrs[argc-c];
		}
		while(i < argc && argc < 255) {
			addrs[i-c] = (uint8_t)atoi(argv[i]);
			i++;
		}
		initialize_paths(identifier);
		int domain_socket = stream_domain_socket(sock_path);
		uint8_t interf_size = assign_mip_addresses(&interfaces, addrs, argc-c);
		accept_packets(interfaces, interf_size, domain_socket);
		close(domain_socket);
		free_ptr_arr((void**)interfaces, interf_size);
		if(arp_size != 0) {
			free_ptr_arr((void**)arp_cache, arp_size);
		}
	}
}
