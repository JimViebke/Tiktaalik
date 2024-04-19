#pragma once

#include "node.hpp"

namespace chess
{
	namespace detail
	{
		template<bool count_evals = false>
		float alpha_beta(Node& node, size_t depth, float alpha, float beta, size_t& n_of_evals)
		{
			if (node.children.size() == 0)
				node.generate_child_boards();

			if (depth == 0 || node.is_terminal())
			{
				if constexpr (count_evals)
				{
					++n_of_evals;
				}

				return node.evaluation();
			}

			evaluation_t eval = -INFINITY;
			for (Node& child : node.children)
			{
				eval = std::max(eval, -alpha_beta<count_evals>(child, depth - 1, -beta, -alpha, n_of_evals));
				if (eval > beta) break; // beta cutoff
				alpha = std::max(alpha, eval);
			}

			return eval;
		}
	}

	Node* alpha_beta(Node& root, size_t depth, bool maximizing_player, size_t& n_of_evals)
	{
		// The root node will usually already have all of its immediate children,
		// but have this here for correctness.
		if (root.children.size() == 0)
			root.generate_child_boards();

		float alpha = -INFINITY;
		float beta = INFINITY;
		Node* best_move = nullptr;

		if (maximizing_player)
		{
			evaluation_t eval = -INFINITY;
			for (Node& child : root.children)
			{
				const auto ab = -detail::alpha_beta<true>(child, depth - 1, -beta, -alpha, n_of_evals);
				if (ab > eval)
				{
					eval = ab;
					best_move = &child;
				}
				alpha = std::max(alpha, eval);
			}
		}
		else
		{
			evaluation_t eval = INFINITY;
			for (Node& child : root.children)
			{
				const auto ab = detail::alpha_beta<true>(child, depth - 1, alpha, beta, n_of_evals);
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
