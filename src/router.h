#ifndef ROUTER
#define ROUTER

#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "packet.h"

typedef int router_id;
typedef int cost;

struct link {
	router_id		   id;
	cost			   cost_to;
	bool			   enabled;
	struct sockaddr_in socket;
	struct link		  *next;
};

struct router {
	router_id id;
	bool	  enabled;

	struct packet_queue  input;
	struct packet_queue  output;

	struct table		*table;

	struct link			*neighbouring_routers;

	struct sockaddr_in	*udp_socket;
};

struct router me;

void create_socket();
void destroy_socket();
#endif
