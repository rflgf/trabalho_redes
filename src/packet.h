#ifndef	PACKET
#define PACKET
#include <semaphore.h>

#include "utils.h"

#define PAYLOAD_MAX_LENGTH 100

typedef int router_id;

enum packet_type {
	CONTROL,
	DATA
};

struct distance_vector {
	router_id				virtual_address;
	cost					distance; // a.k.a. cost
	struct distance_vector *next;
};

union payload {
	char				   *message; /* string. */
	struct distance_vector *distance;
};

struct packet {
	enum packet_type	type;
	router_id			source;
	router_id			destination;
	union payload		payload;
};

struct queue_item {
	struct packet	  *packet;
	struct queue_item *next;
};

struct packet_queue {
	struct queue_item *head;
	sem_t			   lock;
};

// returns a null-terminated string with at most 100 bytes
// and destroys the packet object passed as argument.
char *serialize(struct packet *packet);

// destroys the string passed as argument.
struct packet *deserialize(char *serialized_packet);


// pushes packet to queue
void push(struct packet_queue *queue, struct packet *packet);

// pops the last packet out of the queue
void pop(struct packet_queue *queue);

#endif
