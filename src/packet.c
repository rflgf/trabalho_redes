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
	// since this function is only called on packets
	// constructed in this router we can create a new packet
	// id for each and all of them.
	serialized_packet[0] = ++me.packet_counter;
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
			debug("serialized packet before read destination read on control case [%s]", serialized_packet);
			index += sprintf(&serialized_packet[index], "%d\n", me.id);
			debug("serialized packet after read destination read on control case [%s]", serialized_packet);

			struct distance_vector *dv;
			for (dv = packet->deserialized.payload.distance; dv; dv = dv->next)
			{
				int prior_index = index;
				int written_chars;
				sprintf(&(serialized_packet[index]), "%d %d\n%n", dv->virtual_address, dv->distance, &written_chars);
				index += written_chars;

				// if the content is bigger than 98 chars, we
				// simply ignore the rest of it.
				// in a real impl we would have to split the
				// content into multiple packets.
				if (index >= PAYLOAD_MAX_LENGTH)
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
			int written_chars;
			sprintf(&serialized_packet[index], "%d %d\n%n", me.id, packet->deserialized.destination, &written_chars);
			index += written_chars;

			// payload can't take up the last and first char of the packet.
			int len = strlen(packet->deserialized.payload.message);
			debug("len is %d", len);
			memcpy(&serialized_packet[index], &packet->deserialized.payload.message[0], len > PAYLOAD_MAX_LENGTH? PAYLOAD_MAX_LENGTH: len);
			if (destroy)
				free(packet->deserialized.payload.message);

			break;
		default:
			debug("this [%c] is the packet type flag", packet->deserialized.type);
	}

	packet->serialized = serialized_packet;

	if (destroy)
		free(packet);
	debug("serialized packet as [%s].", serialized_packet);
	return serialized_packet;
}

int deserialize_header(struct packet *packet)
{
	packet->deserialized.id		= packet->serialized[0];
	packet->deserialized.type	= packet->serialized[1];
	packet->deserialized.index	= 2;
	int read_chars;

	switch(packet->deserialized.type)
	{
		case CONTROL:
			packet->deserialized.destination = me.id; // control messages are only shared between neighbours.
			sscanf(&packet->serialized[2], "%d\n%n", &packet->deserialized.source, &read_chars);
			packet->deserialized.index += read_chars;
			break;

		case DATA:
			sscanf(&packet->serialized[2], "%d %d\n%n", &packet->deserialized.source, &packet->deserialized.destination, &read_chars);
			packet->deserialized.index += read_chars;
			break;
		default:
			debug("trying to deserialize header but packet type is [%c]", packet->deserialized.type);
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
	debug("deserialized header as type [%c], origin [%d], destination [%d] and next hop [%d]", packet->deserialized.type, packet->deserialized.source, packet->deserialized.destination, packet->deserialized.next_hop);
	return 0;
}

void deserialize_payload(struct packet *packet)
{
	int index = packet->deserialized.index;
	debug("imma deserialize the payload from %d onward", packet->deserialized.index);
	int num_chars_read;
	switch (packet->deserialized.type)
	{
		case CONTROL:
			packet->deserialized.payload.distance = NULL;
			while (index < PAYLOAD_MAX_LENGTH - 1)
			{
				if (packet->serialized[index + 1] == '\0')
				{
					debug("stopped reading the payload cuz it was shorter than the maximum");
					debug("index was %d", index);
					break;
				}

				struct distance_vector *dv = malloc(sizeof(struct distance_vector));
				sscanf(&packet->serialized[index], "%d %d\n%n", &dv->virtual_address, &dv->distance, &num_chars_read);
				index += num_chars_read;
				debug("ok so i read %d and %d", dv->virtual_address, dv->distance);
				assert(dv);

				dv->next = NULL;

				enqueue_to_distance_vector(dv, &packet->deserialized.payload.distance);
			}
			packet->deserialized.index = index;
			assert(packet->deserialized.payload.distance);
			print_distance_vector(packet->deserialized.payload.distance);
			break;

		case DATA:
			packet->deserialized.payload.message = &packet->serialized[2];
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
		debug("deserializing header on enqueue_to_input");
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
		// free(packet->serialized);
		if (packet->deserialized.type == CONTROL)
			free_distance_vector(packet->deserialized.payload.distance);
		free(packet);
		return;
	}
	// @TODO keep going from here

	struct queue_item *qi = me.output.head;
	struct queue_item *new_queue_item = malloc(sizeof(struct queue_item));
	new_queue_item->packet = packet;
	new_queue_item->next   = NULL;
	if (!qi)
		me.output.head = new_queue_item;
	else
	{
		while (qi->next)
			qi = qi->next;
		qi->next = new_queue_item;
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
	assert(ret);
	return ret;
}

char *evaluate_packet_id(struct packet *packet)
{
	char *buff = malloc(20); // lol
	sprintf(buff, "%u-%d", packet->deserialized.id, packet->deserialized.source);
	return buff;
}
