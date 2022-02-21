#ifndef ROUTER
#define ROUTER

#include <arpa/inet.h>
#include <sys/socket.h>

// forward decls
struct packet_queue;

typedef int router_id;
typedef int cost;

/*
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct in_addr {
    unsigned long s_addr;          // load with inet_pton()
};
*/

struct link {
	router_id	 neighbour_id;
	cost		 cost_to_neighbour;
	struct link *next;
};

struct router {
	router_id id;

	struct packet_queue *input;
	struct packet_queue *output;

	struct table		*table;

	struct link			*neighbouring_routers;

	struct sockaddr_in	*udp_socket;
};

struct router me;

void create_socket();
void destroy_socket();
#endif
