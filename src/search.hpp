#pragma once

#include <algorithm>

#include "move.hpp"
#include "transposition_table.hpp"
#include "util/util.hpp"

namespace chess
{
	extern size_t root_ply;
	extern std::array<tt::key, eval::max_ply * 2> history;
	extern std::array<packed_move, eval::max_ply> killer_moves;
	extern std::atomic_bool searching;
	extern bool pondering;
	extern util::timepoint scheduled_turn_end;
	extern size_t nodes;

	namespace detail
	{
		extern chess::tt::transposition_table tt;
	}

	extern std::array<std::array<board, eval::max_ply>, eval::max_ply> pv_moves;
	extern std::array<size_t, eval::max_ply> pv_lengths;

	void update_pv(const size_t ply, const board& board);

	namespace detail
	{
		inline_toggle size_t swap_tt_move_to_front(const packed_move tt_move,
												   const size_t begin_idx, const size_t end_idx)
		{
			for (size_t idx = begin_idx + 1; idx < end_idx; ++idx)
			{
				if (boards[idx].move_is(tt_move))
				{
					std::swap(boards[begin_idx], boards[idx]);
					std::swap(positions[begin_idx], positions[idx]);
					return begin_idx + 1;
				}
			}

			return begin_idx;
		}

		inline_toggle size_t swap_killer_move_to_front(const size_t ply,
													   const size_t begin_idx, const size_t end_idx)
		{
			const packed_move killer_move = killer_moves[ply];

			size_t killer_move_idx = begin_idx;

			for (size_t idx = begin_idx; idx < end_idx; ++idx)
			{
				if (boards[idx].move_is(killer_move))
				{
					killer_move_idx = idx;
				}
			}

			std::swap(boards[begin_idx], boards[killer_move_idx]);
			std::swap(positions[begin_idx], positions[killer_move_idx]);
			return begin_idx + 1;
		}

		inline_toggle size_t swap_tt_and_killer_move_to_front(const packed_move tt_move, const size_t ply,
															  const size_t begin_idx, const size_t end_idx)
		{
			const packed_move killer_move = killer_moves[ply];

			// If the killer move and TT_move are the same move, only search for it once.
			if (killer_move == tt_move)
			{
				return swap_tt_move_to_front(tt_move, begin_idx, end_idx);
			}

			constexpr size_t unknown = size_t(-1);
			size_t killer_move_idx = unknown;
			size_t out_idx = begin_idx;

			for (size_t idx = begin_idx; idx < end_idx; ++idx)
			{
				const packed_move move = boards[idx].get_packed_move();

				if (move == tt_move)
				{
					std::swap(boards[begin_idx], boards[idx]);
					std::swap(positions[begin_idx], positions[idx]);
					++out_idx;

					if (killer_move_idx != unknown) // We already found the killer move.
					{
						if (killer_move_idx == begin_idx) // The TT move displaced the killer move.
							killer_move_idx = idx; // Fix killer_move_idx.
						break; // Both moves have been found, break.
					}
				}
				else if (move == killer_move)
				{
					killer_move_idx = idx;

					if (out_idx != begin_idx) // If we have already found the TT move, break.
						break;
				}
			}

			// If we found the killer move, swap it forward.
			if (killer_move_idx != unknown)
			{
				std::swap(boards[out_idx], boards[killer_move_idx]);
				std::swap(positions[out_idx], positions[killer_move_idx]);
				++out_idx;
			}

			return out_idx;
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

		// Misses en passant captures.
		inline bool is_capture(const position& parent_position, const board& child_board)
		{
			return parent_position.is_occupied(child_board.get_end_index());
		}

		// The first time begin_idx is a noncapture with a higher static eval than all remaining captures,
		// the remaining captures are "bad captures" and will no longer receive ordering preference.
		// The order of moves can affect what counts as a bad capture.
		template<color_t color_to_move>
		inline_toggle bool swap_best_good_capture_to_front(const size_t parent_idx, const size_t begin_idx, const size_t end_idx)
		{
			size_t best_index = begin_idx;
			eval_t best_eval = boards[begin_idx].get_eval();

			const position& parent_position = positions[parent_idx];

			bool found_good_capture = is_capture(parent_position, boards[begin_idx]);

			for (size_t idx = begin_idx + 1; idx < end_idx; ++idx)
			{
				const board& board = boards[idx];

				if (is_capture(parent_position, board))
				{
					if constexpr (color_to_move == white)
					{
						if (board.get_eval() > best_eval)
						{
							best_index = idx;
							best_eval = board.get_eval();
							found_good_capture = true;
						}
					}
					else
					{
						if (board.get_eval() < best_eval)
						{
							best_index = idx;
							best_eval = board.get_eval();
							found_good_capture = true;
						}
					}
				}
			}

			std::swap(boards[begin_idx], boards[best_index]);
			std::swap(positions[begin_idx], positions[best_index]);

			return found_good_capture;
		}

		template<color_t color_to_move, bool quiescing = false, bool full_window = true>
		eval_t alpha_beta(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta);
	}
}
