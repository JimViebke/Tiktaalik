#include "search.hpp"
#include "util/util.hpp"

namespace chess
{
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

	template<color_t color_to_move>
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

		pv_lengths[ply] = ply;

		board& board = boards[idx];

		if (depth == 0)
		{
			return board.get_static_eval();
		}

		const tt::key key = board.get_key();

		{
			eval_t eval = 0;
			if (tt.probe(eval, key, depth, alpha, beta))
			{
				++tt.hit;
				return eval;
			}
			else
			{
				++tt.miss;
			}
		}

		const size_t begin_idx = first_child_index(idx);
		const size_t end_idx = generate_child_boards<color_to_move>(idx);

		if (board.is_terminal())
		{
			const eval_t eval = board.get_eval(); // Either min, max, or 0.
			tt.store(key, depth, eval_type::exact, eval);
			return eval;
		}

		get_evals_for_children(begin_idx, end_idx, depth);

		eval_t eval = (color_to_move == white ? eval::eval_min : eval::eval_max);
		eval_type node_eval_type = (color_to_move == white ? eval_type::alpha : eval_type::beta);

		for (size_t child_idx = begin_idx; child_idx < end_idx; ++child_idx)
		{
			swap_best_to_front<color_to_move>(child_idx, end_idx);

			eval_t ab = 0;
			if (depth < 4 || child_idx == begin_idx)
			{
				ab = alpha_beta<other_color(color_to_move)>(child_idx, ply + 1, depth - 1, alpha, beta);
			}
			else // We are at least 4 ply from a leaf node, and not on a first child; try a null window search.
			{
				if constexpr (color_to_move == white)
					ab = alpha_beta<other_color(color_to_move)>(child_idx, ply + 1, depth - 1, alpha, alpha + 1);
				else
					ab = alpha_beta<other_color(color_to_move)>(child_idx, ply + 1, depth - 1, beta - 1, beta);

				if (alpha < ab && ab < beta) // If the null window failed, redo the search with the normal (alpha, beta) window.
				{
					ab = alpha_beta<other_color(color_to_move)>(child_idx, ply + 1, depth - 1, alpha, beta);
				}
			}

			if (!searching) return 0;

			if constexpr (color_to_move == white)
			{
				if (ab > eval::eval_max - 100) --ab;

				eval = std::max(eval, ab);
				if (eval >= beta)
				{
					tt.store(key, depth, eval_type::beta, beta);
					return beta;
				}
				if (eval > alpha)
				{
					alpha = eval;
					node_eval_type = eval_type::exact;
					update_pv(ply, boards[child_idx]);
				}
			}
			else
			{
				if (ab < eval::eval_min + 100) ++ab;

				eval = std::min(eval, ab);
				if (eval <= alpha)
				{
					tt.store(key, depth, eval_type::alpha, alpha);
					return alpha;
				}
				if (eval < beta)
				{
					beta = eval;
					node_eval_type = eval_type::exact;
					update_pv(ply, boards[child_idx]);
				}
			}

			// If our immediate children are leaf nodes, sibling nodes cannot be an improvement (futility pruning).
			if (depth == 1) break;
		}

		tt.store(key, depth, node_eval_type, eval);
		return eval;
	}

	template eval_t detail::alpha_beta<white>(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta);
	template eval_t detail::alpha_beta<black>(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta);
}
