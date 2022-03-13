#ifndef ROUTER
#define ROUTER

#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "table_handler.h"
#include "packet.h"

typedef int router_id;
typedef int cost;

struct table_item {
	router_id		   destination;
	router_id		   next_hop; /* the neighbouring router used to get to `destination`. */
	cost			   cost;
	struct table_item *next;
};

struct link {
	router_id			id;
	cost				cost_to;
	bool				enabled;
	time_t				last_heard_from;
	struct in_addr		ip_address; // this may not be useful?
	struct sockaddr_in	socket;
	struct link		   *next;
	struct distance_vector *last_dv;
};

struct router {
	router_id id;
	bool	  enabled;

	struct packet_queue  input;
	struct packet_queue  output;

	struct link			*neighbouring_routers;

	struct in_addr		 ip_address; // this may not be useful?
	struct sockaddr_in	*udp_socket;
	int					 file_descriptor;

	pthread_mutex_t		 mutex;
	pthread_mutex_t		 terminal_mutex;
};

struct router me;

void initialize_router(int id, unsigned short int port, char *ip_address);

void create_socket();
void destroy_socket();
#endif
