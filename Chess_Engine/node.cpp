
/* Jim Viebke
Mar 28 2016 */

#include <algorithm>

#include "node.h"

// Node::Node(const std::vector<Piece> & set_board) : board(set_board) {}

Node::Node(const Board & set_board) : board(set_board) {}

/*void Node::generate_ply(const unsigned & depth)
{
	// if we have not reached the desired ply depth
	if (depth > 0)
	{
		// generate child boards for this position
		const Board::board_list child_boards = get_child_boards();

		// for each child board
		for (const Board & child_board : child_boards)
		{
			// save the child board to its own node
			child_nodes.emplace_back(child_board);

			// make a recursive call to continue populating down the tree
			child_nodes.back().generate_ply(depth - 1);
		}
	}
}*/

//void Node::print_all() const
//{
//	static Board::board_list boards;
//	boards.push_back(board);
//
//	Board::print_board(boards);
//	std::cin.ignore();
//
//	for (const Node & node : child_nodes)
//	{
//		node.print_all();
//	}
//	boards.pop_back();
//}

void Node::print_size() const
{
	std::map<size_t, size_t> node_counter;
	// hardcode start position
	node_counter[0] = 1;

	size(node_counter);

	for (auto it = node_counter.begin(); it != node_counter.end(); ++it)
		std::cout << "Number of child nodes from ply " << it->first << ": " << it->second << ".\n";
}

/*
void Node::divide(const unsigned & depth)
{
	// generate child boards for this position
	const Board::board_list child_boards = get_child_boards();

	// save the child boards to the node
	for (const Board & child_board : child_boards)
		child_nodes.emplace_back(child_board);

	size_t total_nodes = 0;

	for (Node & node : child_nodes)
	{
		std::map<size_t, size_t> node_counter; // <depth, child count>
		node.divide(node_counter, depth);

		// print the move
		std::cout << node.get_move() << " ";
		if (node_counter.size() > 0)
		{
			const size_t moves = (----node_counter.end())->second;
			total_nodes += moves;
			std::cout << moves << "\n";
		}
		else
			std::cout << 0 << "\n";
	}

	std::cout << "Total nodes: " << total_nodes << std::endl;
}
*/

void Node::size(std::map<size_t, size_t> & node_counter, const unsigned & depth) const
{
	node_counter[depth] += children.size();
	for (const Node & child : children)
	{
		child.size(node_counter, depth + 1);
	}
}

// sort positions by rank instead of file (the default sort)
//bool Node::rank_sort(const std::pair<std::string, size_t> & left, const std::pair<std::string, size_t> & right)
//{
//	if (left.first[1] == right.first[1] && left.first[3] < right.first[3]) return true;
//
//	return (left.first[1] < right.first[1]);
//}
