#pragma once

#include <iostream>
#include <map>
#include <variant>
#include <vector>

#include "board.hpp"
#include "types.hpp"
#include "util/util.hpp"

namespace chess
{
	template<color_t color_to_move>
	class node : public colorable<color_to_move>
	{
	public:
		using other_node_t = node<other_color(color_to_move)>;

		std::vector<other_node_t> children;
		size_t index = 0;

		using node_mask_t = uint8_t;

		explicit node(const size_t set_index) : index(set_index) {}

		const board& get_board() const { return boards[index]; }
		board& get_board() { return boards[index]; }

		void generate_child_boards(const position& position);
		void clear_node()
		{
			children.clear();
		}

		void perft(const position& position, const depth_t max_depth);
		void divide(const position& position, const depth_t max_depth);

	private:
		// inner perft
		void perft(const position& current_position,
				   const depth_t depth,
				   const depth_t max_depth,
				   std::map<depth_t, size_t>& node_counter);

		friend node<white>;
		friend node<black>;
	};

	using root_v = std::variant<node<white>, node<black>>;
	using best_move_v = std::variant<node<white>*, node<black>*>;
}
