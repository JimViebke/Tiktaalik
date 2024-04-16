#pragma once

#include "node.hpp"

namespace chess
{
	namespace detail
	{
		float alpha_beta(Node& node, size_t depth, float alpha, float beta, bool maximizing_player)
		{
			if (depth == 0 || node.is_terminal())
				return node.evaluation();

			if (node.children.size() == 0)
				node.generate_child_boards();

			if (maximizing_player)
			{
				evaluation_t eval = -INFINITY;
				for (Node& child : node.children)
				{
					eval = std::max(eval, alpha_beta(child, depth - 1, alpha, beta, false));
					if (eval > beta) break; // beta cutoff
					alpha = std::max(alpha, eval);
				}
				return eval;
			}
			else
			{
				evaluation_t eval = INFINITY;
				for (Node& child : node.children)
				{
					eval = std::min(eval, alpha_beta(child, depth - 1, alpha, beta, true));
					if (eval < alpha) break; // alpha cutoff
					beta = std::min(beta, eval);
				}
				return eval;
			}
		}
	}

	std::string alpha_beta(Node& root, size_t depth, bool maximizing_player)
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
				const auto ab = detail::alpha_beta(child, depth - 1, alpha, beta, false);
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
				const auto ab = detail::alpha_beta(child, depth - 1, alpha, beta, true);
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
			return "";
		}

		return best_move->board.move_to_string();
	}
}
