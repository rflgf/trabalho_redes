#include <string.h>

#include "receiver.h"
#include "packet.h"
#include "router.h"

// does not deserialize anything, just listens, gets queue semaphore and appends message (as char) to queue

void *receiver_f(void *data)
{
	while (true)
	{
		char *buffer = calloc(PAYLOAD_MAX_LENGTH, sizeof(char));
info("hewwo");
		int error_check = recvfrom(me.file_descriptor, buffer, PAYLOAD_MAX_LENGTH, 0, NULL, NULL);
		if (error_check == -1)
			die("erro %d em recv\n", error_check);
info("fufufufu");
		pthread_mutex_lock(&me.terminal_mutex);
		printf("%10s", buffer);
		pthread_mutex_unlock(&me.terminal_mutex);

		debug("receiver_f from receiver.c is acquiring me.mutex");
		pthread_mutex_lock(&me.mutex);
		if (me.enabled)
		{
			pthread_mutex_unlock(&me.mutex);
			enqueue(buffer);
		}
		pthread_mutex_unlock(&me.mutex);
	}
}
