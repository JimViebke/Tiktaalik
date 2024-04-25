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

		using node_mask_t = uint8_t;

		explicit Node(const Board& set_board);

		bool has_generated_children() const { return node_mask & generated_children; }

		void generate_child_boards()
		{
			// Generate all immediate child nodes with boards.
			// If any child node exists, all exist.
			if (has_generated_children())
			{
				std::cout << "(skipping generating child boards for move " << board.move_to_string() << " with " << children.size() << " children)\n";
				return;
			}

			const auto& child_boards = board.generate_child_boards();
			children.reserve(child_boards.size());
			for (const Board& child_board : child_boards)
				children.emplace_back(child_board);

			set_has_generated_children();
		}

		bool is_terminal() const
		{
			// Anything other than "unknown" is a terminal (end) state.
			return board.get_result() != result::unknown;
		}

		eval_t evaluation() const
		{
			if (is_terminal())
			{
				const result result = board.get_result();
				if (result == result::white_wins_by_checkmate)
					return eval::eval_max;
				else if (result == result::black_wins_by_checkmate)
					return eval::eval_min;
				else if (result == result::draw_by_stalemate)
					return 0;
				else
					std::cout << "Unknown terminal state: [" << size_t(result) << "]\n";
			}

			return board.evaluate_position();
		}

		void perft(const size_t max_depth);
		void divide(const size_t max_depth);

	private:
		static constexpr node_mask_t generated_children = 1 << 0;

		void set_has_generated_children() { node_mask |= generated_children; }

		// inner perft
		void perft(const size_t depth,
				   const size_t max_depth,
				   std::map<size_t, size_t>& node_counter);
	};
}
