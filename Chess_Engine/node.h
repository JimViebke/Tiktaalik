#pragma once

#include <iostream>
#include <map>

#include "board.h"

class Node
{
private:
	Board board;
	unsigned my_ply_depth;
	std::list<Node> child_nodes;

	Node(const Board & set_board, unsigned set_my_ply_depth);

public:
	Node(const std::vector<Piece> & set_board);
	Node(const Board & set_board);

	void generate_ply(const unsigned & depth);

	void print_all() const;

	void print_size() const;

private:
	void size(std::map<size_t, size_t> & node_counter) const;
};
