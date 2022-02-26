#ifndef ROUTER
#define ROUTER

#include <time.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "table_handler.h"
#include "packet.h"

typedef int router_id;
typedef int cost;

struct table_item {
	router_id		   destination;
	router_id		   neighbouring_router; /* the neighbouring router used to get to `destination`. */
	cost			   cost;
	struct table_item *next;
};

struct 	table {
	struct table_item *items;
};

struct link {
	router_id			id;
	cost				cost_to;
	bool				enabled;
	time_t				last_heard_from;
	struct sockaddr_in	socket;
	struct link		   *next;
};

struct router {
	router_id id;
	bool	  enabled;

	struct packet_queue  input;
	struct packet_queue  output;

	struct table		 table;

	struct link			*neighbouring_routers;

	struct sockaddr_in	*udp_socket;
	int					 file_descriptor;
};

struct router me;

void create_socket();
void destroy_socket();
#endif
