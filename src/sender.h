#ifndef SENDER
#define SENDER

#include "packet.h"
#include "router.h"
#include "table_handler.h"
#include "utils.h"

void *sender_f(void *arg);

void free_table(struct table_item *table);
#endif
