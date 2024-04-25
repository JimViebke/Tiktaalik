#pragma once

#include "node.hpp"

namespace chess
{
	namespace detail
	{
		template<bool maximizing_player, bool count_evals = false>
		eval_t alpha_beta(Node& node, size_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
		{
			if (node.has_generated_children())
			{
				std::stable_sort(node.children.begin(), node.children.end(), [](const Node& a, const Node& b)
				{
					if constexpr (maximizing_player)
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

			eval_t eval = (maximizing_player ? eval::eval_min : eval::eval_max);

			for (Node& child : node.children)
			{
				child.set_eval(eval);
			}

			for (Node& child : node.children)
			{
				const eval_t ab = alpha_beta<!maximizing_player, count_evals>(child, depth - 1, alpha, beta, n_of_evals);

				if constexpr (maximizing_player)
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

	template<bool maximizing_player>
	Node* alpha_beta(Node& root, size_t depth, size_t& n_of_evals)
	{
		if (root.has_generated_children())
		{
			std::stable_sort(root.children.begin(), root.children.end(), [](const Node& a, const Node& b)
			{
				if constexpr (maximizing_player)
					return a.get_eval() > b.get_eval();
				else
					return a.get_eval() < b.get_eval();
			});
		}

		// The root node will usually already have all of its immediate children,
		// but have this here for correctness.
		if (!root.has_generated_children())
			root.generate_child_boards();

		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (maximizing_player ? eval::eval_min : eval::eval_max);
		// default to first move if one exists
		Node* best_move = (root.children.size() > 0) ? root.children.data() : nullptr;

		for (Node& child : root.children)
		{
			const eval_t ab = detail::alpha_beta<!maximizing_player, true>(child, depth - 1, alpha, beta, n_of_evals);

			if constexpr (maximizing_player)
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
