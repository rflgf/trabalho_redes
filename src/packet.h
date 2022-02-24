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

// it may be the case that `payload` holds invalid memory, namely
// when `destination` is not this router instance (i.e, `struct
// router me`). maybe zero out or nullify the field so we don't
// move garbage around and lead to UB or safety issues.

struct deserialized {
	enum packet_type	type;
	router_id			source;
	router_id			destination;
	union payload		payload;
};

union packet {
	char  *serialized;
	struct deserialized;
};

struct queue_item {
	union packet	  *packet;
	struct queue_item *next;
};

struct packet_queue {
	struct queue_item *head;
	sem_t			   lock;
};

// returns a null-terminated string with at most 100 bytes
// and destroys the packet object passed as argument.
char *serialize(struct packet *packet);

// does not destroy anything.
struct packet *deserialize_header(char *serialized_packet);

// destroys the packet->payload.message.
struct packet *deserialize_payload(struct packet *packet);

// append packet to the start of the output queue
void enqueue_to_output(struct packet *packet);

// append packet to the start of the input queue
void enqueue_to_input(char *serialized_packet);

#define enqueue(X) _Generic((X),		\
	struct packet *: enqueue_to_output, \
			 char *: enqueue_to_input,  \
)(X)

// removes the last packet out of the queue
void dequeue(struct packet_queue *queue);

#endif
