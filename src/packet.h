#ifndef	PACKET
#define PACKET
#include "router.h"
#include "utils.h"

#define PAYLOAD_MAX_LENGTH 100

typedef int virtual_address;

enum packet_type {
	CONTROL,
	DATA
};

struct distance_vector {
	virtual_address			virtual_address;
	cost					distance; // a.k.a. cost
	struct distance_vector *next;
};

union payload {
	char				   *message; /* string. */
	struct distance_vector *distance;
};

struct packet {
	enum packet_type	type;
	virtual_address		source;
	virtual_address		destination;
	union payload		payload;
};

struct queue_item {
	struct packet	  *packet;
	struct queue_item *next;
};

struct packet_queue {
	struct queue_item *head;

};

// returns a null-terminated string with at most 100 bytes
// and destroys the packet object passed as argument.
char *serialize(struct packet *packet);

// destroys the string passed as argument.
struct packet *deserialize(char *serialized_packet);

#endif
