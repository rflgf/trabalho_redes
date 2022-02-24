#include "packet_handler.h"

void *packet_handler_f(void *arg)
{
	// unlocks on input queue > 0
	// deserializes data and
	// - if im the destination
	// - - if control, do the control thing
	// - - if message, print out to the terminal
	// - else put it in the output queue
}
