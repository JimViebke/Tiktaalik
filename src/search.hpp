#pragma once

#include <algorithm>

#include "move.hpp"
#include "transposition_table.hpp"
#include "util/util.hpp"

namespace chess
{
	extern size_t root_ply;
	extern std::array<tt::key, eval::max_ply * 2> history;
	extern std::atomic_bool searching;
	extern bool pondering;
	extern util::timepoint scheduled_turn_end;
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
		inline_toggle void swap_tt_move_to_front(const packed_move tt_move,
												 const size_t begin_idx, const size_t end_idx)
		{
			if (tt_move == 0) return;

			for (size_t idx = begin_idx; idx < end_idx; ++idx)
			{
				if (boards[idx].move_is(tt_move))
				{
					std::swap(boards[begin_idx], boards[idx]);
					std::swap(positions[begin_idx], positions[idx]);
					return;
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

		template<color_t color_to_move, bool full_window = true>
		eval_t alpha_beta(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta);
	}
}
