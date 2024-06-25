#pragma once

#include <algorithm>

#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	extern std::atomic_bool searching;
	extern size_t nodes;

	namespace detail
	{
		extern chess::tt::transposition_table tt;
	}

	constexpr size_t max_depth = 256;

	extern std::array<std::array<board, max_depth>, max_depth> pv_moves;
	extern std::array<size_t, max_depth> pv_lengths;

	void update_pv(const size_t ply, const board& board);

	namespace detail
	{
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
			else // don't probe the TT for nodes that are leaves or close to leaves.
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
		eval_t alpha_beta(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta);
	}
}
