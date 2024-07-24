#include "search.hpp"
#include "util/util.hpp"

namespace chess
{
	size_t root_ply{ 0 };
	std::array<tt::key, eval::max_ply * 2> history{};
	std::atomic_bool searching{ false };
	bool pondering{ false };
	util::timepoint scheduled_turn_end{ 0 };
	size_t nodes{ 0 };

	namespace detail
	{
		chess::tt::transposition_table tt;
	}

	std::array<size_t, max_depth> pv_lengths;
	std::array<std::array<board, max_depth>, max_depth> pv_moves;

	void update_pv(const size_t ply, const board& board)
	{
		pv_moves[ply][ply] = board;
		for (size_t next_ply = ply + 1; next_ply < pv_lengths[ply + 1]; ++next_ply)
		{
			pv_moves[ply][next_ply] = pv_moves[ply + 1][next_ply];
		}

		pv_lengths[ply] = pv_lengths[ply + 1];
	}

	template<color_t color_to_move, bool quiescing, bool full_window>
	eval_t detail::alpha_beta(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta)
	{
		using eval_type = tt::eval_type;

		++nodes;
		if (nodes % 1024 == 0)
		{
			// Stop searching if we're out of time.
			if (util::time_in_ms() >= scheduled_turn_end)
			{
				searching = false;
				return 0;
			}
		}

		if constexpr (!quiescing && full_window)
			pv_lengths[ply] = ply;

		board& board = boards[idx];
		const tt::key key = board.get_key();

		if constexpr (!quiescing)
		{
			// If this position is a repetition, evaluate it as an (unfavorable) draw.
			// Todo: Scale contempt from large to small, so we don't try to draw over small disadvantages early in the game.
			const size_t fifty_move_counter = board.get_fifty_move_counter();
			auto history_end = history.data() + root_ply + ply;
			if (fifty_move_counter >= 4)
			{
				const auto earliest_possible_repetition = std::max(history_end - fifty_move_counter, history.data());

				for (auto history_ptr = history_end - 4; history_ptr >= earliest_possible_repetition; history_ptr -= 2)
					if (*history_ptr == key)
						// Evaluate a repeated position as unfavorable (-100 centipawns). These evals look
						// reversed because we are considering the position itself rather than its moves/children.
						// That is, if color_to_move == white, this evaluation is from black's perspective.
						return (color_to_move == white) ? 100 : -100;
			}

			*history_end = key; // This position is not a repetition; add it to history.

			if (depth == 0)
			{
				// Repeat the recursive call, with quiescing.
				return alpha_beta<color_to_move, true, full_window>(idx, ply, 0, alpha, beta);
			}
		}
		else
		{
			const eval_t stand_pat = board.get_eval();

			if constexpr (color_to_move == white)
			{
				if (stand_pat >= beta)
					return beta;
				alpha = std::max(alpha, stand_pat);
			}
			else
			{
				if (stand_pat <= alpha)
					return alpha;
				beta = std::min(beta, stand_pat);
			}
		}

		packed_move best_move = 0;
		if constexpr (!quiescing)
		{
			eval_t tt_eval = 0;
			if (tt.probe(tt_eval, best_move, key, depth, alpha, beta, ply))
			{
				return tt_eval;
			}
		}

		const size_t begin_idx = first_child_index(idx);
		const size_t end_idx = generate_child_boards<color_to_move, quiescing ? gen_moves::captures : gen_moves::all>(idx);

		if (begin_idx == end_idx)
		{
			// The position is either terminal (eval is min, max, or 0) or quiescent.
			const eval_t eval = board.get_eval();

			if constexpr (!quiescing)
				tt.store(key, depth, eval_type::exact, eval);

			// If we are quiescing or in stalemate (eval == 0), return eval.
			// Otherwise, the position is a checkmate; apply a distance penalty.
			return (quiescing || eval == 0) ? eval :
				((color_to_move == white) ? -eval::mate + ply : eval::mate - ply);
		}

		if (!quiescing && best_move != 0)
			swap_tt_move_to_front(best_move, begin_idx, end_idx);
		else
			swap_best_to_front<color_to_move>(begin_idx, end_idx);

		eval_t eval = (color_to_move == white ? -eval::mate : eval::mate);
		eval_type node_eval_type = (color_to_move == white ? eval_type::alpha : eval_type::beta);

		for (size_t child_idx = begin_idx; child_idx < end_idx; ++child_idx)
		{
			eval_t ab = 0;
			//if (depth < 4 || child_idx == begin_idx)
			//{
			ab = alpha_beta<other_color(color_to_move), quiescing, full_window>(child_idx, ply + !quiescing, depth - !quiescing, alpha, beta);
			//}
			//else // We are at least 4 ply from a leaf node, and not on a first child; try a null window search.
			//{
			//	if constexpr (color_to_move == white)
			//		ab = alpha_beta<other_color(color_to_move), false>(child_idx, ply + 1, depth - 1, alpha, alpha + 1);
			//	else
			//		ab = alpha_beta<other_color(color_to_move), false>(child_idx, ply + 1, depth - 1, beta - 1, beta);

			//	if (alpha < ab && ab < beta) // If the null window failed, redo the search with the normal (alpha, beta) window.
			//	{
			//		ab = alpha_beta<other_color(color_to_move), full_window>(child_idx, ply + 1, depth - 1, alpha, beta);
			//	}
			//}

			if (!searching) return 0;

			if constexpr (color_to_move == white)
			{
				eval = std::max(eval, ab);
				if (eval >= beta)
				{
					if constexpr (!quiescing && full_window)
						tt.store(key, depth, eval_type::beta, beta, ply, boards[child_idx].get_packed_move());
					return beta;
				}
				if (eval > alpha)
				{
					alpha = eval;
					node_eval_type = eval_type::exact;
					best_move = boards[child_idx].get_packed_move();
					if constexpr (!quiescing && full_window)
						update_pv(ply, boards[child_idx]);
				}
			}
			else
			{
				eval = std::min(eval, ab);
				if (eval <= alpha)
				{
					if constexpr (!quiescing && full_window)
						tt.store(key, depth, eval_type::alpha, alpha, ply, boards[child_idx].get_packed_move());
					return alpha;
				}
				if (eval < beta)
				{
					beta = eval;
					node_eval_type = eval_type::exact;
					best_move = boards[child_idx].get_packed_move();
					if constexpr (!quiescing && full_window)
						update_pv(ply, boards[child_idx]);
				}
			}

			swap_best_to_front<color_to_move>(child_idx + 1, end_idx);
		}

		// If no move was an improvement, best_move stays as whatever we previously read from the TT.
		if constexpr (!quiescing && full_window)
			tt.store(key, depth, node_eval_type, eval, ply, best_move);

		return eval;
	}

	template eval_t detail::alpha_beta<white, true>(const size_t, const size_t, const depth_t, eval_t, eval_t);
	template eval_t detail::alpha_beta<white, false>(const size_t, const size_t, const depth_t, eval_t, eval_t);
	template eval_t detail::alpha_beta<black, true>(const size_t, const size_t, const depth_t, eval_t, eval_t);
	template eval_t detail::alpha_beta<black, false>(const size_t, const size_t, const depth_t, eval_t, eval_t);
}
