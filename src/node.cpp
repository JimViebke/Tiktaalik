#include "move.hpp"
#include "node.hpp"

namespace chess
{
	template<color_t color_to_move>
	void node<color_to_move>::generate_child_boards(const position& position)
	{
		// Generate all immediate child nodes.
		// If any child node exists, all exist.

		clear_node(); // todo: remove me
		children.reserve(40); // todo: remove me

		const size_t end_idx = chess::generate_child_boards(*this, position);
	}

	template<color_t color_to_move>
	void node<color_to_move>::perft(const position& position, const depth_t max_depth)
	{
		if (max_depth == 0) return;

		std::map<depth_t, size_t> node_counter; // <depth, node count>
		// hardcode start position
		node_counter[0] = 1;

		const auto start_time = util::time_in_ms();
		perft(position, 1, max_depth, node_counter);

		std::cout << "Ply:\tNodes:\n";
		for (auto& counter : node_counter)
		{
			std::cout << counter.first << '\t' << counter.second << '\n';
		}

		std::cout << "(" << util::time_in_ms() - start_time << " ms)\n";
	}

	template<color_t color_to_move>
	void node<color_to_move>::divide(const position& position, const depth_t max_depth)
	{
		if (max_depth < 1)
		{
			std::cout << "Divide depth must be at least one.\n";
			return;
		}

		generate_child_boards(position);

		size_t total_nodes = 0;

		for (auto& node : children)
		{
			std::map<depth_t, size_t> node_counter; // <depth, node count>
			node.perft(position, 1, max_depth - 1, node_counter);

			const auto last = node_counter.crbegin();

			if (last != node_counter.crend())
			{
				std::cout << node.get_board().move_to_string() << ": " << last->second << '\n';
				total_nodes += last->second;
			}
		}

		std::cout << "Total nodes: " << total_nodes << '\n';
	}

	// inner perft
	template<color_t color_to_move>
	void node<color_to_move>::perft(const position& current_position,
									const depth_t depth,
									const depth_t max_depth,
									std::map<depth_t, size_t>& node_counter)
	{
		generate_child_boards(current_position);

		node_counter[depth] += children.size();

		if (depth < max_depth)
		{
			for (auto& child : children)
			{
				position child_position{};
				make_move<child.color_to_move()>(child_position, current_position, boards[child.index]);
				child.perft(child_position, depth + 1, max_depth, node_counter);
			}
		}
	}

	template void node<white>::generate_child_boards(const position&);
	template void node<black>::generate_child_boards(const position&);

	template void node<white>::perft(const position&, const depth_t);
	template void node<black>::perft(const position&, const depth_t);

	template void node<white>::divide(const position&, const depth_t);
	template void node<black>::divide(const position&, const depth_t);

	template void node<white>::perft(const position&, const depth_t, const depth_t, std::map<depth_t, size_t>&);
	template void node<black>::perft(const position&, const depth_t, const depth_t, std::map<depth_t, size_t>&);
}
