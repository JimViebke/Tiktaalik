#pragma once

#include "node.hpp"
#include "transposition_table.hpp"

namespace chess
{
	namespace detail
	{
		constexpr size_t max_ply = 10;
		static std::array<position, max_ply + 1> positions{};

		static chess::tt::transposition_table tt;

		template<typename node_t>
		void make_move(const node_t& current_node, const size_t ply)
		{
			make_move(positions[ply], positions[ply - 1], current_node.board);
		}

		template<typename node_t>
		eval_t alpha_beta(node_t& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
		{
			using eval_type = tt::eval_type;

			make_move(node, ply);

			const tt::key key = tt::make_key(positions[ply], node.board);

			{
				eval_t eval = 0;
				if (tt.probe(eval, key, depth, alpha, beta))
				{
					node.set_eval(eval);
					return eval;
				}
			}

			if (depth == 0)
			{
				++n_of_evals;
				const eval_t eval = node.get_static_eval();

				if constexpr (config::verify_incremental_static_eval)
					if (eval != positions[ply].evaluate_position())
						std::cout << "Incremental and generated static evals mismatch\n";

				tt.store(key, depth, eval_type::exact, eval);
				node.set_eval(eval);
				return eval;
			}

			if (!node.has_generated_children())
			{
				node.generate_child_boards(positions[ply]);
			}

			if (node.is_terminal())
			{
				const eval_t eval = node.terminal_eval();
				tt.store(key, depth, eval_type::exact, eval);
				return eval;
			}

			eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);

			if constexpr (tt::config::use_tt_move_ordering)
			{
				for (auto& child : node.children)
				{
					// generate the child position
					make_move(child, ply + 1);
					// generate the child's zobrish hash
					const tt::key child_key = tt::make_key(positions[ply + 1], child.board);
					// set the node's eval to the cached eval, regardless of depth, or min/max if unknown
					eval_t cached_eval = 0;
					const bool hit = tt.probe(cached_eval, child_key, 0, alpha, beta); // call with 0 (no depth requirement)
					//if (hit) std::cout << "tt hit for sorting hint\n";
					node.set_eval(hit ? cached_eval : eval);
				}
			}

			std::stable_sort(node.children.begin(), node.children.end(), [](const auto& a, const auto& b)
			{
				if constexpr (node.white_to_move())
					return a.get_eval() > b.get_eval();
				else
					return a.get_eval() < b.get_eval();
			});

			eval_type node_eval_type = (node.white_to_move() ? eval_type::alpha : eval_type::beta);

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
					if (eval >= beta)
					{
						tt.store(key, depth, eval_type::beta, beta);
						node.set_eval(beta);
						return beta;
						// break;
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
						node.set_eval(alpha);
						return alpha;
						// break;
					}
					if (eval < beta)
					{
						beta = eval;
						node_eval_type = eval_type::exact;
					}
				}
			}

			tt.store(key, depth, node_eval_type, eval);
			node.set_eval(eval);
			return eval;
		}
	}

	template<typename node_t>
	best_move_v search(node_t& node, depth_t depth, size_t& n_of_evals)
	{
		eval_t alpha = eval::eval_min;
		eval_t beta = eval::eval_max;
		eval_t eval = (node.white_to_move() ? eval::eval_min : eval::eval_max);

		if constexpr (tt::config::use_tt_move_ordering)
		{
			for (auto& child : node.children)
			{
				// generate the child position
				detail::make_move(child, 1);
				// generate the child's zobrish hash
				const tt::key child_key = tt::make_key(detail::positions[1], child.board);
				// set the node's eval to the cached eval, regardless of depth, or min/max if unknown
				eval_t cached_eval = 0;
				const bool hit = detail::tt.probe(cached_eval, child_key, 0, alpha, beta); // call with 0 (no depth requirement)
				// if (hit) std::cout << "tt hit for sorting hint\n";
				node.set_eval(hit ? cached_eval : eval);
			}
		}

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
