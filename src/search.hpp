#pragma once

#include <algorithm>

#include "node.hpp"
#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	namespace detail
	{
		extern chess::tt::transposition_table tt;
		extern size_t tt_hit;
		extern size_t tt_miss;

		template<typename node_t>
		void get_evals_for_children(node_t& parent_node, const depth_t depth)
		{
			const size_t begin_idx = first_child_index(parent_node.index);
			const size_t end_idx = begin_idx + parent_node.children.size();

			if (tt::config::use_tt_move_ordering && depth > 1)
			{
				for (size_t idx = begin_idx; idx != end_idx; ++idx)
				{
					board& board = boards[idx];

					const tt::key child_key = tt::make_key<node_t::other_color()>(positions[idx], board);

					eval_t cached_eval = 0;
					const bool hit = detail::tt.simple_exact_probe(cached_eval, child_key);
					if (hit)
						++detail::tt_hit;
					else
						++detail::tt_miss;

					board.set_eval(hit ? cached_eval : board.get_static_eval());
				}
			}
			else // don't probe the TT for leaf nodes
			{
				for (size_t idx = begin_idx; idx != end_idx; ++idx)
				{
					board& board = boards[idx];
					board.set_eval(board.get_static_eval());
				}
			}
		}

		template<bool white_to_move, typename child_nodes_t>
		__forceinline void stable_sort_children(child_nodes_t& children)
		{
			std::stable_sort(children.begin(), children.end(), [](const auto& a, const auto& b)
			{
				const board& board_a = boards[a.index];
				const board& board_b = boards[b.index];
				if constexpr (white_to_move)
					return board_a.get_eval() > board_b.get_eval();
				else
					return board_a.get_eval() < board_b.get_eval();
			});
		}

		template<typename node_t>
		eval_t alpha_beta(node_t& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
	}

	template<typename node_t>
	best_move_v search(node_t& node, depth_t depth, size_t& n_of_evals)
	{
		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);

		node.clear_node();
		node.generate_child_boards(positions[0]);

		detail::get_evals_for_children(node, depth);
		detail::stable_sort_children<node.white_to_move()>(node.children);

		// default to first move if one exists
		typename node_t::other_node_t* best_move = (node.children.size() > 0) ? node.children.data() : nullptr;

		for (auto& child : node.children)
		{
			const eval_t ab = detail::alpha_beta(child, 1, depth - 1, alpha, beta, n_of_evals);

			if constexpr (node.white_to_move())
			{
				if (ab > eval)
				{
					eval = ab;
					best_move = &child;
				}
				alpha = std::max(alpha, eval);
			}
			else
			{
				if (ab < eval)
				{
					eval = ab;
					best_move = &child;
				}
				beta = std::min(beta, eval);
			}
		}

		if (best_move == nullptr)
		{
			std::cout << "No best move found (node is likely terminal).\n";
		}

		return best_move;
	}
}
