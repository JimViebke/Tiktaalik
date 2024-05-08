#pragma once

#include <iostream>
#include <map>
#include <variant>
#include <vector>

#include "board.hpp"
#include "move.hpp"
#include "types.hpp"
#include "util/util.hpp"

namespace chess
{
	template<color_t color_to_move>
	class node : public colorable<color_to_move>
	{
	public:
		using other_node_t = node<other(color_to_move)>;

		std::vector<other_node_t> children;
		board<color_to_move> board;

		using node_mask_t = uint8_t;

		explicit node(const chess::board<color_to_move> set_board) : board(set_board) {}

		bool has_generated_children() const { return node_mask & generated_children; }

		void set_eval(const eval_t set_eval) { eval = set_eval; }
		eval_t get_eval() const { return eval; }

		void generate_child_boards(const position& position)
		{
			// Generate all ply-1 child nodes.
			// If any child node exists, all exist.

			// Skip generating child boards if they have already been generated
			if (has_generated_children()) return;

			const auto& child_boards = chess::generate_child_boards(board, position);
			children.reserve(child_boards.size());
			for (const auto& child_board : child_boards)
				children.emplace_back(child_board);

			set_has_generated_children();
		}

		bool is_terminal() const
		{
			// Anything other than "unknown" is a terminal (end) state.
			return board.get_result() != result::unknown;
		}

		eval_t terminal_eval()
		{
			const result result = board.get_result();
			if (result == result::white_wins_by_checkmate ||
				result == result::black_wins_by_checkmate)
				eval = eval::eval_min;
			else if (result == result::draw_by_stalemate)
				eval = 0;
			else
				std::cout << "Unknown terminal state: [" << size_t(result) << "]\n";
			return eval;
		}

		void perft(const position& position, const size_t max_depth)
		{
			if (max_depth == 0) return;

			std::map<size_t, size_t> node_counter; // <depth, node count>
			// hardcode start position
			node_counter[0] = 1;

			const auto start_time = util::time_in_ms();
			perft(position, 1, max_depth, node_counter);

			std::cout << "Ply:\tNodes:\n";
			for (auto& counter : node_counter)
			{
				std::cout << counter.first << '\t' << counter.second << '\n';
			}

			std::cout << "(" << util::time_in_ms() - start_time << " ms)\n";
		}
		void divide(const position& position, const size_t max_depth)
		{
			if (max_depth < 1)
			{
				std::cout << "Divide depth must be at least one.\n";
				return;
			}

			// generate child boards if they don't exist already
			generate_child_boards(position);

			size_t total_nodes = 0;

			for (auto& node : children)
			{
				std::map<size_t, size_t> node_counter; // <depth, node count>
				node.perft(position, 1, max_depth - 1, node_counter);

				const auto last = node_counter.crbegin();

				if (last != node_counter.crend())
				{
					std::cout << node.board.move_to_string() << ": " << last->second << '\n';
					total_nodes += last->second;
				}
			}

			std::cout << "Total nodes: " << total_nodes << '\n';
		}

	private:
		static constexpr node_mask_t generated_children = 1 << 0;

		void set_has_generated_children() { node_mask |= generated_children; }
		void clear_has_generated_children() { node_mask &= ~generated_children; }

		eval_t eval = 0;

		node_mask_t node_mask = 0;

		// inner perft
		void perft(const position& current_position,
				   const size_t depth,
				   const size_t max_depth,
				   std::map<size_t, size_t>& node_counter)
		{
			generate_child_boards(current_position);

			node_counter[depth] += children.size();

			if (depth < max_depth)
			{
				for (auto& child : children)
				{
					position child_position{};
					make_move(child_position, current_position, child.board);
					child.perft(child_position, depth + 1, max_depth, node_counter);
				}
			}
		}

		friend node<white>;
		friend node<black>;
	};

	using root_v = std::variant<node<white>, node<black>>;
	using best_move_v = std::variant<node<white>*, node<black>*>;
}
