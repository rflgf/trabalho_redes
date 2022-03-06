#ifndef PACKET_HANDLER
#define PACKET_HANDLER

#include "packet.h"

typedef int router_id;
typedef int cost;

void *packet_handler_f(void *arg);

void evaluate_distance_vector(router_id source, struct distance_vector *dv_start);
#endif
