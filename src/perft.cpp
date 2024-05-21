#include <iostream>
#include <map>

#include "move.hpp"
#include "perft.hpp"
#include "piece_defines.hpp"
#include "position.hpp"
#include "util/util.hpp"

namespace chess
{
	namespace detail
	{
		template<color_t color_to_move>
		void perft(size_t idx,
				   const depth_t depth,
				   const depth_t max_depth,
				   std::map<depth_t, size_t>& node_counter)
		{
			const size_t begin_idx = first_child_index(idx);
			const size_t end_idx = generate_child_boards<color_to_move, true>(idx);

			node_counter[depth] += (end_idx - begin_idx);

			if (depth < max_depth)
			{
				for (size_t child_idx = begin_idx; child_idx != end_idx; ++child_idx)
				{
					perft<other_color(color_to_move)>(child_idx, depth + 1, max_depth, node_counter);
				}
			}
		}
	}

	template<color_t color_to_move>
	void perft(const depth_t max_depth)
	{
		if (max_depth == 0) return;

		std::map<depth_t, size_t> node_counter; // <depth, node count>
		// hardcode start position
		node_counter[0] = 1;

		const auto start_time = util::time_in_ms();
		detail::perft<color_to_move>(0, 1, max_depth, node_counter);

		std::cout << "Ply:\tNodes:\n";
		for (auto& counter : node_counter)
		{
			std::cout << counter.first << '\t' << counter.second << '\n';
		}

		std::cout << "(" << util::time_in_ms() - start_time << " ms)\n";
	}

	template<color_t color_to_move>
	void divide(const depth_t max_depth)
	{
		if (max_depth < 1)
		{
			std::cout << "Divide depth must be at least one.\n";
			return;
		}

		const size_t end_idx = generate_child_boards<color_to_move>(0);

		size_t total_nodes = 0;

		for (size_t idx = first_sibling_index(end_idx); idx != end_idx; ++idx)
		{
			std::map<depth_t, size_t> node_counter; // <depth, node count>
			detail::perft<other_color(color_to_move)>(idx, 1, max_depth - 1, node_counter);

			const auto last = node_counter.crbegin();

			if (last != node_counter.crend())
			{
				std::cout << boards[idx].move_to_string() << ": " << last->second << '\n';
				total_nodes += last->second;
			}
		}

		std::cout << "Total nodes: " << total_nodes << '\n';
	}

	template void perft<white>(const depth_t);
	template void perft<black>(const depth_t);

	template void divide<white>(const depth_t);
	template void divide<black>(const depth_t);
}
