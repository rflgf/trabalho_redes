#ifndef	PACKET
#define PACKET
#include <semaphore.h>
#include <stdbool.h>

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
// when this router instance is not `destination` (i.e, `struct
// router me`). maybe zero out or nullify `payload` so we don't
// move garbage around, leading to UB or safety issues.
struct deserialized {
	enum packet_type	type;
	router_id			source;
	router_id			destination;
	int					index;
	union payload		payload;
};

union packet {
	char *serialized;
	struct deserialized deserialized;
};

struct queue_item {
	union packet	  *packet;
	struct queue_item *next;
};

struct packet_queue {
	struct queue_item *head;
	int				   current_size;
	pthread_mutex_t	   mutex;
	sem_t			   semaphore; // at most MAX_QUEUE_ITEMS.
};

// returns a null-terminated string with at most 100 bytes
// and destroys the packet object passed as argument if
// `destroy` is true.
char *serialize(union packet *packet, bool destroy);

// does not destroy anything.
void deserialize_header(union packet *serialized_packet);

// meant to be used on the product of a `deserialize_header` call.
void deserialize_payload(union packet *packet);

// append packet to the start of the output queue
void enqueue_to_output(union packet *packet);

// append packet to the start of the input queue
void enqueue_to_input(char *serialized_packet);

#define enqueue(X) _Generic((X),	   \
	union packet *: enqueue_to_output, \
			char *: enqueue_to_input   \
)(X)

// removes the last packet out of the queue
union packet *dequeue(struct packet_queue *queue);

void free_distance_vector(struct distance_vector *dv);

void free_packet();
#endif
