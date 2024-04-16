
/* Jim Viebke
Mar 28 2016 */

#include <algorithm>

#include "node.hpp"
#include "util.hpp"

namespace chess
{
	Node::Node(const Board& set_board) : board(set_board) {}

	void Node::perft(const size_t max_depth)
	{
		if (max_depth == 0) return;

		std::map<size_t, size_t> node_counter; // <depth, node count>
		// hardcode start position
		node_counter[0] = 1;

		const auto start_time = util::time_in_ms();
		perft(1, max_depth, node_counter);

		std::cout << "Ply:\tNodes:\n";
		for (auto& counter : node_counter)
		{
			std::cout << counter.first << '\t' << counter.second << '\n';
		}

		std::cout << "(" << util::time_in_ms() - start_time << " ms)\n";
	}

	void Node::divide(const size_t max_depth)
	{
		if (max_depth < 1)
		{
			std::cout << "Divide depth must be at least one.\n";
			return;
		}

		// generate child boards if they don't exist already
		generate_child_boards();

		size_t total_nodes = 0;

		for (Node& node : children)
		{
			std::map<size_t, size_t> node_counter; // <depth, node count>
			node.perft(1, max_depth - 1, node_counter);

			const auto last = node_counter.crbegin();

			if (last != node_counter.crend())
			{
				std::cout << node.board.move_to_string() << ": " << last->second << '\n';
				total_nodes += last->second;
			}
		}

		std::cout << "Total nodes: " << total_nodes << '\n';
	}

	// perft inner
	void Node::perft(const size_t depth,
					 const size_t max_depth,
					 std::map<size_t, size_t>& node_counter)
	{
		generate_child_boards();

		node_counter[depth] += children.size();

		if (depth < max_depth)
		{
			for (auto& child : children)
			{
				child.perft(depth + 1, max_depth, node_counter);
			}
		}

		// We're not going to use these nodes again; clear them.
		children.clear();
	}
}
