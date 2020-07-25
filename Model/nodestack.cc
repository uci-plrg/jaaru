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
 * @param params The model-checker parameters
 * @param act The ModelAction to associate with this Node. May be NULL.
 * @param par The parent Node in the NodeStack. May be NULL if there is no
 * parent.
 * @param nthreads The number of threads which exist at this point in the
 * execution trace.
 */
Node::Node(Node *par) :
	parent(par),
	read_from(),
	read_from_idx(0)
{
}

/** @brief Node desctructor */
Node::~Node() {
}

/** Prints debugging info for the ModelAction associated with this Node */
void Node::print() const {
	model_print("          read from: %s", read_from_empty() ? "empty" : "non-empty ");
	for (int i = read_from_idx + 1;i < (int)read_from.size();i++)
		model_print("[%d]", read_from[i].p2->get_seq_number());
	model_print("\n");
}

/****************************** read from ********************************/

/** @brief Prints info about read_from set */
void Node::print_read_from()
{
	for (unsigned int i = 0;i < read_from.size();i++)
		read_from[i].p2->print();
}

/**
 * Add an action to the read_from set.
 * @param act is the action to add
 */
void Node::add_read_from(ModelExecution * execution, ModelAction *act)
{
	read_from.push_back(Pair<ModelExecution *, ModelAction *>(execution,act));
}

/**
 * Gets the next 'read_from' action from this Node. Only valid for a node
 * where this->action is a 'read'.
 * @return The first element in read_from
 */
ModelAction * Node::get_read_from() const
{
	if (read_from_idx < read_from.size())
		return read_from[read_from_idx].p2;
	else
		return NULL;
}

ModelAction * Node::get_read_from(int i) const
{
	return read_from[i].p2;
}

int Node::get_read_from_size() const
{
	return read_from.size();
}

/**
 * Checks whether the readsfrom set for this node is empty.
 * @return true if the readsfrom set is empty.
 */
bool Node::read_from_empty() const
{
	return ((read_from_idx + 1) >= read_from.size());
}

/**
 * Increments the index into the readsfrom set to explore the next item.
 * @return Returns false if we have explored all items.
 */
bool Node::increment_read_from()
{
	DBG();
	if (read_from_idx < read_from.size()) {
		read_from_idx++;
		return read_from_idx < read_from.size();
	}
	return false;
}

/************************** end read from ********************************/

NodeStack::NodeStack() :
	node_list(),
	head_idx(-1),
	total_nodes(0)
{
	total_nodes++;
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
	total_nodes = 1;
}

Node * NodeStack::get_head() const
{
	if (node_list.empty() || head_idx < 0)
		return NULL;
	return node_list[head_idx];
}

Node * NodeStack::get_next() const
{
	if (node_list.empty()) {
		DEBUG("Empty\n");
		return NULL;
	}
	unsigned int it = head_idx + 1;
	if (it == node_list.size()) {
		DEBUG("At end\n");
		return NULL;
	}
	return node_list[it];
}

void NodeStack::reset_execution()
{
	head_idx = -1;
}
