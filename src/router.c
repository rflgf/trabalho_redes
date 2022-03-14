#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "router.h"
#include "utils.h"

void initialize_router(int id, unsigned short int port, char *ip_address)
{
	if (pthread_cond_init(&me.sleep_cond_var, NULL) != 0)
		die("pthread_cond_init");

	me.udp_socket = calloc(1, sizeof(struct sockaddr_in));
	me.file_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (me.file_descriptor == -1)
		die("socket.");

    me.udp_socket->sin_family		= AF_INET;
    me.udp_socket->sin_port			= htons(port);

	// convert IP address to binary
	if (inet_pton(AF_INET, ip_address, &me.udp_socket->sin_addr.s_addr) != 1)
		die("inet_aton");

    // bind socket to port
	int err = bind(me.file_descriptor, (struct sockaddr*) me.udp_socket, sizeof(struct sockaddr_in));
    if (err)
    //if (!bind(me.file_descriptor, (struct sockaddr*) me.udp_socket, sizeof(me.udp_socket)))
        die("erro ao fazer o bind: %d", err);

	me.enabled = true;

	// initializing semaphore stuff.
	sem_init(&me.input.semaphore, 0, 0);	// at most MAX_QUEUE_ITEMS.
	sem_init(&me.output.semaphore, 0, 0);

	pthread_mutex_init(&me.mutex, NULL);
	pthread_mutex_init(&me.terminal_mutex, NULL);
	pthread_mutex_init(&me.input.mutex, NULL);
	pthread_mutex_init(&me.output.mutex, NULL);
	pthread_mutex_init(&me.packet_counter_mutex, NULL);

	me.packet_counter = 0;
}

void destruct_socket();
