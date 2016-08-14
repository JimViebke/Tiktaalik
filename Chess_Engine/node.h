#pragma once

#include <iostream>

#include "board.h"

class Node
{
private:
	Board board;
	std::vector<Node> child_nodes;

public:
	Node(const std::vector<Piece> & set_board);
	Node(const Board & set_board);

	void generate_ply(const unsigned & depth);

	void print_all() const;

	void print_size() const;

	void divide(const unsigned & depth);

	static bool Node::rank_sort(const std::pair<std::string, size_t> & left, const std::pair<std::string, size_t> & right);

private:
	void divide(std::map<size_t, size_t> & node_counter, const size_t & max_depth, const size_t & current_depth = 0)
	{
		if (current_depth == max_depth) return;

		// generate child boards for this position
		const Board::board_list child_boards = board.get_child_boards();

		// save the child boards to the node
		child_nodes.reserve(child_boards.size()); // allocation costs 14%
		for (const Board & child_boards : child_boards)
			child_nodes.emplace_back(child_boards);

		// record the number of child boards
		node_counter[current_depth] += child_nodes.size();

		// descend the tree to find the children of each node
		for (Node & node : child_nodes)
			node.divide(node_counter, max_depth, current_depth + 1);

		// On our way back up, erase all child nodes. This time, we're only storing a maximum of A*D boards,
		// where A is the average number of child boards per position (~30) and D is the depth.
		child_nodes.clear();
	}

	void size(std::map<size_t, size_t> & node_counter, const unsigned & depth = 1) const;
};
