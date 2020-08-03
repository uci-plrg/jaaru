/** @file nodestack.h
 *  @brief Stack of operations for use in backtracking.
 */

#ifndef __NODESTACK_H__
#define __NODESTACK_H__

#include <cstddef>
#include <inttypes.h>
#include "classlist.h"
#include "mymemory.h"
#include "stl-model.h"



/**
 * @brief A single node in a NodeStack
 *
 * Represents a single node in the NodeStack. Each Node is associated with up
 * to one action and up to one parent node. A node holds information
 * regarding the last action performed (the "associated action"), the thread
 * choices that have been explored (explored_children) and should be explored
 * (backtrack), and the actions that the last action may read from.
 */
class Node {
public:
	Node(int mrf_size);
	~Node();

	void increment_read_from();
	bool has_more_choices() const;
	void print_read_from();
	int get_choice() const;
	int get_read_from_size() const;
	void print() const;

	MEMALLOC
private:

	/**
	 * The set of past ModelActions that this the action at this Node may
	 * read from. Only meaningful if this Node represents a 'read' action.
	 */
	int read_from_idx;
	int rf_size;
};

typedef ModelVector<Node *> node_list_t;

/**
 * @brief A stack of nodes
 *
 * Holds a Node linked-list that can be used for holding backtracking,
 * may-read-from, and replay information. It is used primarily as a
 * stack-like structure, in that backtracking points and replay nodes are
 * only removed from the top (most recent).
 */
class NodeStack {
public:
	NodeStack();
	~NodeStack();

	Node * get_head() const;
	Node * get_next();
	void reset_execution();
	void repeat_prev_execution() { curr_backtrack = NULL; }
	void pop_restofstack(int numAhead);
	void full_reset();
	void print() const;
	bool has_another_execution() {return last_backtrack != NULL;}
	Node * create_node(uint numchoices);
	Node * explore_next(uint numchoises);

	MEMALLOC
private:
	node_list_t node_list;
	Node * last_backtrack;
	Node * curr_backtrack;
	/**
	 * @brief the index position of the current head Node
	 *
	 * This index is relative to node_list. The index should point to the
	 * current head Node. It is negative when the list is empty.
	 */
	int head_idx;
};

#endif	/* __NODESTACK_H__ */
