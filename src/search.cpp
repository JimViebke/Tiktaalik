#include "search.hpp"

namespace chess
{
	namespace detail
	{
		chess::tt::transposition_table tt;

		size_t tt_hit = 0;
		size_t tt_miss = 0;
	}

	template<typename node_t>
	eval_t detail::alpha_beta(node_t& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
	{
		using eval_type = tt::eval_type;

		if constexpr (config::verify_incremental_static_eval)
			if (node.get_static_eval() != positions[node.index].evaluate_position())
				std::cout << "Incremental and generated static evals mismatch\n";

		if (depth == 0)
		{
			++n_of_evals;
			return node.get_static_eval();
		}

		const position& position = positions[node.index];

		const tt::key key = tt::make_key<node.color_to_move()>(position, node.get_board());

		{
			eval_t eval = 0;
			if (tt.probe(eval, key, depth, alpha, beta))
			{
				return eval;
			}
		}

		node.generate_child_boards(position);

		if (node.is_terminal())
		{
			const eval_t eval = node.terminal_eval();
			tt.store(key, depth, eval_type::exact, eval);
			return eval;
		}

		order_moves(node, ply + 1, depth);

		eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);
		eval_type node_eval_type = (node.white_to_move() ? eval_type::alpha : eval_type::beta);

		for (auto& child : node.children)
		{
			eval_t ab = alpha_beta(child, ply + 1, depth - 1, alpha, beta, n_of_evals);

			if constexpr (node.white_to_move())
			{
				if (ab > eval::eval_max - 100) --ab;

				eval = std::max(eval, ab);
				if (eval >= beta)
				{
					tt.store(key, depth, eval_type::beta, beta);
					node.clear_node();
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
					node.clear_node();
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
		node.clear_node();
		return eval;
	}

	template eval_t detail::alpha_beta<node<white>>(node<white>& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
	template eval_t detail::alpha_beta<node<black>>(node<black>& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
}
