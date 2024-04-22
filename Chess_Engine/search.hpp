#pragma once

#include "node.hpp"

namespace chess
{
	namespace detail
	{
		template<bool maximizing_player, bool count_evals = false>
		evaluation_t alpha_beta(Node& node, size_t depth, evaluation_t alpha, evaluation_t beta, size_t& n_of_evals)
		{
			if (node.children.size() == 0)
				node.generate_child_boards();

			if (depth == 0 || node.is_terminal())
			{
				if constexpr (count_evals) ++n_of_evals;

				return node.evaluation();
			}

			evaluation_t eval = (maximizing_player ? -INFINITY : INFINITY);

			for (Node& child : node.children)
			{
				const evaluation_t ab = alpha_beta<!maximizing_player, count_evals>(child, depth - 1, alpha, beta, n_of_evals);

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

			return eval;
		}
	}

	template<bool maximizing_player>
	Node* alpha_beta(Node& root, size_t depth, size_t& n_of_evals)
	{
		// The root node will usually already have all of its immediate children,
		// but have this here for correctness.
		if (root.children.size() == 0)
			root.generate_child_boards();

		Node* best_move = nullptr;

		evaluation_t alpha = -INFINITY;
		evaluation_t beta = INFINITY;
		evaluation_t eval = (maximizing_player ? -INFINITY : INFINITY);

		for (Node& child : root.children)
		{
			const evaluation_t ab = detail::alpha_beta<!maximizing_player, true>(child, depth - 1, alpha, beta, n_of_evals);

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
