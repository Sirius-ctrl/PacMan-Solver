#ifndef __NODE__
#define __NODE__

#include "utils.h"



/**
 * Data structure containing the node information
 */
struct node_s{
    int priority;
    float acc_reward;
    int depth;
    int num_childs;
    float children_rewards[4];    // to record the child reward for calculating the avg score for the base node
    move_t move;
    state_t state;
    struct node_s* parent;
};

typedef struct node_s node_t;


#endif
