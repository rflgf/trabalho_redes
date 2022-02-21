#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <pthread.h>

#include "utils.h"
#include "packet.h"
#include "router.h"

#define SERVER "127.0.0.1"
#define PORT   8888		// the port on which to send data

struct router_meta_info {
	int id,
		port;
	struct in_addr ip_address; // IPv4 address in binary form
	struct router_meta_info *next;
};

struct router_meta_info *router;

void *listener_f (void *);
void *sender_f   (void *);
void *handler_f  (void *);

enum menu_options {
	quit,
	sleep_or_wake,
	list_links,
	remove_link
};

int main(int argc, char **argv)
{
	int error_check;

	// main thread will handle the terminal stuff.
	pthread_t receiver,
			  sender,
			  packet_handler;

	// receiver
	error_check = pthread_create(&receiver, NULL, listener_f, NULL);
	if (error_check)
		die("Erro pthread_create receiver");

	// sender
	error_check = pthread_create(&sender, NULL, sender_f, NULL);
	if (error_check)
		die("Erro pthread_create sender");

	// packet_handler
	error_check = pthread_create(&packet_handler, NULL, handler_f, NULL);
	if (error_check)
		die("Erro pthread_create packet_handler");

	bool run;

	while (run)
	{
		printf("ID: %d\n", router->id);
		printf("Escolha alguma das opções:\n");
		printf("\t1 - Sair\n");
		printf("\t2 - Desligar roteador\n");
		printf("\t3 - Ver enlaces do roteador\n");
		printf("\t4 - Desligar um enlace\n");

		enum menu_options input;
		scanf("%d", &input);

		switch (input)
		{
			case quit:
				run = !run;
				break;

			case sleep_or_wake:
				break;

			case list_links:
				break;

			case remove_link:
				break;

			default:
				printf("Opção inválida\n");
				continue;
		}
	}

	// joins
	error_check = pthread_join(&receiver, NULL);
	if (error_check)
		die("Erro pthread_join no receiver\n");

	error_check = pthread_join(&sender, NULL);
	if (error_check)
		die("Erro pthread_join no sender\n");

	error_check = pthread_join(&packet_handler, NULL);
	if (error_check)
		die("Erro pthread_join no packet_handler\n");

	if (parse_args(argc, argv))
		return -1;

	return 0;
}
