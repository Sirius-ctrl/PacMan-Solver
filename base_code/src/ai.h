#ifndef __AI__
#define __AI__

#include <stdint.h>
#include <unistd.h>
#include "node.h"
#include "priority_queue.h"
#define INIT_SIZE_OF_EXPLORED 100
#define DIRECTIONS 4

void initialize_ai();

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats , output_t* output);

#endif
