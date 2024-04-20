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

		explicit Node(const Board& set_board);

		void generate_child_boards()
		{
			// Generate all immediate child nodes with boards.
			// If any child node exists, all exist.
			if (children.size() != 0)
			{
				std::cout << "(skipping generating child boards for move " << board.move_to_string() << " with " << children.size() << " children)\n";
				return;
			}

			const auto& child_boards = board.generate_child_boards();
			children.reserve(child_boards.size());
			for (const Board& child_board : child_boards)
				children.emplace_back(child_board);
		}

		bool is_terminal() const
		{
			// Anything other than "unknown" is a terminal (end) state.
			return board.get_result() != result::unknown;
		}

		evaluation_t evaluation() const
		{
			return board.evaluate_position();
		}

		void perft(const size_t max_depth);
		void divide(const size_t max_depth);

	private:
		// inner perft
		void perft(const size_t depth,
				   const size_t max_depth,
				   std::map<size_t, size_t>& node_counter);
	};
}
