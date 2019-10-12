#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>


#include "ai.h"
#include "utils.h"
#include "priority_queue.h"


struct heap h;

float get_reward( node_t* n );

/**
 * Function called by pacman.c
*/
void initialize_ai(){
	heap_init(&h);
}

/**
 * function to copy a src into a dst state
*/
void copy_state(state_t* dst, state_t* src){
	//Location of Ghosts and Pacman
	memcpy( dst->Loc, src->Loc, 5*2*sizeof(int) );

    //Direction of Ghosts and Pacman
	memcpy( dst->Dir, src->Dir, 5*2*sizeof(int) );

    //Default location in case Pacman/Ghosts die
	memcpy( dst->StartingPoints, src->StartingPoints, 5*2*sizeof(int) );

    //Check for invincibility
    dst->Invincible = src->Invincible;
    
    //Number of pellets left in level
    dst->Food = src->Food;
    
    //Main level array
	memcpy( dst->Level, src->Level, 29*28*sizeof(int) );

    //What level number are we on?
    dst->LevelNumber = src->LevelNumber;
    
    //Keep track of how many points to give for eating ghosts
    dst->GhostsInARow = src->GhostsInARow;

    //How long left for invincibility
    dst->tleft = src->tleft;

    //Initial points
    dst->Points = src->Points;

    //Remiaining Lives
    dst->Lives = src->Lives;   

}

node_t* create_init_node( state_t* init_state ){
	node_t * new_n = (node_t *) malloc(sizeof(node_t));
	new_n->parent = NULL;	
	new_n->priority = 0;
	new_n->depth = 0;
	new_n->num_childs = 0;
	copy_state(&(new_n->state), init_state);
	new_n->acc_reward =  get_reward( new_n );
	// -1 represent no movement
	new_n->move = -1;
	
	return new_n;
	
}


float heuristic( node_t* n ){
	float i=0.0,l=0.0,g=0.0;

	// if game over
	if((n->state).Lives == 0){
		g = 100.0;
	}

	if(n->parent == NULL){
		perror("no parents has been found");
		exit(EXIT_FAILURE);
	}

	// retrive the parent node as it must exist
	node_t* parent = n->parent;

	//if we lost a life
	if((n->state).Lives < (parent->state).Lives){
		l = 10.0;
	}

	// if the last state is not invincible but becomes invincible now
	if((n->state).Invincible && (!(parent->state).Invincible)){
		i = 10;
	}

	return i-l-g;
}



float get_reward ( node_t* n ){
	// new node has no reward assigned
	if(n->parent == NULL) {
		return 0;
	}

	float reward = 0;
	float h = heuristic(n);
	float score_changes = (n->state).Points - (n->parent->state).Points;
	float discount = pow(0.99, n->depth);

	reward = h + score_changes;
	
	return discount * reward;
}


/**
 * Apply an action to node n and return a new node resulting from executing the action
*/
bool applyAction(node_t* curr_node, node_t* child_node, move_t action ){

	bool changed_dir = false;

	// testing if the action is valid and update the state
    changed_dir = execute_move_t( &((child_node)->state), action );

	// only doing the following things if "action" is valid
	if(changed_dir) {
		// update all node properties
		child_node->parent = curr_node;
		child_node->depth = curr_node->depth + 1;
		child_node->priority = curr_node->priority - 1;
		child_node->move = action;
		child_node->acc_reward = get_reward(child_node);
		node_t* trace_node = curr_node;
		// update the child number and acc_reward all the way down to the init node
		while (true) {
			fprintf(stderr, "now we are at d=%d\n", trace_node->depth);
			(trace_node->num_childs)++;
			child_node->acc_reward += trace_node->acc_reward;
			fprintf(stderr, "now d=%d has %d child\n", trace_node->depth, trace_node->num_childs);
			if(trace_node->parent != NULL){
				trace_node = trace_node->parent;
			} else {
				break;
			}
		}
		
	}

	return changed_dir;

}


/**
 * Find best action by building all possible paths up to budget
 * and back propagate using either max or avg
 */
move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats ){
	fprintf(stderr, "------------------------\n");
	float best_action_score[DIRECTIONS];

	for (unsigned i = 0; i < DIRECTIONS; i++) {
		best_action_score[i] = -1;
	}
	
	// printing stats
	unsigned generated_nodes = 0;
	unsigned expanded_nodes = 0;
	unsigned max_depth = 0;

	unsigned curr_size = INIT_SIZE_OF_EXPLORED;
	unsigned curr_used = 0;
	node_t **explored = (node_t **)malloc(INIT_SIZE_OF_EXPLORED*sizeof(node_t*));

	//initialize the node and add to frontier
	node_t *init_node = create_init_node(&init_state);
	heap_push(&h, init_node);

	while(h.count != 0){
		node_t* curr_node = heap_delete(&h);

		if(curr_node->depth > max_depth) { max_depth=curr_node->depth; }

		// resize the array if it already reach its maximum capacity
		if(curr_used == curr_size){
			curr_size *= 2;
			explored = (node_t **)realloc(explored, sizeof(node_t*) * curr_size);
		}
		explored[curr_used++] = curr_node;

		// if we haven't reach the budget yet
		if(expanded_nodes < budget) {
			++expanded_nodes;
			// the child nodes for the current parent node
			// in left, right, up, down sequence of direction, which is the same
			// as the enum defined in utils (left=0, right=1, up=2, down=3)
			node_t *direction_node[DIRECTIONS];
			// create four copies of the current state for each direction
			for (unsigned i = 0; i < DIRECTIONS; i++) {
				direction_node[i] = create_init_node(&(curr_node->state));
				if( applyAction(curr_node, direction_node[i], i) ){
					++generated_nodes;

					// propagate score back
					if (propagation == max) {
						node_t *trace_node = direction_node[i];

						while(trace_node->depth != 1) {
							trace_node = trace_node->parent;
						}

						if (best_action_score[trace_node->move] < direction_node[i]->state.Points) {
							best_action_score[trace_node->move] = direction_node[i]->state.Points;
						}
					} else if (propagation == avg) {
						if(curr_node->depth == 0){
							best_action_score[direction_node[i]->move] = direction_node[i]->state.Points;
						} else {
							node_t *trace_node = curr_node;

							while (true) {
								if (trace_node->depth == 1) {
									move_t side = trace_node->move;
									int child_nums = trace_node->num_childs;
									
									// the child shouldn't be zero (for debuging purpose)
									if (child_nums <= 0) {
										exit(EXIT_FAILURE);
									}

									best_action_score[side] = (best_action_score[side] * (child_nums) + direction_node[i]->state.Points) / (child_nums+1);
									break;
								} else {
									trace_node = trace_node->parent;
								}
							}
						}

					} else {
						fprintf(stderr, "Invalid propagate mode!");
						exit(EXIT_FAILURE);
					}

					// push the new node to our priority queue
					if( (direction_node[i]->state).Lives == curr_node->state.Lives) {
						heap_push(&h, direction_node[i]);
					} else {
						// remove the node as it lost a life
						free(direction_node[i]);
					}

				} else {
					// remove the node as it is not a valid movement
					free(direction_node[i]);
				}
			}
		}
	}

	// free the memory of explored node and free explored itself
	for (unsigned i = 0; i < curr_used; i++){
		free(explored[i]);
	}
	free(explored);
	
	sprintf(stats, "Max Depth: %d Expanded nodes: %d  Generated nodes: %d\n",max_depth,expanded_nodes,generated_nodes);
	
	// retrive the action has the best score overall
	move_t best_action = left;
	float best_score = best_action_score[0];

	fprintf(stderr, "best_action_score[0]=%f  ", best_score);

	for (unsigned i = 1; i < DIRECTIONS; i++) {
		fprintf(stderr, "best_action_score[%d]=%f  ", i, best_action_score[i]);
		if (best_action_score[i] > best_score) {
			best_score = best_action_score[i];
			best_action = i;
		}
	}

	//if there are multiple best score, try to break tie randomly
	int res[4];
	int max_num=0;
	for (unsigned i = 0; i < DIRECTIONS; i++) {
		if(best_action_score[i] == best_score){
			res[max_num++] = i;
		}
	}
	best_action = res[rand() % max_num];

	fprintf(stderr, "\n final choice is %d", best_action);

	if(best_action == left)
		sprintf(stats, "%sSelected action: Left\n",stats);
	if(best_action == right)
		sprintf(stats, "%sSelected action: Right\n",stats);
	if(best_action == up)
		sprintf(stats, "%sSelected action: Up\n",stats);
	if(best_action == down)
		sprintf(stats, "%sSelected action: Down\n",stats);

	sprintf(stats, "%sScore Left %f Right %f Up %f Down %f",stats,best_action_score[left],best_action_score[right],best_action_score[up],best_action_score[down]);
	return best_action;
}

