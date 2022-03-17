#include <stdbool.h>
#include <string.h>

#include "packet.h"
#include "sender.h"
#include "router.h"
#include "table_handler.h"
#include "utils.h"

void *sender_f(void *arg)
{
	while (true)
	{
		if (!me.enabled)
		{
			pthread_mutex_lock(&me.mutex);
			pthread_cond_wait(&me.sleep_cond_var, &me.mutex);
		}

		struct packet *p = dequeue(&me.output);
		info("next hop is %d for desitnation %d", p->deserialized.next_hop, p->deserialized.destination);
		struct link *l = get_link_by_id(p->deserialized.next_hop);
		if (!l)
		{
			char *packet_id = evaluate_packet_id(p);
			info("enlace %d desabilitado, descartando pacote #%s de %d", p->deserialized.next_hop, packet_id, p->deserialized.source);
			free(packet_id);
			continue;
		}

		struct sockaddr_in socket = l->socket;

		int error_check = sendto(me.file_descriptor, p->serialized, strlen(p->serialized), 0, (struct sockaddr *) &socket, sizeof(socket));

		if (error_check == -1)
			die("sendto");

		free(p->serialized);

		// supposedly we don't have `p->deserialized.payload.message|distance` at this point.
		// @TODO initialize these two as null so we can make a null check here and free them.

		pthread_mutex_unlock(&me.output.mutex);
	}

}

void free_table(struct table_item *table)
{
	if (!table)
		return;
	while (table)
	{
		struct table_item *aux = table;
		table = table->next;
		free(aux);
	}
}
