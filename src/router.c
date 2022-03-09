#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "router.h"
#include "utils.h"

void initialize_router(int id, int port, char *ip_address)
{
	me.udp_socket = calloc(1, sizeof(struct sockaddr_in *));
	me.file_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (me.file_descriptor == -1)
		die("socket.");

    me.udp_socket->sin_family		= AF_INET;
    me.udp_socket->sin_port			= htons(port);

	// convert IP address to binary
	struct in_addr ip;
	if (inet_pton(AF_INET, ip_address, &ip.s_addr) != 1)
		die("inet_aton");

	me.udp_socket->sin_addr = ip;

    // bind socket to port
    if (!bind(me.file_descriptor, (struct sockaddr*) &me.udp_socket, sizeof(me.udp_socket)))
        die("erro ao fazer o bind.");

	me.enabled = true;

	// initializing semaphore stuff.
	sem_init(&me.input.semaphore, 0, 0);	// at most MAX_QUEUE_ITEMS.
	sem_init(&me.output.semaphore, 0, 0);

	pthread_mutex_init(&me.mutex, NULL);
	pthread_mutex_init(&me.terminal_mutex, NULL);
	pthread_mutex_init(&me.input.mutex, NULL);
	pthread_mutex_init(&me.output.mutex, NULL);
}

void destruct_socket();
