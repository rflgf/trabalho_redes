#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "utils.h"
#include "router.h"
#include "table_handler.h"

typedef int cost;

char router_filename[100] = "roteador.config";
char link_filename[100]   = "enlaces.config";
time_t CONNECTION_TIMEOUT = 6;					// in seconds
time_t SLEEP_TIME		  = 2;					// in seconds
int MAX_QUEUE_ITEMS		  = 6;

void die(const char *format, ...)
{
	va_list args;
	char color[200] = "\033[0;31m[ error ] ";
	char *color_end = "\n\033[0m\0";
    strcat(color, format);
	strcat(color, color_end);
	va_start(args, format);
    vfprintf(stderr, color, args);
    va_end(args);
	exit(1);
}

int parse_args(int argc, char **argv)
{
	me.neighbouring_routers = NULL;
	me.input.head  = NULL;
	me.output.head = NULL;

	bool virt_address_provided = false;
	int virt_add;

	for (int i = 1; i < argc; i += 2)
	{
		if(argv[i][0] != '-' ||
		   argv[i][2] != '\0')
		{
		arg_inv:
			printf("Argumento inválido: '%s'\n-h para ajuda.\n", argv[i]);
			return PARSE_ERR_UNKOWN_ARG;
		}

		switch (argv[i][1])
		{
			case 'h':
				printf("Comando\t\tDescrição\n\n");
				printf("-h\t\tMostrar esta mensagem\n\n");
				printf("-a [virtual address]\tDefine [virtual address] como endereço virtual do socket\n");
				printf("-l [file]\tDefine [file] como arquivo de configuração dos enlaces.\n\t\tSe omitido, 'enlaces.config' será utilizado\n");
				printf("-m [max]\tDefine [max] como numero máximo de itens por fila.\n");
				printf("-r [file]\tDefine [file] como arquivo de configuração dos roteadores.\n\t\tSe omitido, 'roteador.config' será utilizado\n\n");
				printf("-s [time]\tDefine [time] como tempo de congelamento da thread de envio de vetores distância e\n\tcontador do tempo entre mensagens de controles dos enlaces.\n");
				printf("-t [time]\tDefine [time] (em segundos inteiros) como tempo máximo de espera por uma mensagem de controle\n\tdos enlaces antes de removê-los indefinidamente.\n\tSe omitido, 6 será utilizado como padrão.\n");
				return -1;

			case 'a':
				if (i + 1 >= argc)
					die("Endereço virtual do roteador não fornecido.\n");
				virt_address_provided = true;
				virt_add = atoi(argv[i + 1]);
				printf("add is: %d\n", virt_add);
				break;

			case 'l':
				if (i + 1 >= argc)
					goto no_file_provided;
				strcpy(link_filename, argv[i + 1]);
				break;

			case 'm':
				if (i + 1 >= argc)
					die("Argumento para -m não fornecido.\n");
				MAX_QUEUE_ITEMS = atoi(argv[i + 1]);
				break;

			case 'r':
				if (i + 1 >= argc)
				no_file_provided:
					die("Nome do arquivo não fornecido.\n");

				strcpy(router_filename, argv[i + 1]);
				break;

			case 's':
				if (i + 1 >= argc)
					die("Argumento para -s não fornecido.\n");
				SLEEP_TIME = (time_t) atoi(argv[i + 1]);
				break;

			case 't':
				if (i + 1 >= argc)
					die("Argumento para -t não fornecido.\n");
				CONNECTION_TIMEOUT = (time_t) atoi(argv[i + 1]);
				break;

			default:
				goto arg_inv;
		};
	}

	if (!virt_address_provided)
	{
		printf("Por favor, forneça um endereço virtual para o roteador. Para mais informações, utilize -h\n");
		return PARSE_ERR_NO_VIRT_ADD_PROVIDED;
	}
	else
		me.id = virt_add;

	/* we calculate the links before the routers so we know from
	   which routers we need to keep infomation such as as IP
	   address and port. this also means that such information
	   is static and dictated exclusively by the charge files,
	   not being possible to "create" new links at runtime,
	   only enabling and disabling statically assigned ones. */
	if (parse_link_config(link_filename, virt_add))
		die("parse_link_config");

	// calculate all possible routers
	if (parse_router_config((char *) router_filename, virt_add))
		die("parse_router_config");

	return 0;
}

struct link *get_link_by_id(router_id id)
{
	struct link *neighbour;
	for (neighbour = me.neighbouring_routers; neighbour; neighbour = neighbour->next)
		if (neighbour->id == id)
			return neighbour;

	return NULL;
}

int parse_router_config(char *filename, router_id virtual_address)
{
	FILE *desc = fopen(filename, "r");

	if (!desc)
		return PARSE_ERR_FILE_NOT_FOUND;

	fseek(desc, 0, SEEK_SET);

	char line[100];

	while (fgets(line, sizeof(line), desc))
	{
		int id, port;
		char ip_address[INET_ADDRSTRLEN]; // IPv4 only
		sscanf(line, "%d %d %s", &id, &port, &ip_address[0]);

		if (id != virtual_address)
		{
			struct link *potential_neighbour;
			if (potential_neighbour = get_link_by_id(id))
			{
				potential_neighbour->enabled = true;

				struct sockaddr_in *socket = &potential_neighbour->socket;

				socket->sin_family = AF_INET;
				socket->sin_port = htons(port);
				memset(&socket->sin_zero, 0, 8);
				int error_check = inet_pton(AF_INET, &ip_address[0], &socket->sin_addr.s_addr);
				if (error_check != 1)
					die("falha %d em inet_pton com ip %s", error_check, &ip_address[0]);
			}

			else
				continue;
		}

		else
			initialize_router(virtual_address, port, &ip_address[0]);

	if (feof(desc))
		break;
	}

	fclose(desc);

	return SUCCESS;
}

int parse_link_config(char *filename, router_id my_virtual_address)
{
	FILE *desc = fopen(filename, "r");

	if (!desc)
		return PARSE_ERR_FILE_NOT_FOUND;

	fseek(desc, 0, SEEK_SET);

	char line[100];

	while (fgets(line, sizeof(line), desc))
	{
		int source, destination, cost;
		sscanf(line, "%d %d %d", &source, &destination, &cost);

		if (source == my_virtual_address)
			add_link(destination, cost);

		else if (destination == my_virtual_address)
			add_link(source, cost);

		if (feof(desc))
			break;
	}

	fclose(desc);

	return SUCCESS;
}

void add_link(router_id neighbour_id, cost cost)
{
	struct link *new_neighbour = malloc(sizeof(struct link));

	new_neighbour->enabled = false;
	new_neighbour->cost_to = cost;
	new_neighbour->next	   = NULL;

	// we keep it false until we know for sure it's being used.
	new_neighbour->id = neighbour_id;

	if (!me.neighbouring_routers)
		me.neighbouring_routers = new_neighbour;
	else
	{
		struct link *neighbours = me.neighbouring_routers;
		while (neighbours->next)
			neighbours = neighbours->next;
		neighbours->next = new_neighbour;
	}
}
