#include <stdbool.h>

#include "sender.h"

void *sender_f(void *arg)
{
	while (true)
	{
		sem_wait(&me.output.lock);

		sem_post(&me.output.lock);
	}

}
