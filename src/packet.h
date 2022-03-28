#ifndef	PACKET
#define PACKET
#include <semaphore.h>
#include <stdbool.h>

#define PAYLOAD_MAX_LENGTH 100

typedef unsigned char packet_id;
typedef int router_id;
typedef int cost;

enum __attribute__((packed)) packet_type {
	CONTROL = 'c',
	DATA	= 'd'
};

struct distance_vector {
	router_id				virtual_address;
	cost					distance; // a.k.a. cost
	router_id				next_hop;
	struct distance_vector *next;
};

union payload {
	char				   *message; /* string. */
	struct distance_vector *distance;
};

// it may be the case that `payload` holds invalid memory, namely
// when this router instance is not `destination` (i.e, `struct
// router me`). maybe zero out or nullify `payload` so we don't
// move garbage around, leading to UB or safety issues.
struct deserialized {
	packet_id			id;
	enum packet_type	type;
	router_id			source;
	router_id			destination;
	router_id			next_hop;
	int					index;
	union payload		payload;
};

struct packet {
	char *serialized;
	struct deserialized deserialized;
};

struct queue_item {
	struct packet	  *packet;
	struct queue_item *next;
};

struct packet_queue {
	struct queue_item *head;
	int				   current_size;
	pthread_mutex_t	   mutex;
	sem_t			   semaphore; // at most MAX_QUEUE_ITEMS.
};

#include "table_handler.h"
#include "utils.h"

// returns a null-terminated string with at most 100 bytes
// and destroys the packet object passed as argument if
// `destroy` is true.
char *serialize(struct packet *packet, bool destroy);

// does not destroy anything.
int deserialize_header(struct packet *serialized_packet);

// meant to be used on the product of a `deserialize_header` call.
void deserialize_payload(struct packet *packet);

// append packet to the start of the output queue
void enqueue_to_output(struct packet *packet);

// append packet to the start of the input queue
void enqueue_to_input(char *serialized_packet);

#define enqueue(X) _Generic((X),		\
	struct packet *: enqueue_to_output, \
			 char *: enqueue_to_input   \
)(X)

// removes the last packet out of the queue
struct packet *dequeue(struct packet_queue *queue);

char *evaluate_packet_id(struct packet *packet);

void free_distance_vector(struct distance_vector *dv);

void free_packet();
#endif
