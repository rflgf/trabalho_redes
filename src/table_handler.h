#ifndef TABLE_HANDLER
#define TABLE_HANDLER

#include "utils.h"

typedef int cost;
typedef int router_id;

struct distance_vector *calculate_distance_vector();

struct table_item *get_table_item_by_destination(router_id destination, struct table_item *table);

struct table_item *calculate_table();

void remove_link(struct link *neighbour);

void *table_handler_f(void *data);
#endif
