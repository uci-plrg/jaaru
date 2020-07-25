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
	Node(Node *par);
	~Node();

	/** @return the parent Node to this Node; that is, the action that
	 * occurred previously in the stack. */
	Node * get_parent() const { return parent; }

	bool increment_read_from();
	bool read_from_empty() const;
	unsigned int read_from_size() const;
	void print_read_from();
	void add_read_from(ModelExecution *Execution, ModelAction *act);
	ModelAction * get_read_from() const;
	ModelAction * get_read_from(int i) const;
	int get_read_from_size() const;
	void print() const;

	MEMALLOC
private:
	Node * const parent;

	/**
	 * The set of past ModelActions that this the action at this Node may
	 * read from. Only meaningful if this Node represents a 'read' action.
	 */
	ModelVector<Pair<ModelExecution *, ModelAction *> > read_from;
	unsigned int read_from_idx;
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
	Node * get_next() const;
	void reset_execution();
	void pop_restofstack(int numAhead);
	void full_reset();
	int get_total_nodes() { return total_nodes; }

	void print() const;

	MEMALLOC
private:
	node_list_t node_list;

	/**
	 * @brief the index position of the current head Node
	 *
	 * This index is relative to node_list. The index should point to the
	 * current head Node. It is negative when the list is empty.
	 */
	int head_idx;
	int total_nodes;
};

#endif	/* __NODESTACK_H__ */
