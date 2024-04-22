#include "notation.hpp"

namespace chess
{
	void move_to_notation(std::stringstream& ss,
						  const Node& parent_node,
						  const Node& result_node)
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
			result_node.board.get_color_to_move()))
		{
			ss << '+';
		}
	}

	std::string move_to_notation_str(const Node& parent_node,
									 const Node& result_node)
	{
		std::stringstream ss;
		move_to_notation(ss, parent_node, result_node);
		return ss.str();
	}
}
