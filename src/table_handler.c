#include <stdio.h>
#include <unistd.h>

#include "table_handler.h"
#include "packet.h"

// every N seconds calculate my distance vector (make a function for this because packet_handler.c will need it too) and send it to every enabled link
// check for timestamps associated with the table's distance vectors and remove the ones that are outdated
void *table_handler_f(void *arg)
{
	while (true)
	{
		pthread_mutex_lock(&me.mutex);
		struct link *neighbour;
		for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
		{
			if (!neighbour->enabled)
				continue;

			// check if link to neighbour is still up.
			time_t timestamp = time(NULL);
			if (timestamp - neighbour->last_heard_from > CONNECTION_TIMEOUT)
			{
				printf("conexão com %d alcançou o tempo limite de espera,\ndesconectando removendo enlace...\n", neighbour->id);
				neighbour->enabled = false;

				remove_link(neighbour);
			}

			// update neighbour as to our own distance vector.
			union packet *new_p					 = malloc(sizeof(union packet));
			new_p->deserialized.type			 = CONTROL;
			new_p->deserialized.source			 = me.id;
			new_p->deserialized.destination		 = neighbour->id;
			new_p->deserialized.payload.distance = calculate_distance_vector();
			serialize(new_p, true);
			enqueue(new_p);
		}
		pthread_mutex_unlock(&me.mutex);
		sleep(SLEEP_TIME);
	}
}

// returns -1 if `id` is not in our list of links.
cost get_link_cost(router_id id)
{
	struct link *n;
	pthread_mutex_lock(&me.mutex);
	for (n = me.neighbouring_routers; n; n = n->next)
		if (n->id == id)
		{
			pthread_mutex_unlock(&me.mutex);
			return n->cost_to;
		}
	pthread_mutex_unlock(&me.mutex);
	return -1;
}

struct table_item *get_table_item_by_destination(router_id destination)
{
	struct table_item *item;
	pthread_mutex_lock(&me.mutex);
	for (item = me.table.items; item; item = item->next)
		if (item->destination == destination)
		{
			pthread_mutex_unlock(&me.mutex);
			return item;
		}
	pthread_mutex_unlock(&me.mutex);
	return NULL;
}

// evaluates the list of distance vectors `dv_start` and
// updates my table of distance vectors if needed.
// `source` indicates what router address the DVs were sent by,
// while `dv_start` is the list of DVs.
void evaluate_distance_vector(router_id source, struct distance_vector *dv_start)
{
	struct distance_vector *dv;
	for (dv = dv_start; dv; dv = dv->next)
	{
		router_id destination = dv->virtual_address;
		cost cost_to_source	  = get_link_cost(source);
		if (cost_to_source == -1)
			die("%d recebeu um DV de %d, mas o enlace de %d para %d é inexistente!",
				me.id,
				source,
				source,
				me.id);

		struct table_item *table_item = get_table_item_by_destination(destination);

		if (dv->distance + cost_to_source < table_item->cost)
		{
			pthread_mutex_lock(&me.table.mutex);
			table_item->neighbouring_router = source;
			table_item->cost				= dv->distance + cost_to_source;
			pthread_mutex_unlock(&me.table.mutex);
		}
	}

	free_distance_vector(dv_start);
}

// creates a distance vector based on the current table
// of distance vectors to be sent to the router's neighbours.
struct distance_vector *calculate_distance_vector()
{
	struct distance_vector *list_item;
	struct distance_vector *aux = NULL;

	pthread_mutex_lock(&me.table.mutex);
	struct table_item *table_item;
	for (table_item = me.table.items; table_item; table_item = table_item->next)
	{
		list_item				   = malloc(sizeof(struct distance_vector));
		list_item->next			   = NULL;
		list_item->virtual_address = table_item->destination;
		list_item->distance		   = table_item->cost;

		if (aux)
			aux->next = list_item;
		aux	= list_item;
	}

	pthread_mutex_unlock(&me.table.mutex);
	return list_item;
}

void remove_link(struct link *neighbour)
{
	pthread_mutex_lock(&me.mutex);
	neighbour->enabled = false;
	pthread_mutex_unlock(&me.mutex);

	pthread_mutex_lock(&me.table.mutex);
	struct table_item *ti = me.table.items;
	while (ti)
		if (ti->neighbouring_router == neighbour->id)
			if (ti->next)
			{
				struct table_item *aux = ti->next;
				free(ti);
				ti = aux;
			}
			else
				free(ti);
	pthread_mutex_unlock(&me.table.mutex);
}
