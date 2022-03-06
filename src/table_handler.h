#ifndef TABLE_HANDLER
#define TABLE_HANDLER

#include "router.h"
#include "packet.h"
#include "utils.h"

struct distance_vector *calculate_distance_vector();

struct table_item *get_table_item_by_destination(router_id destination, struct table_item *table);

struct table_item *calculate_table();

void remove_link(struct link *neighbour);

void *table_handler_f(void *data);
#endif
