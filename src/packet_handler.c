#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "packet_handler.h"
#include "router.h"
#include "packet.h"

void *packet_handler_f(void *arg)
{
	while (true)
	{
		if (!me.enabled)
		{
			pthread_mutex_lock(&me.mutex);
			pthread_cond_wait(&me.sleep_cond_var, &me.mutex);
		}
		struct packet* p = dequeue(&me.input);
		deserialize_header(p);

		if (p->deserialized.destination == me.id)
		{
			char *packet_id = evaluate_packet_id(p);
			info("recebido pacote #%s de %d.", packet_id, p->deserialized.source);
			free(packet_id);
			switch (p->deserialized.type)
			{
				case CONTROL:
					deserialize_payload(p);
					// @TODO check whether this is leaking memory.
					// we may need to free p.
					evaluate_distance_vector(p->deserialized.source, p->deserialized.payload.distance);
					break;

				case DATA:
					deserialize_payload(p);
					char *message = p->deserialized.payload.message;

					printf("---------- mensagem recebida ----------\n");
					printf("de: %d\npara:%d\n", p->deserialized.source, p->deserialized.destination);
					printf("\t%s\n", message);
					printf("---------------------------------------\n");
					break;
			}
			free(p->serialized);
			free(p);
		}

		else
		{
			char *packet_id = evaluate_packet_id(p);
			info("repassando pacote #%s de %d.", packet_id, p->deserialized.source);
			free(packet_id);
			enqueue(p);
		}
	}
}

// replaces the link-associated router's last DV by `dv_start`.
// `source` indicates what router address the DVs were sent by.
void evaluate_distance_vector(router_id source, struct distance_vector *dv_start)
{
	pthread_mutex_lock(&me.mutex);

	struct link *source_link = get_link_by_id(source);

	source_link->enabled = true;
	source_link->last_heard_from = time(NULL);
	free_distance_vector(source_link->last_dv);
	source_link->last_dv = dv_start;

	pthread_mutex_unlock(&me.mutex);
}

