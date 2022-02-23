#ifndef UTILS
#define UTILS

#include <stdlib.h>
#include <stddef.h>

// errors
#define SUCCESS						   0
#define PARSE_ERR_UNKOWN_ARG		   1
#define PARSE_ERR_FILE_NOT_FOUND	   2
#define PARSE_ERR_NO_FILE_PROVIDED	   3
#define PARSE_ERR_NO_VIRT_ADD_PROVIDED 4
#define SOCKET_ERR_PORT				   5

#define BUFLEN 512 // max length of buffer

typedef int router_id;
typedef int cost;

void die(const char *format, ...);

int parse_args(int argc, char **argv);

int parse_router_config(char *filename, int virtual_address);

int parse_link_config(char *filename, int virtual_address);

void add_link(router_id neighbour_id, cost cost);
#endif
