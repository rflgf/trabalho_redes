#include <stdio.h>
#include <unistd.h>

#include "table_handler.h"
#include "packet.h"
#include "router.h"
#include "utils.h"

// every N seconds calculate my distance vector (make a function for this because packet_handler.c will need it too) and send it to every enabled link
// check for timestamps associated with the table's distance vectors and remove the ones that are outdated
void *table_handler_f(void *arg)
{

	while (true)
	{
		struct table_item *table = calculate_table();
		struct distance_vector *dv = calculate_distance_vector(table);
		time_t current_time = time(NULL);

		pthread_mutex_lock(&me.mutex);
		struct link *neighbour;
		for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
			if (neighbour->enabled)
			{
				if (current_time - neighbour->last_heard_from > CONNECTION_TIMEOUT)
				{
					printf("conexão com %d alcançou o tempo limite de espera,\ndesconectando removendo enlace...\n", neighbour->id);
					free_distance_vector(neighbour->last_dv);
					neighbour->enabled = false;
				}

				struct packet *p = malloc(sizeof(struct packet));
				p->deserialized.type = CONTROL;
				p->deserialized.source = me.id;
				p->deserialized.payload.distance = dv;
				p->deserialized.destination = neighbour->id;
				p->deserialized.index = 0;
				serialize(p, false);
				enqueue(p);
			}
		pthread_mutex_unlock(&me.mutex);
		// @TODO free table
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

struct table_item *get_table_item_by_destination(router_id destination, struct table_item *table)
{
	struct table_item *item;
	for (item = table; item; item = item->next)
		if (item->destination == destination)
			return item;
	return NULL;
}

struct distance_vector *populate_dv_with_links()
{
	struct distance_vector *ret = NULL;
	struct distance_vector *aux;
	pthread_mutex_lock(&me.mutex);

	struct link *neighbour;
	for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
		if (neighbour->enabled)
		{
			struct distance_vector *new_item = malloc(sizeof(struct distance_vector));
			new_item->virtual_address = neighbour->id;
			new_item->distance = neighbour->cost_to;
			new_item->next = NULL;

			if (!ret)
			{
				ret = new_item; // create head.
				aux = new_item;
			}
			else
			{
				aux->next = new_item;
				aux		  = new_item;
			}
		}
	pthread_mutex_unlock(&me.mutex);
	return ret;
}

struct distance_vector *get_dv_by_destination(router_id destination, struct distance_vector *table)
{
	struct distance_vector *item;
	for (item = table; item; item = item->next)
		if (item->virtual_address == destination)
			break;
	return item;
}

void enqueue_to_distance_vector(struct distance_vector *dv, struct distance_vector *list)
{
	if (!list)
	{
		list = dv;
		list->next = NULL;
		return;
	}

	struct distance_vector *list_item;
	for (list_item = list; list_item->next; list_item = list_item->next)
		;
	list_item->next = dv;
	list_item->next = NULL;
}

// creates a distance vector based on the active links' distance vectors.
struct distance_vector *calculate_distance_vector()
{
	struct distance_vector *ret_dv = populate_dv_with_links();

	pthread_mutex_lock(&me.mutex);

	struct link *neighbour;
	for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
	{
		if (!neighbour->enabled)
			continue;

		struct distance_vector *neighbours_dv;
		for (neighbours_dv = neighbour->last_dv; neighbours_dv; neighbours_dv = neighbours_dv->next)
		{
			struct distance_vector *dv = get_dv_by_destination(neighbours_dv->virtual_address, ret_dv);
			// destination not in table yet, create it.
			if (!dv)
			{
				dv = malloc(sizeof(struct distance_vector));
				dv->virtual_address = neighbour->id;
				dv->distance = neighbours_dv->distance + neighbour->cost_to;
				dv->next = NULL;

				// this is an unnecessary step cuz a NULL `ret_table` means no enabled links.
				if (!ret_dv)
				{
					ret_dv = dv;
					continue;
				}

				enqueue_to_distance_vector(dv, ret_dv);
			}
			else
				// destination in the table but found a cheaper hop.
				if (dv->distance > neighbours_dv->distance + neighbour->cost_to)
					dv->distance = neighbours_dv->distance + neighbour->cost_to;
		}
	}
	pthread_mutex_unlock(&me.mutex);
	return ret_dv;
}

void remove_link(struct link *neighbour)
{
	pthread_mutex_lock(&me.mutex);
	neighbour->enabled = false;
	free_distance_vector(neighbour->last_dv);
	pthread_mutex_unlock(&me.mutex);
}

struct table_item *populate_table_with_links()
{
	struct table_item *ret = NULL;
	struct table_item *aux;
	pthread_mutex_lock(&me.mutex);

	struct link *neighbour;
	for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
		if (neighbour->enabled)
		{
			struct table_item *new_item = malloc(sizeof(struct table_item));
			new_item->destination = neighbour->id;
			new_item->next_hop = neighbour->id;
			new_item->cost = neighbour->cost_to;
			new_item->next = NULL;

			if (!ret)
			{
				ret = new_item;
				aux = new_item;
			}
			else
			{
				aux->next = new_item;
				aux		  = new_item;
			}
		}
	pthread_mutex_unlock(&me.mutex);
	return ret;
}

void enqueue_to_table(struct table_item *new_item, struct table_item *table)
{
	if (!table)
		table = new_item;

	else
	{
		struct table_item *item;
		for (item = table; item; item = item->next)
			;
		item->next = new_item;
	}
}


// applies the distributed Bellman-Ford algorithm to create a table.
struct table_item *calculate_table()
{
	struct table_item *ret_table = populate_table_with_links();

	pthread_mutex_lock(&me.mutex);

	struct link *neighbour;
	for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
	{
		if (!neighbour->enabled)
			continue;

		struct distance_vector *neighbours_dv;
		for (neighbours_dv = neighbour->last_dv; neighbours_dv; neighbours_dv = neighbours_dv->next)
		{
			struct table_item *ti = get_table_item_by_destination(neighbours_dv->virtual_address, ret_table);
			// destination not in table yet, create it.
			if (!ti)
			{
				ti = malloc(sizeof(struct table_item));
				ti->next_hop = neighbour->id;
				ti->cost = neighbours_dv->distance + neighbour->cost_to;
				ti->next = NULL;

				// this is an unnecessary step cuz a NULL `ret_table` means no enabled links.
				if (!ret_table)
				{
					ret_table = ti;
					continue;
				}

				enqueue_to_table(ti, ret_table);
			}
			else
			{
				// destination in the table but found a cheaper hop.
				if (ti->cost > neighbours_dv->distance + neighbour->cost_to)
				{
					ti->cost = neighbours_dv->distance + neighbour->cost_to;
					ti->next_hop = neighbour->id;
				}
			}
		}
	}

	pthread_mutex_unlock(&me.mutex);
	return ret_table;
}
