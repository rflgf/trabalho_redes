#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "utils.h"
#include "packet.h"
#include "table_handler.h"


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

	pthread_mutex_lock(&me.packet_counter_mutex);
	serialized_packet[0] = me.packet_counter++;
	pthread_mutex_unlock(&me.packet_counter_mutex);

	switch (packet->deserialized.type)
	{
		case CONTROL:
			/*
				formatting as follows
				<packet_id>csource_virtual_addr
				b_virtual_addr b_cost
				c_virtual_addr c_cost
				...
			*/
			serialized_packet[1] = CONTROL;

			int index = 2;
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
				<packet_id>dsource_virtual_addr destination_virtual_addr
				a_virtual_addr a_cost
				b_virtual_addr b_cost
				c_virtual_addr c_cost
				...
			*/

			serialized_packet[1] = DATA;
			index = 2;
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

int deserialize_header(struct packet *packet)
{
	packet->deserialized.id		= packet->serialized[0];
	packet->deserialized.type	= packet->serialized[1];
	packet->deserialized.index	= 2;
	switch(packet->deserialized.type)
	{
		case CONTROL:
			packet->deserialized.destination = me.id; // control messages are only shared between neighbours.
			packet->deserialized.index += sscanf(&packet->serialized[2], "%d\n", &packet->deserialized.source);
			break;

		case DATA:
			packet->deserialized.index += sscanf(&packet->serialized[2], "%d %d\n", &packet->deserialized.source, &packet->deserialized.destination);
			break;
	}

	if (packet->deserialized.destination != me.id)
	{
		struct table_item *table = calculate_table();
		struct table_item *t = get_table_item_by_destination(packet->deserialized.destination, table);
		char *packet_id = evaluate_packet_id(packet);
		if (!t)
		{
			info("impossível encontrar caminho\npara %d, descartando pacote #%s\nde %d para %d.",
				packet->deserialized.destination,
				packet_id,
				packet->deserialized.source,
				packet->deserialized.destination);
			free(packet_id);
			free(packet->serialized);
			free(packet);
			free(t);
			free_table(table);
			return -1;
		}
		packet->deserialized.next_hop = t->next_hop;
		debug("calculei next hop para pacote #%s de %d para %d como %d",
			packet_id,
			packet->deserialized.source,
			packet->deserialized.destination,
			packet->deserialized.next_hop);
		free(packet_id);
	}
	return 0;
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
		#ifdef INFO
		struct packet p;
		p.serialized = serialized_packet;
		deserialize_header(&p);
		char *packet_id = evaluate_packet_id(&p);
		info("fila de entrada cheia, descartando pacote #%s de %d", packet_id, p.deserialized.source);
		free(packet_id);
		#endif
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
		char *packet_id = evaluate_packet_id(packet);
		info("fila de saída cheia, descartando pacote #%s de %d", packet_id, packet->deserialized.source);
		free(packet_id);
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

char *evaluate_packet_id(struct packet *packet)
{
	char *buff = malloc(20); // lol
	sprintf(buff, "%u-%d", packet->deserialized.id, packet->deserialized.source);
	return buff;
}
