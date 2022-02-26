#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <pthread.h>

#include "utils.h"
#include "packet.h"
#include "router.h"
#include "table_handler.h"
#include "packet_handler.h"
#include "sender.h"
#include "receiver.h"

enum menu_options {
	QUIT,
	SEND_MESSAGE,
	SLEEP_OR_WAKE,
	LIST_LINKS
};

int main(int argc, char **argv)
{
	if (parse_args(argc, argv))
		return -1;

	// initializing semaphore stuff.
	sem_init(&me.input.lock, 0, 1);
	sem_init(&me.output.lock, 0, 1);

	int error_check;

	// main thread will handle the terminal stuff.
	pthread_t receiver,
			  sender,
			  packet_handler,
			  table_handler;

	pthread_create(&receiver, NULL, receiver_f, NULL);
	pthread_create(&sender, NULL, sender_f, NULL);
	pthread_create(&packet_handler, NULL, packet_handler_f, NULL);
	pthread_create(&table_handler, NULL, table_handler_f, NULL);

	bool run;

	while (run)
	{
		printf("ID: %d\n", me.id);
		printf("Escolha alguma das opções:\n");
		printf("\t0 - Sair\n");
		printf("\t1 - Enviar mensagem\n");
		if (me.enabled)
			printf("\t2 - Desligar roteador\n");
		else
			printf("\t2 - Ligar roteador\n");
		printf("\t3 - Ver enlaces do roteador\n");

		enum menu_options input;
		scanf("%d", (int *) &input);

		switch (input)
		{
			case QUIT:
				run = !run;
				break;

			case SEND_MESSAGE:
					break;

			case SLEEP_OR_WAKE:
				break;

			case LIST_LINKS:
				break;

			default:
				printf("Opção inválida\n");
				continue;
		}
	}

	// joins
	error_check = pthread_join(receiver, NULL);
	if (error_check)
		die("Erro pthread_join no receiver");

	error_check = pthread_join(sender, NULL);
	if (error_check)
		die("Erro pthread_join no sender");

	error_check = pthread_join(packet_handler, NULL);
	if (error_check)
		die("Erro pthread_join no packet_handler");

	error_check = pthread_join(table_handler, NULL);
	if (error_check)
		die("Erro pthread_join no table_handler");

	return 0;
}
