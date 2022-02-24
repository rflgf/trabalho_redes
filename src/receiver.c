#include <string.h>

#include "receiver.h"
#include "packet.h"
#include "router.h"

// does not deserialize anything, just listen, get queue semaphore and append message (as char) to queue

void *receiver_f(void *data)
{
	while (true)
	{
		char buffer[PAYLOAD_MAX_LENGTH];
		memset(buffer, '\0', PAYLOAD_MAX_LENGTH);

		int error_check = recv(me.udp_socket, buffer, PAYLOAD_MAX_LENGTH, 0);
		if (error_check == -1)
			die("erro %d em recv\n", error_check);

		struct packet *new_packet = deserialize(buffer);

		push(&me.input, new_packet);
	}
}
