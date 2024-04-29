#pragma once

#include <sstream>
#include <string>

#include "node.hpp"

namespace chess
{
	inline file char_to_file(const char c) { return c - 'a'; }
	inline rank char_to_rank(const char c) { return 8 - (c - '0'); }

	template<typename parent_node_t, typename result_node_t>
		requires (!std::is_same<parent_node_t, result_node_t>::value)
	void move_to_notation(std::stringstream& ss,
						  const parent_node_t& parent_node,
						  const result_node_t& result_node)
	{
		const std::string move = result_node.board.move_to_string();
		const auto start_file = char_to_file(move[0]);
		const auto start_rank = char_to_rank(move[1]);

		const piece piece = parent_node.board.piece_at(start_rank, start_file);
		if (!piece.is_pawn())
		{
			ss << piece.to_algebraic_char();
		}
		// todo: Add logic for castling
		// todo: Add logic for en passant captures
		// todo: Add logic for promotion

		const auto target_file = char_to_file(move[2]);
		const auto target_rank = char_to_rank(move[3]);
		if (parent_node.board.is_occupied(target_rank, target_file))
		{
			if (parent_node.board.piece_at(start_rank, start_file).is_pawn())
			{
				ss << move[0]; // a pawn is capturing; append the pawn's file
			}
			ss << 'x';
		}

		// add the destination coordinates
		ss << move[2] << move[3];

		if (result_node.board.is_king_in_check(
			result_node_t::color_to_move()))
		{
			if (result_node.is_terminal())
				ss << '#';
			else
				ss << '+';
		}
	}

	template<typename parent_node_t, typename result_node_t>
		requires (!std::is_same<parent_node_t, result_node_t>::value)
	std::string move_to_notation_str(const parent_node_t& parent_node,
									 const result_node_t& result_node)
	{
		std::stringstream ss;
		move_to_notation(ss, parent_node, result_node);
		return ss.str();
	}
}
