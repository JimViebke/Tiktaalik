#pragma once

#include <iostream>

#include "board.h"

class Node
{
public:
	Board board;
	std::vector<Node> children;

	// explicit Node(const std::vector<Piece> & set_board);
	explicit Node(const Board & set_board);

	void generate_child_boards()
	{
		// Generating all immediate child nodes is an atomic operation.
		// If any child nodes exist, all exist.
		if (children.size() != 0)
		{
			std::cout << "(skipped generating child boards of new position)\n";
			return;
		}

		// all feels kinda ugly
		Board::board_list child_boards = board.generate_child_boards();

		for (const auto& child_board : child_boards)
			if (child_board.position.size() != 64)
				std::cout << "Move generation returned a board without a position.";

		children.reserve(child_boards.size());
		for (const Board& child_board : child_boards)
			children.emplace_back(child_board);

		for (const auto& child_node : children)
			if (child_node.board.position.size() != 64)
				std::cout << "Created a node with a board, without a valid position.";
	}

	void audit_boards() const
	{
		if (board.position.size() != 64)
		{
			std::cout << "Node has invalid board!\n";
		}

		for (const auto& child : children)
		{
			if (child.board.position.size() != 64)
			{
				std::cout << "Child node has invalid board!";
			}
		}
	}

private:
	// stashing this as private until I figure out if I need its
	// void generate_ply(const unsigned & depth);
	// stashing this as private until I figure out if I need it
	void print_size() const;
	// stashing this as private until I figure out if I need it
	// void divide(const unsigned & depth);
	// stashing this as private until I figure out if I need it
	// static bool Node::rank_sort(const std::pair<std::string, size_t> & left, const std::pair<std::string, size_t> & right);

	void divide(std::map<size_t, size_t> & node_counter, const size_t & max_depth, const size_t & current_depth = 0)
	{
		if (current_depth == max_depth) return;

		generate_child_boards();

		// record the number of child boards
		node_counter[current_depth] += children.size();

		// descend the tree to find the children of each node
		for (Node & node : children)
			node.divide(node_counter, max_depth, current_depth + 1);

		// On our way back up, erase all child nodes. This time, we're only storing a maximum of A*D boards,
		// where A is the average number of child boards per position (~30) and D is the depth.
		children.clear();
	}

	void size(std::map<size_t, size_t> & node_counter, const unsigned & depth = 1) const;
};
