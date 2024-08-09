/* 2021 December 16 */

#include "bitboard.hpp"
#include "game.hpp"
#include "move.hpp"
#include "search.hpp"

namespace chess
{
	template <color_t color_to_move>
	eval_t Game::search(const size_t end_idx, const depth_t depth)
	{
		++nodes;

		eval_t alpha = -eval::mate;
		eval_t beta = eval::mate;
		eval_t eval = (color_to_move == white ? -eval::mate : eval::mate);

		eval_t tt_eval = 0; // ignored
		packed_move tt_move = 0;
		detail::tt.probe(tt_eval, tt_move, boards[0].get_key(), depth, alpha, beta, 0);

		const size_t begin_idx = first_child_index(0);

		detail::swap_tt_move_to_front(tt_move, begin_idx, end_idx);

		for (size_t child_idx = begin_idx; child_idx != end_idx; ++child_idx)
		{
			eval_t ab = detail::alpha_beta<other_color(color_to_move)>(child_idx, 1, depth - 1, alpha, beta);

			if (!searching) return eval;

			if constexpr (color_to_move == white)
			{
				if (ab > eval)
				{
					eval = ab;
					update_pv(0, boards[child_idx]);
					send_info(eval);
					tt_move = boards[child_idx].get_packed_move();
				}
				alpha = std::max(alpha, eval);
			}
			else
			{
				if (ab < eval)
				{
					eval = ab;
					update_pv(0, boards[child_idx]);
					send_info(eval);
					tt_move = boards[child_idx].get_packed_move();
				}
				beta = std::min(beta, eval);
			}

			detail::swap_best_to_front<color_to_move>(child_idx + 1, end_idx);
		}

		// Store the best move in the TT.
		detail::tt.store(boards[0].get_key(), depth, tt::eval_type::exact, eval, 0, tt_move);

		return eval;
	}

	template eval_t Game::search<white>(const size_t, depth_t);
	template eval_t Game::search<black>(const size_t, depth_t);
}
