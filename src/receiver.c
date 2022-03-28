#include <string.h>

#include "receiver.h"

// does not deserialize anything, just listens, gets queue semaphore and appends message (as char) to queue

void *receiver_f(void *data)
{
	while (true)
	{
		if (!me.enabled)
		{
			pthread_mutex_lock(&me.mutex);
			pthread_cond_wait(&me.sleep_cond_var, &me.mutex);
		}
		char *buffer = calloc(PAYLOAD_MAX_LENGTH, sizeof(char));

		int error_check = recvfrom(me.file_descriptor, buffer, PAYLOAD_MAX_LENGTH, 0, NULL, NULL);
		if (error_check == -1)
			die("erro %d em recv\n", error_check);

		pthread_mutex_lock(&me.mutex);
		if (me.enabled)
		{
			pthread_mutex_unlock(&me.mutex);
			enqueue(buffer);
		}
		pthread_mutex_unlock(&me.mutex);
	}
}
