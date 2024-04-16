#pragma once

#include <iostream>
#include <map>

#include "board.hpp"
#include "types.hpp"

namespace chess
{
	class Node
	{
	public:
		Board board;
		std::vector<Node> children;

		// explicit Node(const std::vector<Piece> & set_board);
		explicit Node(const Board& set_board);

		bool is_terminal() const
		{
			// Anything other than "unknown" is a terminal (end) state.
			return board.get_result() != result::unknown;
		}

		evaluation_t evaluation() const
		{
			return board.evaluate_position();
		}

		void generate_child_boards()
		{
			// Generate all immediate child nodes with boards.
			// If any child node exists, all exist.
			if (children.size() != 0)
			{
				std::cout << "(skipping generating child boards for move " << board.move_to_string() << " with " << children.size() << " children)\n";
				return;
			}

			Board::board_list child_boards = board.generate_child_boards();

			children.reserve(child_boards.size());
			for (const Board& child_board : child_boards)
				children.emplace_back(child_board);
		}

	private:
		// void generate_ply(const unsigned & depth);
		void print_size() const;
		// void divide(const unsigned & depth);
		// static bool Node::rank_sort(const std::pair<std::string, size_t> & left, const std::pair<std::string, size_t> & right);

		void divide(std::map<size_t, size_t>& node_counter, const size_t& max_depth, const size_t& current_depth = 0)
		{
			if (current_depth == max_depth) return;

			generate_child_boards();

			// record the number of child boards
			node_counter[current_depth] += children.size();

			// descend the tree to find the children of each node
			for (Node& node : children)
				node.divide(node_counter, max_depth, current_depth + 1);

			// On our way back up, erase all child nodes. This time, we're only storing a maximum of A*D boards,
			// where A is the average number of child boards per position (~30) and D is the depth.
			children.clear();
		}

		void size(std::map<size_t, size_t>& node_counter, const unsigned& depth = 1) const;
	};
}
