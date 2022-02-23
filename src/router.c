#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "router.h"
#include "utils.h"


struct router* create_router(int id, int port, struct in_addr ip_address)
{
	// @TODO wtf are these
    struct sockaddr_in si_other;
    int i, slen=sizeof(si_other), recv_len;
    char buf[BUFLEN];
    char message[BUFLEN];

	struct router *router = calloc(1, sizeof(struct router));

	struct sockaddr_in *udp_socket = calloc(1, sizeof(struct sockaddr_in*));
	int unix_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (unix_socket == -1)
		die("porta fornecida não está disponível.");

    udp_socket->sin_family		= AF_INET;
    udp_socket->sin_port		= htons(port);
    udp_socket->sin_addr.s_addr = htonl(INADDR_ANY); // @TODO i dont think this shouold use htonl

    // bind socket to port
    if (bind(socket, (struct sockaddr*) &udp_socket, sizeof(udp_socket)) == -1)
        die("erro ao fazer o bind.");

	router->udp_socket = udp_socket;

	// @TODO create input and output packet queue for the router
	return router;

    // keep listening for data
    while(true)
    {
        printf("Waiting for data...");
        fflush(stdout);
        // receive a reply and print it
        // clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);

        // try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            die("recvfrom()");
	}
}

void destruct_socket();
