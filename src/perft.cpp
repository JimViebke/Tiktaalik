#include <iostream>
#include <utility>

#include "move.hpp"
#include "perft.hpp"
#include "util/util.hpp"

namespace chess
{
	template <color color_to_move>
	size_t perft(size_t idx, const depth_t depth)
	{
		const size_t begin_idx = first_child_index(idx);
		const size_t end_idx = generate_child_boards<color_to_move, gen_moves::all, false, true>(idx);

		if (depth == 0)
		{
			return end_idx - begin_idx;
		}

		size_t count = 0;
		for (size_t idx = begin_idx; idx != end_idx; ++idx)
		{
			count += perft<other_color(color_to_move)>(idx, depth - 1);
		}
		return count;
	}

	template <color color_to_move>
	void divide(const depth_t max_depth)
	{
		if (max_depth < 1)
		{
			std::cout << "Divide depth must be at least one.\n";
			return;
		}

		const auto start_time = util::time_in_ms();
		size_t total_nodes = 0;
		const size_t end_idx = generate_child_boards<color_to_move, gen_moves::all, false, true>(0);

		for (size_t idx = first_child_index(0); idx != end_idx; ++idx)
		{
			size_t count = 1;

			if (max_depth > 1)
			{
				count = perft<other_color(color_to_move)>(idx, max_depth - 2);
			}

			std::cout << boards[idx].move_to_string() << ": " << count << '\n';
			total_nodes += count;
		}

		const auto elapsed_ms = std::max(util::timepoint{1}, util::time_in_ms() - start_time);
		std::cout << "\nLeaf nodes: " << total_nodes << '\n';
		// Divide by 1'000 to convert nodes/ms to Mnodes/s.
		std::cout << std::format("{} ms ({:.1f} Mnps)\n\n", elapsed_ms, (float(total_nodes) / elapsed_ms) / 1'000);
	}

	template void divide<white>(const depth_t);
	template void divide<black>(const depth_t);
}
