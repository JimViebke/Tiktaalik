#pragma once

#include "node.hpp"

namespace chess
{
	namespace detail
	{
		constexpr size_t max_ply = 10;
		static std::array<position, max_ply + 1> positions{};

		template<typename node_t>
		void make_move(const node_t& current_node, const size_t ply)
		{
			make_move(positions[ply], positions[ply - 1], current_node.board);
		}

		template<typename node_t>
		eval_t alpha_beta(node_t& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
		{
			make_move(node, ply);

			if (depth == 0)
			{
				++n_of_evals;
				return node.evaluate(positions[ply]);
			}

			if (!node.has_generated_children())
			{
				node.generate_child_boards(positions[ply]);
			}

			if (node.is_terminal())
			{
				return node.terminal_eval();
			}

			std::stable_sort(node.children.begin(), node.children.end(), [](const auto& a, const auto& b)
			{
				if constexpr (node.white_to_move())
					return a.get_eval() > b.get_eval();
				else
					return a.get_eval() < b.get_eval();
			});

			eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);

			for (auto& child : node.children)
			{
				child.set_eval(eval);
			}

			for (auto& child : node.children)
			{
				eval_t ab = alpha_beta(child, ply + 1, depth - 1, alpha, beta, n_of_evals);

				if constexpr (node.white_to_move())
				{
					if (ab > eval::eval_max - 100) --ab;

					eval = std::max(eval, ab);
					if (eval >= beta) break;
					alpha = std::max(alpha, eval);
				}
				else
				{
					if (ab < eval::eval_min + 100) ++ab;

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
	best_move_v search(node_t& node, depth_t depth, size_t& n_of_evals)
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
			node.generate_child_boards(detail::positions[0]);

		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);
		// default to first move if one exists
		typename node_t::other_node_t* best_move = (node.children.size() > 0) ? node.children.data() : nullptr;

		for (auto& child : node.children)
		{
			const eval_t ab = detail::alpha_beta(child, 1, depth - 1, alpha, beta, n_of_evals);

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
