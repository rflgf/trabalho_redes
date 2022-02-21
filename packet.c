#include "packet.h"

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
// and destroys the packet object passed as argument.
char *serialize(struct packet *packet)
{
	char *serialized_packet = calloc(PAYLOAD_MAX_LENGTH, sizeof(char));

	switch (packet->type)
	{
		case control:
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
			struct distance_vector dv = packet->distance;
			while (distance_vector)
			{
				prior_index = index;
				index += sprintf(&serialized_packet[index], "%d %d\n", distance_vector->router_id, distance_vector->distance);
				distance_vector = distance_vector->next;

				// if the content is bigger than 98 chars, we
				// simply ignore the rest of it.
				if (index >= PAYLOAD_MAX_LENGTH - 2)
				{
					serialized_packet[prior_index + 1] = '\0';
					break;
				}
			}

			free_distance_vector(packet->distance);
			break;

		case data;
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
			memcpy(&serialized_packet[1], packet->payload->message, PAYLOAD_MAX_LENGTH - 1);
			free(packet->message);
			break;
	}

	free(packet);
	return serialized_packet;
}

// destroys the string passed as argument.
struct packet *deserialize(char *serialized_packet)
{
	struct packet *deserialized_packet = malloc(sizeof(struct packet));
	int index = sscanf(serialized_packet[1], "%d %d", &deserialized_packet->source, &deserialized_packet->destination);

	switch (serialized_packet[0])
	{
		case 'c':
			deserialized_packet->type = control;

			while (index < PAYLOAD_MAX_LENGTH - 1)
			{
				if (serialized_packet[index + 1] == '\0')
					break;

				struct distance_vector *dv = malloc(sizeof(distance_vector));
				index += sscanf(serialized_packet[index], "%d %d", &distance_vector->virtual_address, &distance_vector->distance);

				distance_vector->next = NULL;
			}
			break;

		case 'd':
			deserialized_packet->type = data;
			strcpy(&deserialized_packet->payload->message, &serialized_packet[1]);
			break;

		default:
			die("header do pacote corrompido.");
	}

	free(serialized_packet);
	return deserealized_packet;
}
