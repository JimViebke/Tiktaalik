#pragma once

#include <sstream>
#include <string>

#include "board.hpp"
#include "node.hpp"
#include "position.hpp"

namespace chess
{
	inline file char_to_file(const char c) { return c - 'a'; }
	inline rank char_to_rank(const char c) { return 8 - (c - '0'); }

	template<typename child_node_t>
	void move_to_notation(std::stringstream& ss,
						  const position& parent_position,
						  const child_node_t& child_node)
	{
		const std::string move = child_node.get_board().move_to_string();
		const auto start_file = char_to_file(move[0]);
		const auto start_rank = char_to_rank(move[1]);

		const piece piece = parent_position.piece_at(start_rank, start_file);
		if (!piece.is_pawn())
		{
			ss << piece.to_algebraic_char();
		}
		// todo: Add logic for castling
		// todo: Add logic for en passant captures
		// todo: Add logic for promotion

		const auto target_file = char_to_file(move[2]);
		const auto target_rank = char_to_rank(move[3]);
		if (parent_position.is_occupied(target_rank, target_file))
		{
			if (parent_position.piece_at(start_rank, start_file).is_pawn())
			{
				ss << move[0]; // a pawn is capturing; append the pawn's file
			}
			ss << 'x';
		}

		// add the destination coordinates
		ss << move[2] << move[3];

		position child_position{};
		make_move<child_node.color_to_move()>(child_position, parent_position, child_node.get_board());

		if (is_king_in_check(child_position, child_node_t::color_to_move()))
		{
			if (child_node.is_terminal())
				ss << '#';
			else
				ss << '+';
		}
	}
}
