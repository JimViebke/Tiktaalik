#pragma once

#include <algorithm>

#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	extern std::atomic_bool searching;

	namespace detail
	{
		extern chess::tt::transposition_table tt;

		inline_toggle void get_evals_for_children(const size_t begin_idx, const size_t end_idx)
		{
			size_t hits = 0;

			for (size_t idx = begin_idx; idx != end_idx; ++idx)
			{
				board& board = boards[idx];

				eval_t cached_eval = 0;
				const bool hit = detail::tt.simple_exact_probe(cached_eval, board.get_key());
				hits += hit;

				const eval_t static_eval = board.get_static_eval();

				board.set_eval(hit ? cached_eval : static_eval);
			}

			detail::tt.hit += hits;
			detail::tt.miss += (end_idx - begin_idx) - hits;
		}

		inline_toggle void get_evals_for_children(const size_t begin_idx, const size_t end_idx, const depth_t depth)
		{
			if (tt::config::use_tt_move_ordering && depth > 5)
			{
				get_evals_for_children(begin_idx, end_idx);
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

		template<color_t color_to_move>
		inline_toggle void swap_best_to_front(const size_t begin_idx, const size_t end_idx)
		{
			size_t best_index = begin_idx;
			eval_t best_eval = boards[begin_idx].get_eval();

			for (size_t idx = begin_idx + 1; idx < end_idx; ++idx)
			{
				const board& board = boards[idx];

				if constexpr (color_to_move == white)
				{
					if (board.get_eval() > best_eval)
					{
						best_index = idx;
						best_eval = board.get_eval();
					}
				}
				else
				{
					if (board.get_eval() < best_eval)
					{
						best_index = idx;
						best_eval = board.get_eval();
					}
				}
			}

			std::swap(boards[begin_idx], boards[best_index]);
			std::swap(positions[begin_idx], positions[best_index]);
		}

		template<color_t color_to_move>
		eval_t alpha_beta(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
	}

	template<color_t color_to_move>
	size_t search(const size_t end_idx, depth_t depth, size_t& n_of_evals)
	{
		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (color_to_move == white ? eval::eval_min : eval::eval_max);

		const size_t begin_idx = first_child_index(0);

		detail::get_evals_for_children(begin_idx, end_idx, depth);

		// default to first move if one exists
		size_t best_move_idx = (begin_idx != end_idx) ? begin_idx : 0;

		for (size_t idx = begin_idx; idx != end_idx; ++idx)
		{
			detail::swap_best_to_front<color_to_move>(idx, end_idx);

			const eval_t ab = detail::alpha_beta<other_color(color_to_move)>(idx, 1, depth - 1, alpha, beta, n_of_evals);

			if (!searching) return 0;

			if constexpr (color_to_move == white)
			{
				if (ab > eval)
				{
					eval = ab;
					best_move_idx = idx;
				}
				alpha = std::max(alpha, eval);
			}
			else
			{
				if (ab < eval)
				{
					eval = ab;
					best_move_idx = idx;
				}
				beta = std::min(beta, eval);
			}
		}

		if (best_move_idx == 0)
		{
			std::cout << "No best move found (node is likely terminal).\n";
		}

		return best_move_idx;
	}
}
