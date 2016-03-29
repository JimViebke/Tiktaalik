
/* Jim Viebke
Mar 28 2016 */

#include "node.h"

Node::Node(const Board & set_board, unsigned set_my_ply_depth) :
	board(set_board), my_ply_depth(set_my_ply_depth) {}

Node::Node(const Board & set_board) :
	board(set_board), my_ply_depth(0) {}

void Node::generate_ply(const unsigned & depth)
{
	// if we have not reached the ply depth
	if (my_ply_depth < depth)
	{
		// generate child boards for this position
		const std::list<Board> temp = board.get_child_boards();

		// for each child board, add a node
		for (const Board & board : temp)
		{
			child_nodes.push_back(Node(board, my_ply_depth + 1));

			// make a recursive call to continue populating down the tree
			child_nodes.back().generate_ply(depth);
		}
	}
}

void Node::print_all() const
{
	static std::list<Board> boards;
	boards.push_back(board);

	Board::print_board(boards);
	std::cin.ignore();

	for (const Node & node : child_nodes)
	{
		node.print_all();
	}
	boards.pop_back();
}

void Node::print_size() const
{
	std::map<size_t, size_t> node_counter;
	size(node_counter);

	for (auto it = node_counter.begin(); it != node_counter.end(); ++it)
		std::cout << "Number of child nodes from ply " << it->first << ": " << it->second << ".\n";
}

void Node::size(std::map<size_t, size_t> & node_counter) const
{
	node_counter[my_ply_depth] += child_nodes.size();
	for (const Node & child : child_nodes)
	{
		child.size(node_counter);
	}
}
