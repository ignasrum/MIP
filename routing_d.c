#include "routing_d.h"

void print_routing_table() {
	printf("CURRENT ROUTING TABLE: \n");
	struct node *current = routing_table;
	while(current != NULL) {
		printf("	ENTRY: \n");
		struct r_table_entry *entry = (struct r_table_entry*)current->data;
		printf("		Destination mip: %d \n", entry->dst_mip);
		printf("		Next hop mip: %d \n", entry->next_hop);
		printf("		Source mip: %d \n", entry->src_mip);
		printf("		Weight: %d \n", entry->weight);
		current = current->next_node;
	}
}

uint8_t find_next_hop(uint8_t dest_mip) {
	struct node *current = routing_table;
	while(current != NULL) {
		struct r_table_entry *entry = (struct r_table_entry*)current->data;
		if(entry && entry->dst_mip == dest_mip) {
			return entry->next_hop;
		}
		current = current->next_node;
	}
	return 255;
}

int add_routing_table_entry(struct r_table_entry *new_entry) {
	struct node *current = routing_table;
	while(current != NULL) {
		struct r_table_entry *entry = (struct r_table_entry*)current->data;
		/* if entry with same destination address already exists */
		if(entry && entry->dst_mip == new_entry->dst_mip) {
			/* compare weight */
			if(entry->weight < new_entry->weight) {
				/* if previous entry has lower weight */
				/* discard new entry, keep previous */
				free(new_entry);
				return 1;
			} else {
				/* remove previous entry and add new entry */
				routing_table = remove_node(routing_table, entry, sizeof(struct r_table_entry));
				current = routing_table;
			}
		}
		current = current->next_node;
	}
	routing_table = add_data(routing_table, new_entry);
	return 0;
}

int update_routing_table(char* buf, int buf_size, uint8_t type) {
	/* if local addresses from mip daemon */
	if(type == 1) {
		printf("Received mip addresses from mipd \n");
		int i = 0;
		while(i < buf_size - 1) {
			/* construct entry */
			uint8_t mip = buf[i+1];
			printf("	MIP Address: %d \n", mip);
			struct r_table_entry *new_entry = calloc(1, sizeof(struct r_table_entry));
			new_entry->dst_mip = mip;
			new_entry->next_hop = mip;
			new_entry->src_mip = mip;
			new_entry->weight = 0;
			/* add entry */
			add_routing_table_entry(new_entry);
			i++;
		}
	/* if routing table from another routing daemon */
	} else if(type == 0) {
		printf("Received routing table \n");
		printf("size of routing_table: %d \n", buf_size);
		int i = 1;
		while(i < buf_size - 1) {
			/* construct entry */
			uint8_t dst_mip = buf[i];
			i++;
			uint8_t next_hop = buf[i];
			i++;
			uint8_t src_mip = buf[i];
			i++;
			uint8_t weight = buf[i];
			i++;
			uint8_t local_mip = ((struct r_table_entry*)routing_table->data)->src_mip;
			struct r_table_entry *new_entry = calloc(1, sizeof(struct r_table_entry));
			new_entry->dst_mip = dst_mip;
			new_entry->next_hop = src_mip;
			new_entry->src_mip = local_mip;
			new_entry->weight = weight + 1;
			/* add entry */
			add_routing_table_entry(new_entry);
		}
	}
	print_routing_table();
	return 1;
}

int send_routing_table(int route_info_sock) {
	/* send routing table to neighbouring routing daemons */
	uint8_t type = 0;
	size_t size = (list_size(routing_table) * 4) + 1;
	int empty = 1;
	if(size > 0) {
		char buffer[size];
		buffer[0] = type;
		int i = 1;
		struct node *current = routing_table;
		while(current != NULL) {
			struct r_table_entry *entry = (struct r_table_entry*)current->data;
			if(entry) {
				/* if routing table is not empty */
				empty = 0;
				buffer[i] = entry->dst_mip;
				i++;
				buffer[i] = entry->next_hop;
				i++;
				buffer[i] = entry->src_mip;
				i++;
				buffer[i] = entry->weight;
				i++;
			}
			current = current->next_node;
		}
		if(empty == 0) {
			printf("Sending routing table of size: %d \n", size);
			send_on_dgram_domain_sock(route_info_sock, buffer, size, mipd_info_path);
		}
	}
	return 0;
}

void *threaded_send_r_table(void *vargp) {
	/* sleep 5 seconds, then send routing table to neighbouring routing daemons */
	int *route_info_sock = (int*)vargp;
	while(1) {
		sleep(5);
		send_routing_table(*route_info_sock);
	}
}

int accept_packets(int route_info_sock, int route_request_sock) {
	int max_events = 10;
	struct epoll_event ev, events[max_events];
	int nfds, epollfd;

	/* create epoll instance */
  	epollfd = epoll_create1(0);
  	if (epollfd == -1) {
  		perror("epoll_create1");
    		return 1;
  	}
	memset(&ev, 0, sizeof(struct epoll_event));
   	ev.events = EPOLLIN;
   	ev.data.fd = route_info_sock;
   	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, route_info_sock, &ev) == -1) {
       		perror("epoll_ctl: route_info_sock");
       		exit(EXIT_FAILURE);
   	}
   	ev.data.fd = route_request_sock;
   	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, route_request_sock, &ev) == -1) {
       		perror("epoll_ctl: route_request_sock");
       		exit(EXIT_FAILURE);
   	}
	while(1) {
		/* wait for events */
        	nfds = epoll_wait(epollfd, events, max_events, -1);
       		if (nfds == -1) {
                	perror("epoll_wait");
                	exit(EXIT_FAILURE);
       		}

		int i = 0;
               	while(i < nfds) {
			uint8_t buf[1500];
			memset(buf, 0, 1500);	
			int rc = read(events[i].data.fd, buf, 1500);

			if(events[i].data.fd == route_info_sock) {
				/* if event on route info socket */
				printf("Message on route info socket \n");
				uint8_t type = buf[0];
				update_routing_table(buf, rc, type);
			} else if(events[i].data.fd == route_request_sock) {
				/* if event on route request socket */
				printf("Message on route request socket \n");
				uint8_t mip;
				memcpy(&mip, &buf[0], sizeof(uint8_t));
				printf("Received MIP %d \n", mip);
				uint8_t next_hop = find_next_hop(mip);
				if(next_hop != 255) {
					uint8_t to_send[2] = {mip, next_hop};
					send_on_dgram_domain_sock(route_request_sock, to_send, 2, mipd_request_path);
				}
			}
			i++;
               	}
  	}	
	close(epollfd);
}

int main(int argc, char* argv[]) {
	char* identifier;
	if(argc < 2) {
		printf(HELP_MESSAGE);
		exit(1);
	}
	if(argv[1] && strcmp(argv[1], "-h") == 0) {
		printf(HELP_MESSAGE);
		exit(1);
	} else {
		identifier = argv[1];
	}
	initialize_paths(identifier);
	routing_table = initialize_list();
	char* buffer = "ROUTE_INFO";
	/* setup sockets */	
	int route_info_sock = dgram_domain_socket(rd_info_path);
	int route_request_sock = dgram_domain_socket(rd_request_path);
	pthread_t thread;
	pthread_create(&thread, NULL, threaded_send_r_table, &route_info_sock);
	accept_packets(route_info_sock, route_request_sock);
	pthread_join(thread, NULL);
	free_linked_list(routing_table);
	return 0;
}
