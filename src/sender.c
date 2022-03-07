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
		pthread_mutex_lock(&me.mutex);
		if (!me.enabled)
		{
			pthread_mutex_unlock(&me.mutex);
			// @TODO barrier/cond this
		}
		pthread_mutex_unlock(&me.mutex);

		struct table_item *table = calculate_table();
		pthread_mutex_lock(&me.output.mutex);

		struct queue_item *qi = me.output.head;
		while (qi)
		{
			union packet *p = qi->packet;
			router_id link = get_table_item_by_destination(p->deserialized.destination, table)->next_hop;

			struct sockaddr_in socket = get_link_by_id(link)->socket;

			int error_check = sendto(me.file_descriptor, p->serialized, strlen(p->serialized), 0, (struct sockaddr *) &socket, sizeof(socket));

			if (error_check == -1)
				die("sendto");

			free(p->serialized);

			// supposedly we don't have `p->deserialized.payload.message|distance` at this point.
			// @TODO initialize these two as null so we can make a null check here and free them.

			struct queue_item *aux = qi;
			free(qi);
			qi = aux->next;
		}

		pthread_mutex_unlock(&me.output.mutex);
		free_table(table);
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
