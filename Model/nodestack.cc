#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <string.h>
#include "nodestack.h"
#include "action.h"
#include "common.h"
#include "threads-model.h"
#include "modeltypes.h"
#include "execution.h"

/**
 * @brief Node constructor
 *
 * Constructs a single Node for use in a NodeStack. Each Node is associated
 * with exactly one ModelAction (exception: the first Node should be created
 * as an empty stub, to represent the first thread "choice") and up to one
 * parent.
 *
 */
Node::Node(int mrf_size) :
	read_from_idx(0),
	rf_size(mrf_size)
{
}

/** @brief Node desctructor */
Node::~Node() {
}

/** Prints debugging info for the ModelAction associated with this Node */
void Node::print() const {
}

/****************************** read from ********************************/

/** @brief Prints info about read_from set */
void Node::print_read_from()
{
}

/**
 * Gets the next 'read_from' action from this Node. Only valid for a node
 * where this->action is a 'read'.
 */
int Node::get_read_from() const {
	return read_from_idx;
}

int Node::get_read_from_size() const
{
	return rf_size;
}

/**
 * Checks whether the readsfrom set for this node is empty.
 * @return true if the readsfrom set is empty.
 */
bool Node::read_from_empty() const {
	return (read_from_idx + 1) >= rf_size;
}

/**
 * Increments the index into the readsfrom set to explore the next item.
 * @return Returns false if we have explored all items.
 */
void Node::increment_read_from() {
	DBG();
	read_from_idx++;
}

/************************** end read from ********************************/

NodeStack::NodeStack() :
	node_list(),
	last_backtrack(NULL),
	curr_backtrack(NULL),
	head_idx(-1)
{
}

NodeStack::~NodeStack()
{
	for (unsigned int i = 0;i < node_list.size();i++)
		delete node_list[i];
}

void NodeStack::print() const
{
	model_print("............................................\n");
	model_print("NodeStack printing node_list:\n");
	for (unsigned int it = 0;it < node_list.size();it++) {
		if ((int)it == this->head_idx)
			model_print("vvv following action is the current iterator vvv\n");
		node_list[it]->print();
	}
	model_print("............................................\n");
}


/**
 * Empties the stack of all trailing nodes after a given position and calls the
 * destructor for each. This function is provided an offset which determines
 * how many nodes (relative to the current replay state) to save before popping
 * the stack.
 * @param numAhead gives the number of Nodes (including this Node) to skip over
 * before removing nodes.
 */
void NodeStack::pop_restofstack(int numAhead)
{
	/* Diverging from previous execution; clear out remainder of list */
	unsigned int it = head_idx + numAhead;
	for (unsigned int i = it;i < node_list.size();i++)
		delete node_list[i];
	node_list.resize(it);
}

/** Reset the node stack. */
void NodeStack::full_reset()
{
	for (unsigned int i = 0;i < node_list.size();i++)
		delete node_list[i];
	node_list.clear();
	reset_execution();
}

Node * NodeStack::get_head() const
{
	if (node_list.empty() || head_idx < 0)
		return NULL;
	return node_list[head_idx];
}

Node * NodeStack::get_next() {
	if (node_list.empty()) {
		DEBUG("Empty\n");
		return NULL;
	}
	unsigned int it = head_idx + 1;
	if (it == node_list.size()) {
		DEBUG("At end\n");
		return NULL;
	}
	head_idx++;
	return node_list[it];
}

void NodeStack::reset_execution() {
	curr_backtrack = last_backtrack;
	last_backtrack = NULL;
	head_idx = -1;
}

Node * NodeStack::explore_next(SnapVector<Pair<ModelExecution *, ModelAction *>* > * rf_set) {
	Node * node = get_next();
	if (node == NULL) {
		node = create_node(rf_set);
	} else {
		ASSERT(((int)rf_set->size()) == node->get_read_from_size());
		if (node == curr_backtrack) {
			node->increment_read_from();
			pop_restofstack(1);	//dump other nodes...
			curr_backtrack = NULL;
		}
	}
	if (!node->read_from_empty())
		last_backtrack = node;
	return node;
}

Node * NodeStack::create_node(SnapVector<Pair<ModelExecution *, ModelAction *>* > * rf_set) {
	Node * n = new Node(rf_set->size());
	node_list.push_back(n);
	head_idx++;
	return n;
}
