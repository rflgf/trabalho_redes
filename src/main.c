#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

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
	bool run = true;

	while (run)
	{
		printf("---------------------------------------\n");
		printf("ID: %d\n", me.id);
		printf("escolha alguma das opções:\n");
		printf("\t0 - sair\n");
		printf("\t1 - enviar mensagem\n");
		if (me.enabled)
			printf("\t2 - desligar roteador\n");
		else
			printf("\t2 - ligar roteador\n");
		printf("\t3 - ver enlaces do roteador\n");

		enum menu_options input;
		scanf("%d", (int *) &input);

		switch (input)
		{
			case QUIT:
				run = !run;
				break;

			case SEND_MESSAGE:

				printf("------------- nova mensagem -----------\n");
				router_id destination;
				char message[PAYLOAD_MAX_LENGTH]; // @TODO tratar tamanho correto da mensagem.
				printf("para: ");
				scanf("%d", &destination);

				printf("mensagem: ");
				scanf("%100s", message);
				fflush(stdin);
				printf("\n---------------------------------------\n");

				if (destination == me.id)
				{
					printf("---------- mensagem recebida ----------\n");
					printf("de: %d\npara:%d\n", me.id, me.id);
					printf("%s", message);
					printf("---------------------------------------\n");
					continue;
				}

				struct packet *p = malloc(sizeof(struct packet));
				p->deserialized.type = DATA;
				p->deserialized.source = me.id;
				p->deserialized.destination = destination;
				p->deserialized.index = 0;
				p->deserialized.payload.message = message;
				serialize(p, false);

				struct queue_item *qi = malloc(sizeof(struct queue_item));
				qi->next = NULL;
				qi->packet = p;

				enqueue(p);

				break;

			case SLEEP_OR_WAKE:
				pthread_mutex_lock(&me.mutex);
				me.enabled = !me.enabled;
				pthread_mutex_unlock(&me.mutex);
				break;

			case LIST_LINKS:
				printf("---------- lista de enlaces -----------\n");
				printf("\tid\tcusto/dist\thabilitado\n");
				//pthread_mutex_lock(&me.mutex);
				printf("a\n");
				struct link *l;
				for (l = me.neighbouring_routers; l; l = l->next)
					printf("\t%d\t%d\t\t%s\n",
						l->id,
						l->cost_to,
						l->enabled? "\033[0;32mV\033[0m": "\033[0;31m-\033[0m");
				//pthread_mutex_unlock(&me.mutex);
				printf("---------------------------------------\n");
				break;

			default:
				printf("opção inválida\n");
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
