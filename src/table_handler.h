#ifndef TABLE_HANDLER
#define TABLE_HANDLER

#include "router.h"
#include "packet.h"
#include "utils.h"

void distributed_bellman_ford();

void evaluate_distance_vector(router_id source, struct distance_vector *dv);

struct distance_vector *calculate_distance_vector();

struct table_item *get_table_item_by_destination(router_id destination);

void remove_link(struct link *neighbour);

void *table_handler_f(void *data);
#endif
