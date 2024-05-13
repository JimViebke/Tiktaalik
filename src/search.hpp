#pragma once

#include <algorithm>

#include "node.hpp"
#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	namespace detail
	{
		constexpr size_t max_ply = 10;
		extern std::array<position, max_ply + 1> positions;

		extern chess::tt::transposition_table tt;
		extern size_t tt_hit;
		extern size_t tt_miss;

		template<typename node_t>
		void make_move(const node_t& current_node, const size_t ply)
		{
			make_move(positions[ply], positions[ply - 1], current_node.board);
		}

		template<bool white_to_move, typename child_nodes_t>
		__forceinline void stable_sort_children(child_nodes_t& children)
		{
			std::stable_sort(children.begin(), children.end(), [](const auto& a, const auto& b)
			{
				if constexpr (white_to_move)
					return a.get_eval() > b.get_eval();
				else
					return a.get_eval() < b.get_eval();
			});
		}

		template<typename node_t>
		void order_moves(node_t& node, const size_t ply, const depth_t depth)
		{
			if constexpr (tt::config::use_tt_move_ordering)
			{
				if (depth > 1)
				{
					for (auto& child : node.children)
					{
						detail::make_move(child, ply);
						const tt::key child_key = tt::make_key(positions[ply], child.board);

						eval_t cached_eval = 0;
						const bool hit = detail::tt.simple_exact_probe(cached_eval, child_key);
						if (hit)
							++detail::tt_hit;
						else
							++detail::tt_miss;
						child.set_eval(hit ? cached_eval : child.get_static_eval());
					}
				}
				else // don't probe the TT for leaf nodes
				{
					for (auto& child : node.children)
					{
						child.set_eval(child.get_static_eval());
					}
				}
			}

			stable_sort_children<node.white_to_move()>(node.children);
		}

		template<typename node_t>
		eval_t alpha_beta(node_t& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
	}

	template<typename node_t>
	best_move_v search(node_t& node, depth_t depth, size_t& n_of_evals)
	{
		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);

		node.clear_node();
		node.generate_child_boards(detail::positions[0]);

		detail::order_moves(node, 1, depth);

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
