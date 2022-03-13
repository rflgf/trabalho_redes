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
char *serialize(struct packet *packet, bool destroy)
{
	char *serialized_packet = calloc(PAYLOAD_MAX_LENGTH, sizeof(char));

	switch (packet->deserialized.type)
	{
		case CONTROL:
			/*
				formatting as follows
				csource_virtual_addr
				b_virtual_addr b_cost
				c_virtual_addr c_cost
				...
			*/
			serialized_packet[0] = CONTROL;

			int index = 1;
			index += sprintf(&serialized_packet[index], "%d\n", me.id);
			struct distance_vector *dv = packet->deserialized.payload.distance;
			while (dv)
			{
				int prior_index = index;
				index += sprintf(&serialized_packet[index], "%d %d\n", dv->virtual_address, dv->distance);
				dv = dv->next;

				// if the content is bigger than 98 chars, we
				// simply ignore the rest of it.
				// in a real impl we would have to split the
				// content into multiple packets.
				if (index >= PAYLOAD_MAX_LENGTH - 2)
				{
					serialized_packet[prior_index] = '\0';
					break;
				}
			}


			if (destroy)
				free_distance_vector(packet->deserialized.payload.distance);
			break;

		case DATA:
			/*
				formatting as follows
				dsource_virtual_addr destination_virtual_addr
				a_virtual_addr a_cost
				b_virtual_addr b_cost
				c_virtual_addr c_cost
				...
			*/

			serialized_packet[0] = DATA;
			index = 1;
			index += sprintf(&serialized_packet[index], "%d %d\n", packet->deserialized.source, packet->deserialized.destination);

			// payload can't take up the last and first char of the packet.
			memcpy(&serialized_packet[index], &packet->deserialized.payload.message[0], strlen(packet->deserialized.payload.message) - index);
			if (destroy)
				free(packet->deserialized.payload.message);

			break;
	}

	packet->serialized = serialized_packet;

	if (destroy)
		free(packet);
	return serialized_packet;
}

void deserialize_header(struct packet *packet)
{
	packet->deserialized.type = packet->serialized[0];
	packet->deserialized.index = 1;
	switch(packet->deserialized.type)
	{
		case CONTROL:
			packet->deserialized.destination = me.id;
			packet->deserialized.index += sscanf(&packet->serialized[1], "%d\n", &packet->deserialized.source);
			break;

		case DATA:
			packet->deserialized.index += sscanf(&packet->serialized[1], "%d %d\n", &packet->deserialized.source, &packet->deserialized.destination);
			break;
	}

}

void deserialize_payload(struct packet *packet)
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
				index += sscanf(&packet->serialized[index], "%d %d\n", &dv->virtual_address, &dv->distance);

				dv->next = NULL;
			}
			break;

		case DATA:
			packet->deserialized.payload.message = &packet->serialized[1];
			break;
	}
}

void enqueue_to_input(char *serialized_packet)
{
	pthread_mutex_lock(&me.input.mutex);

	if (me.input.current_size >= MAX_QUEUE_ITEMS)
	{
		printf("fila de input cheia, descartando pacote #...\n");
		pthread_mutex_unlock(&me.input.mutex);
		return;
	}

	if (!me.input.head)
	{
		struct queue_item *new_head = malloc(sizeof(struct queue_item));
		new_head->packet = calloc(1, sizeof(struct packet));
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
		new->packet = malloc(sizeof(struct packet));
		new->packet->serialized = serialized_packet;
	}

	me.input.current_size++;
	pthread_mutex_unlock(&me.input.mutex);
	sem_post(&me.input.semaphore);
}

void enqueue_to_output(struct packet *packet)
{
	pthread_mutex_lock(&me.output.mutex);

	if (me.output.current_size >= MAX_QUEUE_ITEMS)
	{
		pthread_mutex_unlock(&me.output.mutex);
		pthread_mutex_lock(&me.terminal_mutex);
		info("fila de saída cheia, descartando pacote #...");
		pthread_mutex_unlock(&me.terminal_mutex);
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

struct packet *dequeue(struct packet_queue *queue)
{
	sem_wait(&queue->semaphore);

	pthread_mutex_lock(&queue->mutex);

	struct queue_item *qi = queue->head;
	queue->head = qi->next;
	if (!qi || queue->current_size < 0)
		die("tentativa de dequeue em fila vazia");

	queue->current_size--;
	struct packet *ret = qi->packet;
	free(qi);
	if (queue->current_size == 0)
		queue->head = NULL;
	pthread_mutex_unlock(&queue->mutex);
	//assert(ret);
	return ret;
}
