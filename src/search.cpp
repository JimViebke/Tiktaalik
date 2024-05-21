#include "search.hpp"

namespace chess
{
	namespace detail
	{
		chess::tt::transposition_table tt;
	}

	template<color_t color_to_move>
	eval_t detail::alpha_beta(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
	{
		using eval_type = tt::eval_type;

		board& board = boards[idx];

		if (depth == 0)
		{
			++n_of_evals;
			return board.get_static_eval();
		}

		const tt::key key = board.get_key();

		{
			eval_t eval = 0;
			if (tt.probe(eval, key, depth, alpha, beta))
			{
				return eval;
			}
		}

		const size_t begin_idx = first_child_index(idx);
		const size_t end_idx = generate_child_boards<color_to_move>(idx);

		if (board.is_terminal())
		{
			const eval_t eval = board.terminal_eval();
			tt.store(key, depth, eval_type::exact, eval);
			return eval;
		}

		get_evals_for_children(begin_idx, end_idx, depth);

		eval_t eval = (color_to_move == white ? eval::eval_min : eval::eval_max);
		eval_type node_eval_type = (color_to_move == white ? eval_type::alpha : eval_type::beta);

		for (size_t child_idx = begin_idx; child_idx < end_idx; ++child_idx)
		{
			swap_best_to_front<color_to_move>(child_idx, end_idx);

			eval_t ab = alpha_beta<other_color(color_to_move)>(child_idx, ply + 1, depth - 1, alpha, beta, n_of_evals);

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
				}
			}
		}

		tt.store(key, depth, node_eval_type, eval);
		return eval;
	}

	template eval_t detail::alpha_beta<white>(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
	template eval_t detail::alpha_beta<black>(const size_t idx, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
}
