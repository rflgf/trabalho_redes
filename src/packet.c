#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "utils.h"
#include "packet.h"

typedef int router_id;
typedef int cost;

// ------ forward decls -------
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
// ---- end forward decls -----

void free_distance_vector(struct distance_vector *dv)
{
	while (dv)
	{
		struct distance_vector *aux = dv->next;
		free(dv);
		dv = aux;
	}
}

// returns a null-terminated string with at most 100 bytes
// and destroys the packet object passed as argument if
// `destroy` is true.
char *serialize(union packet *packet, bool destroy)
{
	char *serialized_packet = calloc(PAYLOAD_MAX_LENGTH, sizeof(char));

	switch (packet->deserialized.type)
	{
		case CONTROL:
			/*
				formatting as follows
				c source_virtual_addr destination_virtual_addr
				a_virtual_addr a_cost
				b_virtual_addr b_cost
				c_virtual_addr c_cost
				...
			*/
			serialized_packet[0] = 'c';

			// payload can't take up the last or first few chars of the packet.
			int index = 1;
			struct distance_vector *dv = packet->deserialized.payload.distance;
			while (dv)
			{
				int prior_index = index;
				index += sprintf(&serialized_packet[index], "%d %d\n", dv->virtual_address, dv->distance);
				dv = dv->next;

				// if the content is bigger than 98 chars, we
				// simply ignore the rest of it.
				if (index >= PAYLOAD_MAX_LENGTH - 2)
				{
					serialized_packet[prior_index + 1] = '\0';
					break;
				}
			}

			free_distance_vector(packet->deserialized.payload.distance);
			break;

		case DATA:
			/*
				formatting as follows
				d source_virtual_addr destination_virtual_addr
				a_virtual_addr a_cost
				b_virtual_addr b_cost
				c_virtual_addr c_cost
				...
			*/

			serialized_packet[0] = 'd';

			// payload can't take up the last and first char of the packet.
			memcpy(&serialized_packet[1], packet->deserialized.payload.message, PAYLOAD_MAX_LENGTH - 1);
			if (destroy)
				free(packet->deserialized.payload.message);

			break;
	}

	if (destroy)
		free(packet);
	return serialized_packet;
}

void deserialize_header(union packet *packet)
{
	packet->deserialized.type = packet->serialized[0] == 'c'? CONTROL: DATA;

	sscanf(&packet->serialized[1], "%d %d", &packet->deserialized.source, &packet->deserialized.destination);
}

void deserialize_payload(union packet *packet)
{
	int index = packet->deserialized.index;
	switch (packet->deserialized.type)
	{
		case CONTROL:

			while (index < PAYLOAD_MAX_LENGTH - 1)
			{
				if (packet->serialized[index + 1] == '\0')
					break;

				struct distance_vector *dv = malloc(sizeof(struct distance_vector));
				index += sscanf(&packet->serialized[index], "%d %d", &dv->virtual_address, &dv->distance);

				dv->next = NULL;
			}
			break;

		default:
			packet->deserialized.payload.message = &packet->serialized[1];
	}
}

void enqueue_to_input(char *serialized_packet)
{
	pthread_mutex_lock(&me.input.mutex);

	if (me.input.current_size >= MAX_QUEUE_ITEMS)
	{
		printf("fila de input cheia, descartando pacote...\n");
		pthread_mutex_unlock(&me.input.mutex);
		return;
	}

	if (!&me.input.head)
	{
		struct queue_item *new_head = malloc(sizeof(struct queue_item));
		new_head->packet->serialized = serialized_packet;
		new_head->next = NULL;
		me.input.head = new_head;
	}

	else
	{
		struct queue_item *qi = me.input.head;

		while (qi->next)
			qi = qi->next;

		struct queue_item *new = malloc(sizeof(struct queue_item));
		new->next = NULL;
		qi->next = new;
		new->packet->serialized = serialized_packet;
	}

	me.input.current_size++;
	pthread_mutex_unlock(&me.input.mutex);
	sem_post(&me.input.semaphore);
}

void enqueue_to_output(union packet *packet)
{
	pthread_mutex_lock(&me.output.mutex);

	if (me.output.current_size >= MAX_QUEUE_ITEMS)
	{
		printf("fila de output cheia, descartando pacote...\n");
		pthread_mutex_unlock(&me.output.mutex);
		return;
	}

	struct queue_item *qi = me.output.head;
	if (!qi)
	{
		struct queue_item *new_queue_item = malloc(sizeof(struct queue_item));
		new_queue_item->packet = packet;
		new_queue_item->next   = NULL;
		me.output.head		   = new_queue_item;
	}
	else
	{
		while (qi->next)
			qi = qi->next;

		struct queue_item *new_queue_item = malloc(sizeof(struct queue_item));
		new_queue_item->next   = NULL;
		new_queue_item->packet = packet;
		qi->next			   = new_queue_item;
	}

	me.output.current_size++;

	pthread_mutex_unlock(&me.output.mutex);
	sem_post(&me.output.semaphore);
}

union packet *dequeue(struct packet_queue *queue)
{
	sem_wait(&queue->semaphore);

	pthread_mutex_lock(&queue->mutex);
	queue->current_size--;

	struct queue_item *qi = queue->head;
	if (!qi || queue->current_size < 0)
		die("tentativa de dequeue em fila vazia");
	else
		while (qi->next)
		{
			if (!qi->next->next)
				qi->next->next = NULL;
			qi = qi->next;
		}

	union packet *ret = qi->packet;
	free(qi);
	pthread_mutex_unlock(&queue->mutex);
	return ret;
}
