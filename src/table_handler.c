#include "table_handler.h"

void table_handler_f(void *arg)
{
}

// returns -1 if `id` is not in our list of links.
cost get_link_cost(router_id id)
{
	struct link *n;
	for (n = me.neighbouring_routers; n; n = n->next)
		if (n->id == id)
			return n->cost_to;
	return -1;
}

struct table_item *get_table_item_by_destination(router_id destination)
{
	struct table_item *item;
	for (item = me.table->items; item; item = item->next)
		if (item->destination == destination)
			return item;
	return NULL;
}

// evaluates the list of distance vectors `dv_start` and
// changes the process' table of distance vectors if needed.
void evaluate_distance_vector(router_id source, struct distance_vector *dv_start)
{
	struct table_item *distance_vector_table = me.table;

	struct distance_vector *dv;
	for (dv = dv_start; dv; dv = dv->next)
	{
		router_id destination = dv->virtual_address;
		cost cost_to_source	  = get_link_cost(source);
		if (cost_to_source == -1)
			die("link de %d para %d inexistente!");
			// @TODO something like append_to_table(dv, source);

		struct table_item *table_item = get_table_item_by_destination(destination);

		if (dv->distance + cost_to_source < table_item->cost)
		{
			// @TODO aquire mutex to change the table.
			table_item->neighbouring_router = source;
			table_item->cost				= dv->distance + cost_to_source;
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

	struct table_item *table_item;
	for (table_item = me.table->items; table_item; table_item = table_item->next)
	{
		list_item				   = malloc(sizeof(struct distance_vector));
		list_item->next			   = NULL;
		list_item->virtual_address = table_item->destination;
		list_item->distance		   = table_item->cost;

		if (aux)
			aux->next = list_item;
		aux	= list_item;
	}

	return list_item;
}

void remove_link()
{
	// remove link pointer from neighbouring_routers list
	// remove every distance vector from the table that has its origin from the link in question
	// @TODO acquire mutexes to do so
}

