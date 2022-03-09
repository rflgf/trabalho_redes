#ifndef UTILS
#define UTILS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef DEBUG
#include <assert.h>
void debug(const char *format, ...);
#else
#define debug(...) (void)0 /* no-op */
#define assert(...) (void)0 /* no-op */
#endif

// errors
#define SUCCESS						   0
#define PARSE_ERR_UNKOWN_ARG		   1
#define PARSE_ERR_FILE_NOT_FOUND	   2
#define PARSE_ERR_NO_FILE_PROVIDED	   3
#define PARSE_ERR_NO_VIRT_ADD_PROVIDED 4
#define SOCKET_ERR_PORT				   5

int MAX_QUEUE_ITEMS;
time_t SLEEP_TIME;
time_t CONNECTION_TIMEOUT;

typedef int router_id;
typedef int cost;

void die(const char *format, ...);

int parse_args(int argc, char **argv);

int parse_router_config(char *filename, int virtual_address);

int parse_link_config(char *filename, int virtual_address);

void add_link(router_id neighbour_id, cost cost);

struct link *get_link_by_id(router_id id);
#endif
