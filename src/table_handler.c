#include "table_handler.h"

// returns -1 if `id` is not in our list of links.
length get_link_cost(router_id id)
{
	struct link *neighbour = me.neighbouring_routers;
	while (neighbour)
	{
		if (neighbour->id == id)
			return neighbour->cost_to_neighbour;
		neighbour = neighbour->next;
	}

	return -1;
}

struct table_item get_table_item_by_destination(router_id destination)
{
	for (struct table_item *item = me.table->items; item; item = item->next)
	{
		if (item->destination == destination)
			return item;
	}
	return NULL;
}

void evaluate_distance_vector(router_id source, struct distance_vector *dv_start)
{
	struct distance_vector *start = dv;

	struct table_item *distance_vector_table = me.table;

	for (struct distance_vector *dv = dv_start; dv != NULL; dv = dv->next)
	{
		router_id destination = dv->virtual_address;
		length	  cost		  = get_link_cost(destination); // checks this router's link struct.
		if (length == -1)
			die("link inexistente. tratar direito ao invés de implodir");

		struct table_item i = get_table_item_by_destination(destination);
		if (dv->distance + cost < i->cost)
			// add to table.
	}

	free_distance_vector(start);
}
