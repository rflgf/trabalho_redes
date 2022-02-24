#include <stdbool.h>

#include "sender.h"

// unlocks on output queue > 0
// get output (as char arr) and sends it to destination
void *sender_f(void *arg)
{
	while (true)
	{
		sem_wait(&me.output.lock);

		sem_post(&me.output.lock);
	}

}
