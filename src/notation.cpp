#include "board.hpp"
#include "move.hpp"
#include "notation.hpp"
#include "position.hpp"

namespace chess
{
	void move_to_notation(std::stringstream& ss,
						  const size_t parent_idx,
						  const size_t child_idx,
						  const color_t child_color_to_move)
	{
		const position& parent_position = positions[parent_idx];
		const board& child_board = boards[child_idx];

		const size_t start_index = child_board.get_start_index();
		const size_t end_index = child_board.get_end_index();

		const piece piece = parent_position.piece_at(start_index);
		if (!piece.is_pawn())
		{
			ss << piece.to_algebraic_char();
		}
		// todo: Add logic for castling
		// todo: Add logic for en passant captures
		// todo: Add logic for promotion

		const std::string move = child_board.move_to_string();

		if (parent_position.is_occupied(end_index))
		{
			if (parent_position.piece_at(start_index).is_pawn())
			{
				ss << move[0];
			}
			ss << 'x';
		}

		// Add the destination coordinates.
		ss << move[2] << move[3];

		// Generate child boards for this position, to determine if it is terminal.
		if (child_color_to_move == white)
			generate_child_boards<white>(child_idx);
		else
			generate_child_boards<black>(child_idx);

		// Add notation for check and checkmate.
		if (is_king_in_check(positions[child_idx], child_color_to_move))
		{
			if (child_board.is_terminal())
				ss << '#';
			else
				ss << '+';
		}
	}
}
