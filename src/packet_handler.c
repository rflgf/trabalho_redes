#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "packet_handler.h"
#include "router.h"
#include "packet.h"

void *packet_handler_f(void *arg)
{
	// unlocks on input queue > 0
	// deserializes data and
	// - if im the destination
	// - - if control, do the control thing
	// - - if message, print out to the terminal
	// - else put it in the output queue

	while (true)
	{
		union packet* p = dequeue(&me.input);
		deserialize_header(p);

		if (p->deserialized.destination == me.id)
		{
			switch (p->deserialized.type)
			{
				case CONTROL:
					deserialize_payload(p);
					// @TODO check whether this is leaking memory.
					// we may need to free p.
					evaluate_distance_vector(p->deserialized.source, p->deserialized.payload.distance);
					break;

				default:
					deserialize_payload(p);
					char *message = p->deserialized.payload.message;

					printf("mensagem de [%d] para [%d]:\n", p->deserialized.source, p->deserialized.destination);
					printf("\t%s\n", message);

					free(p->deserialized.payload.message);
			}
					free(p->serialized);
					free(p);
		}

		else
			enqueue(p);
	}
}
