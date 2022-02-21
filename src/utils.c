#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "utils.h"
#include "router.h"

char router_filename[100] = "roteador.config";
char link_filename[100]   = "enlaces.config";

struct router_meta_info {
	struct in_addr ip_address; // IPv4 address in binary form
	struct router_meta_info *next;
};

struct router_meta_info *router;

void die(const char *message)
{
	perror("\033[0;31m");
	perror(message);
	perror("\033[0m");
	exit(1);
}

int parse_args(int argc, char **argv)
{
	bool virt_address_provided = false;
	int virt_add;

	for(int i = 1; i < argc; i += 2)
	{
		if(argv[i][0] != '-' ||
		   argv[i][1] != '-' ||
		   argv[i][3] != '\0')
		{
		arg_inv:
			printf("Argumento inválido: '%s'\n--h para ajuda.\n", argv[i]);
			return PARSE_ERR_UNKOWN_ARG;
		}

		switch(argv[i][2])
		{
			case 'h':
				printf("Comando\t\tDescrição\n\n");
				printf("--h\t\tMostrar esta mensagem\n\n");
				printf("--a [virtual address]\tDefine [virtual address] como endereço virtual do socket\n");
				printf("--r [file]\tDefine [file] como arquivo de configuração dos roteadores.\n\t\tSe omitido, 'roteador.config' será utilizado\n\n");
				printf("--l [file]\tDefine [file] como arquivo de configuração dos enlaces.\n\t\tSe omitido, 'enlaces.config' será utilizado\n");
				return -1;

			case 'r':
				if(i + 1 >= argc)
				{
				no_file_provided:
					printf("Nome do arquivo não fornecido.\n");
					return PARSE_ERR_NO_FILE_PROVIDED;
				}
				strcpy(router_filename, argv[i + 1]);
				break;

			case 'l':
				if(i + 1 >= argc)
					goto no_file_provided;
				strcpy(link_filename, argv[i + 1]);
				break;

			case 'a':
				if(i + 1 >= argc)
				{
					printf("Endereço virtual do roteador não fornecido.\n");
					return PARSE_ERR_NO_VIRT_ADD_PROVIDED;
				}
				virt_address_provided = true;
				virt_add = atoi(argv[i + 1]);
				break;

			default:
				goto arg_inv;
		};
	}

	if(!virt_address_provided)
	{
		printf("Por favor, forneça um endereço virtual para o roteador. Para mais informações, utilize --h\n");
		return PARSE_ERR_NO_VIRT_ADD_PROVIDED;
	}

	// calculate all possible routers
	if (!parse_router_config((char *) router_filename, virt_add))
		die("parse_router_config");

	return 0;
}

int parse_router_config(char *filename, int virtual_address)
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
		if(id != virtual_address)
			continue;

		// convert IP address to binary
		if(!inet_aton((char *) &ip_address, &router->ip_address))
			die("Erro em inet_aton()");

	if (feof(desc))
		break;
	}

	fclose(desc);

	return SUCCESS;
}
