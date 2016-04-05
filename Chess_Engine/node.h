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

	void divide() const;

private:
	void size(std::map<size_t, size_t> & node_counter, const unsigned & depth = 1) const;
};
