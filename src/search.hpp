#pragma once

#include "node.hpp"

namespace chess
{
	namespace detail
	{
		template<bool count_evals, typename node_t>
		eval_t alpha_beta(node_t& node, size_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
		{
			if (node.has_generated_children())
			{
				std::stable_sort(node.children.begin(), node.children.end(), [](const auto& a, const auto& b)
				{
					if constexpr (node.white_to_move())
						return a.get_eval() > b.get_eval();
					else
						return a.get_eval() < b.get_eval();
				});
			}
			else if (depth > 0)
			{
				// Generate child boards if this is not a leaf node.
				node.generate_child_boards();
			}

			if (depth == 0 || node.is_terminal())
			{
				if constexpr (count_evals) ++n_of_evals;

				return node.evaluate_position();
			}

			eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);

			for (auto& child : node.children)
			{
				child.set_eval(eval);
			}

			for (auto& child : node.children)
			{
				const eval_t ab = alpha_beta<count_evals>(child, depth - 1, alpha, beta, n_of_evals);

				if constexpr (node.white_to_move())
				{
					eval = std::max(eval, ab);
					if (eval >= beta) break;
					alpha = std::max(alpha, eval);
				}
				else
				{
					eval = std::min(eval, ab);
					if (eval <= alpha) break;
					beta = std::min(beta, eval);
				}
			}

			node.set_eval(eval);

			return eval;
		}
	}

	template<typename node_t>
	best_move_v alpha_beta(node_t& node, size_t depth, size_t& n_of_evals)
	{
		if (node.has_generated_children())
		{
			std::stable_sort(node.children.begin(), node.children.end(), [](const auto& a, const auto& b)
			{
				if constexpr (node.white_to_move())
					return a.get_eval() > b.get_eval();
				else
					return a.get_eval() < b.get_eval();
			});
		}

		// The passed node will usually already have all of its ply-1 children,
		// but have this here for correctness.
		if (!node.has_generated_children())
			node.generate_child_boards();

		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);
		// default to first move if one exists
		typename node_t::other_node_t* best_move = (node.children.size() > 0) ? node.children.data() : nullptr;

		for (auto& child : node.children)
		{
			const eval_t ab = detail::alpha_beta<true>(child, depth - 1, alpha, beta, n_of_evals);

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
