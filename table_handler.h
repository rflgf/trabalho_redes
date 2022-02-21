
#ifndef TABLE_HANDLER
#define TABLE_HANDLER

struct table_item {
	router_id		   destination;
	router_id		   neighbouring_router; /* the neighbouring router used to get to `destination`. */
	length			   cost;
	struct table_item *next;
};

struct 	table {
	struct table_item *items;
};

void distributed_bellman_ford();

void evaluate_distance_vector(router_id source, struct distance_vector *dv);

void table_handler_f(void *data);

#endif
