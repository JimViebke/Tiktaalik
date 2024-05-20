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

		const rank start_rank = child_board.get_start_rank();
		const file start_file = child_board.get_start_file();
		const rank end_rank = child_board.get_end_rank();
		const file end_file = child_board.get_end_file();

		const piece piece = parent_position.piece_at(start_rank, start_file);
		if (!piece.is_pawn())
		{
			ss << piece.to_algebraic_char();
		}
		// todo: Add logic for castling
		// todo: Add logic for en passant captures
		// todo: Add logic for promotion

		const std::string move = child_board.move_to_string();

		if (parent_position.is_occupied(end_rank, end_file))
		{
			if (parent_position.piece_at(start_rank, start_file).is_pawn())
			{
				ss << move[0];
			}
			ss << 'x';
		}

		// add the destination coordinates
		ss << move[2] << move[3];

		position child_position{};
		if (child_color_to_move == white)
			make_move<white>(child_position, parent_position, child_board);
		else
			make_move<black>(child_position, parent_position, child_board);

		if (is_king_in_check(child_position, child_color_to_move))
		{
			if (child_board.is_terminal())
				ss << '#';
			else
				ss << '+';
		}
	}
}
