#include "search.hpp"

namespace chess
{
	namespace detail
	{
		std::array<position, max_ply + 1> positions{};

		chess::tt::transposition_table tt;

		template<bool white_to_move, typename child_nodes_t>
		__forceinline void stable_sort_children(child_nodes_t& children, const depth_t depth)
		{
			if (depth == 1)
			{
				// assume all of our children are new, and only sort by static eval
				std::stable_sort(children.begin(), children.end(), [](const auto& a, const auto& b)
				{
					if constexpr (white_to_move)
						return a.get_static_eval() > b.get_static_eval();
					else
						return a.get_static_eval() < b.get_static_eval();
				});
			}
			else
			{
				std::stable_sort(children.begin(), children.end(), [](const auto& a, const auto& b)
				{
					if constexpr (white_to_move)
						return a.get_eval() > b.get_eval();
					else
						return a.get_eval() < b.get_eval();
				});
			}
		}
	}

	template<typename node_t>
	eval_t detail::alpha_beta(node_t& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals)
	{
		using eval_type = tt::eval_type;

		if (depth == 0)
		{
			++n_of_evals;
			const eval_t eval = node.get_static_eval();

			//if constexpr (config::verify_incremental_static_eval)
			//	if (eval != positions[ply].evaluate_position()) // requires make_move(...) above this point
			//		std::cout << "Incremental and generated static evals mismatch\n";

			//tt.store(key, depth, eval_type::exact, eval);
			node.set_eval(eval);
			return eval;
		}

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

		stable_sort_children<node.white_to_move()>(node.children, depth);

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

	template eval_t detail::alpha_beta<node<white>>(node<white>& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
	template eval_t detail::alpha_beta<node<black>>(node<black>& node, const size_t ply, const depth_t depth, eval_t alpha, eval_t beta, size_t& n_of_evals);
}
