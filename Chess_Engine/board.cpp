
/* Jim Viebke
Mar 14 2016 */

#include <iostream>

#include "board.h"

void Board::print_board(const Board & board, const unsigned & offset)
{
	for (unsigned rank = 0; rank < 8; ++rank)
	{
		// add indent
		for (unsigned i = 0; i < offset * 9; ++i) std::cout << ' ';

		for (unsigned file = 0; file < 8; ++file)
		{
			const Piece & piece = board.piece_at(rank, file);

			if (piece.is_empty()) std::cout << ".";
			else if (piece.is_white())
			{
				if (piece.is_rook()) std::cout << "R";
				else if (piece.is_bishop()) std::cout << "B";
				else if (piece.is_knight()) std::cout << "N";
				else if (piece.is_queen()) std::cout << "Q";
				else if (piece.is_king()) std::cout << "K";
			}
			else if (piece.is_black())
			{
				if (piece.is_rook()) std::cout << "r";
				else if (piece.is_bishop()) std::cout << "b";
				else if (piece.is_knight()) std::cout << "n";
				else if (piece.is_queen()) std::cout << "q";
				else if (piece.is_king()) std::cout << "k";
			}
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}
void Board::print_board(const std::list<Board> & boards)
{
	for (unsigned rank = 0; rank < 8; ++rank)
	{
		// render this rank for each board
		for (std::list<Board>::const_iterator it = boards.cbegin(); it != boards.cend(); ++it)
		{
			for (unsigned file = 0; file < 8; ++file)
			{
				const Piece & piece = it->piece_at(rank, file);

				if (piece.is_empty()) std::cout << ".";
				else if (piece.is_white())
				{
					if (piece.is_pawn()) std::cout << "P";
					else if (piece.is_rook()) std::cout << "R";
					else if (piece.is_bishop()) std::cout << "B";
					else if (piece.is_knight()) std::cout << "N";
					else if (piece.is_queen()) std::cout << "Q";
					else if (piece.is_king()) std::cout << "K";
				}
				else if (piece.is_black())
				{
					if (piece.is_pawn()) std::cout << "p";
					else if (piece.is_rook()) std::cout << "r";
					else if (piece.is_bishop()) std::cout << "b";
					else if (piece.is_knight()) std::cout << "n";
					else if (piece.is_queen()) std::cout << "q";
					else if (piece.is_king()) std::cout << "k";
				}
			}

			// add indent if there is another board to print
			if (it != --boards.cend())
				for (unsigned i = 0; i < 5; ++i) std::cout << ' ';
		}

		std::cout << std::endl;
	}
	std::cout << std::endl;
}

std::list<Board> Board::get_child_boards() const
{
	std::list<Board> child_boards;

	for (int rank = 0; rank < 8; ++rank)
	{
		for (int file = 0; file < 8; ++file)
		{
			// if this piece can move
			if (piece_at(rank, file).is_color(color_to_move))
			{
				if (piece_at(rank, file).is_pawn())
				{
					find_pawn_moves(child_boards, rank, file);
				}
				else if (piece_at(rank, file).is_rook())
				{
					find_rook_moves(child_boards, rank, file);
				}
				else if (piece_at(rank, file).is_bishop())
				{
					find_bishop_moves(child_boards, rank, file);
				}
				else if (piece_at(rank, file).is_knight())
				{
					find_knight_moves(child_boards, rank, file);
				}
				else if (piece_at(rank, file).is_queen())
				{
					find_queen_moves(child_boards, rank, file);
				}
				else if (piece_at(rank, file).is_king())
				{
					find_king_moves(child_boards, rank, file);
				}
			} // end if piece can move
		}
	}

	Board::remove_invalid_boards(child_boards);

	return child_boards;
}

void Board::remove_invalid_boards(std::list<Board> & boards)
{
	for (auto it = boards.begin(); it != boards.end(); )
	{
		if (!it->is_valid_position())
			it = boards.erase(it);
		else
			++it;
	}
}

bool Board::is_valid_position() const
{
	// It is fine if the moving player was placed into check.
	// The color that just moved may not be in check.
	if (is_king_in_check(other_color(color_to_move)))
	{
		return false;
	}

	return true;
}

// these are in the same order that they are called in the generation function, which acts in order of probable piece frequency

void Board::find_pawn_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	if (piece_at(rank, file).is_white())
	{
		if (bounds_check(rank - 1)) // only validate moving forwards once
		{
			// check for moving forward one square
			if (piece_at(rank - 1, file).is_empty())
				child_boards.emplace_back(*this, rank, file, rank - 1, file);
			// check for moving forward two squares
			if (rank == 6 && piece_at(5, file).is_empty() && piece_at(4, file).is_empty())
				child_boards.emplace_back(*this, rank, file, 4, file);
			// check for captures
			if (bounds_check(file + 1) && piece_at(rank - 1, file + 1).is_black())
				child_boards.emplace_back(*this, rank, file, rank - 1, file + 1);
			if (bounds_check(file - 1) && piece_at(rank - 1, file - 1).is_black())
				child_boards.emplace_back(*this, rank, file, rank - 1, file - 1);
			// check for en passant
			if (rank == 3)
			{
				if (en_passant_flag == file - 1 && bounds_check(file - 1) && piece_at(rank, file - 1).is_pawn())
					child_boards.emplace_back(*this, rank, file, rank - 1, file - 1);
				else if (en_passant_flag == file + 1 && bounds_check(file + 1) && piece_at(rank, file + 1).is_pawn())
					child_boards.emplace_back(*this, rank, file, rank - 1, file + 1);
			}
		}
	}
	else if (piece_at(rank, file).is_black())
	{
		if (bounds_check(rank + 1)) // only validate moving forwards once
		{
			// check for moving forward one square
			if (piece_at(rank + 1, file).is_empty())
				child_boards.emplace_back(*this, rank, file, rank + 1, file);
			// check for moving forward two squares
			if (rank == 1 && piece_at(2, file).is_empty() && piece_at(3, file).is_empty())
				child_boards.emplace_back(*this, rank, file, 3, file);
			// check for captures
			if (bounds_check(file + 1) && piece_at(rank + 1, file + 1).is_white())
				child_boards.emplace_back(*this, rank, file, rank + 1, file + 1);
			if (bounds_check(file - 1) && piece_at(rank + 1, file - 1).is_white())
				child_boards.emplace_back(*this, rank, file, rank + 1, file - 1);
			// check for en passant
			if (rank == 4)
			{
				if (en_passant_flag == file - 1 && bounds_check(file - 1) && piece_at(rank, file - 1).is_pawn())
					child_boards.emplace_back(*this, rank, file, rank + 1, file - 1);
				else if (en_passant_flag == file + 1 && bounds_check(file + 1) && piece_at(rank, file + 1).is_pawn())
					child_boards.emplace_back(*this, rank, file, rank + 1, file + 1);
			}
		}
	}
}
void Board::find_rook_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	// rank descending
	for (int end_rank = rank - 1; end_rank >= 0; --end_rank)
	{
		if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

		if (piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
		{
			child_boards.emplace_back(*this, rank, file, end_rank, file);
			continue; // keep searching in the current direction
		}
		// if the rook has encountered an enemy piece
		else if (piece_at(end_rank, file).is_opposing_color(color_to_move))
		{
			// the rook can capture...
			child_boards.emplace_back(*this, rank, file, end_rank, file);
			break; // ...but the rook cannot keep moving
		}
		else break; // the rook cannot move into a friendly piece; stop searching this way
	}

	// rank ascending (documentation same as above)
	for (int end_rank = rank + 1; end_rank < 8; ++end_rank)
	{
		if (!bounds_check(end_rank)) break;

		if (piece_at(end_rank, file).is_empty())
		{
			child_boards.emplace_back(*this, rank, file, end_rank, file);
			continue;
		}
		else if (piece_at(end_rank, file).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, end_rank, file);
			break;
		}
		else break;
	}

	// file descending (documentation same as above)
	for (int end_file = file - 1; end_file >= 0; --end_file)
	{
		if (!bounds_check(end_file)) break;

		if (piece_at(rank, end_file).is_empty())
		{
			child_boards.emplace_back(*this, rank, file, rank, end_file);
			continue;
		}
		else if (piece_at(rank, end_file).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, rank, end_file);
			break;
		}
		else break;
	}

	// file ascending (documentation same as above)
	for (int end_file = file + 1; end_file < 8; ++end_file)
	{
		if (!bounds_check(end_file)) break;

		if (piece_at(rank, end_file).is_empty())
		{
			child_boards.emplace_back(*this, rank, file, rank, end_file);
			continue;
		}
		else if (piece_at(rank, end_file).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, rank, end_file);
			break;
		}
		else break;
	}
}
void Board::find_bishop_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	// working diagonally (rank and file descending)
	for (int offset = 1; offset < 8; ++offset)
	{
		// if the location is off of the board, stop searching in this direction
		if (!bounds_check(rank - offset, file - offset)) break;

		// if the square is empty
		if (piece_at(rank - offset, file - offset).is_empty())
		{
			// the bishop can move here
			child_boards.emplace_back(*this, rank, file, rank - offset, file - offset);
			continue; // keep searching in this direction
		}
		// if the square is occupied by an enemy piece, the bishop can capture it
		else if (piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, rank - offset, file - offset);
			// the bishop made a capture, stop searching in this direction
			break;
		}
		// else, the square is occupied by a friendly piece, stop searching in this direction
		else break;
	}

	// working diagonally (rank descending and file ascending) (documentation same as above)
	for (int offset = 1; offset < 8; ++offset)
	{
		if (!bounds_check(rank - offset, file + offset)) break;

		if (piece_at(rank - offset, file + offset).is_empty())
		{
			child_boards.emplace_back(*this, rank, file, rank - offset, file + offset);
			continue;
		}
		else if (piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, rank - offset, file + offset);
			break;
		}
		else break;
	}

	// working diagonally (rank ascending and file descending) (documentation same as above)
	for (int offset = 1; offset < 8; ++offset)
	{
		if (!bounds_check(rank + offset, file - offset)) break;

		if (piece_at(rank + offset, file - offset).is_empty())
		{
			child_boards.emplace_back(*this, rank, file, rank + offset, file - offset);
			continue;
		}
		else if (piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, rank + offset, file - offset);
			break;
		}
		else break;
	}

	// working diagonally (rank and file ascending) (documentation same as above)
	for (int offset = 1; offset < 8; ++offset)
	{
		if (!bounds_check(rank + offset, file + offset)) break;

		if (piece_at(rank + offset, file + offset).is_empty())
		{
			child_boards.emplace_back(*this, rank, file, rank + offset, file + offset);
			continue;
		}
		else if (piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
		{
			child_boards.emplace_back(*this, rank, file, rank + offset, file + offset);
			break;
		}
		else break;
	}
}
void Board::find_knight_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	if (bounds_check(rank - 2, file + 1) && piece_at(rank - 2, file + 1).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank - 2, file + 1);
	if (bounds_check(rank - 1, file + 2) && piece_at(rank - 1, file + 2).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank - 1, file + 2);

	if (bounds_check(rank + 1, file + 2) && piece_at(rank + 1, file + 2).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank + 1, file + 2);
	if (bounds_check(rank + 2, file + 1) && piece_at(rank + 2, file + 1).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank + 2, file + 1);

	if (bounds_check(rank + 2, file - 1) && piece_at(rank + 2, file - 1).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank + 2, file - 1);
	if (bounds_check(rank + 1, file - 2) && piece_at(rank + 1, file - 2).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank + 1, file - 2);

	if (bounds_check(rank - 1, file - 2) && piece_at(rank - 1, file - 2).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank - 1, file - 2);
	if (bounds_check(rank - 2, file - 1) && piece_at(rank - 2, file - 1).is_opposing_color(color_to_move))
		child_boards.emplace_back(*this, rank, file, rank - 2, file - 1);
}
void Board::find_queen_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	// the function calls themselves are piece-agnostic, so there's no reason this shouldn't work
	find_rook_moves(child_boards, rank, file);
	find_bishop_moves(child_boards, rank, file);
}
void Board::find_king_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	// iterate over all adjacent squares
	for (int rank_d = -1; rank_d <= 1; ++rank_d)
	{
		for (int file_d = -1; file_d <= 1; ++file_d)
		{
			// if the square is not occupied by a friendly piece
			if (bounds_check(rank + rank_d, file + file_d) && piece_at(rank + rank_d, file + file_d).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(*this, rank, file, rank + rank_d, file + file_d);
			}
		}
	}
}

bool Board::is_king_in_check(const color check_color) const
{
	for (int rank = 0; rank < 8; ++rank)
	{
		for (int file = 0; file < 8; ++file)
		{
			// if the piece is a king of the color we're looking for
			if (piece_at(rank, file).is_king())
			{
				// extract the king (50% chance we won't have to do this again)
				const Piece & king = piece_at(rank, file);
				// if this king is the color for which to test for check
				if (king.is_color(check_color))
				{
					return is_king_in_check(king, rank, file);
				}
			}
		}
	}

	return false;
}

bool Board::is_king_in_check(const Piece king, const Rank rank, const File file) const
{
	// iterate over all adjacent squares to check for a king
	for (int rank_d = -1; rank_d <= 1; ++rank_d)
	{
		for (int file_d = -1; file_d <= 1; ++file_d)
		{
			if (rank_d == 0 && file_d == 0) continue; // don't examine the current square
			if (bounds_check(rank + rank_d, file + file_d) && piece_at(rank + rank_d, file + file_d).is_king())
			{
				return true; // the king is in check
			}
		}
	}

	// iterate in all four vertical and horizontal directions to check for a rook or queen (these loops only look within bounds)

	// rank descending
	for (int other_rank = rank - 1; other_rank >= 0; --other_rank)
	{
		// if a square is found that is not empty
		if (piece_at(other_rank, file).is_occupied())
		{
			// if the piece is a rook/queen AND is hostile, the king is in check
			if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
				&& piece_at(other_rank, file).is_opposing_color(king)) return true;
			break; // a piece was found in this direction, stop checking in this direction
		}
	}
	// rank ascending (documentation same as above)
	for (int other_rank = rank + 1; other_rank < 8; ++other_rank)
	{
		if (piece_at(other_rank, file).is_occupied())
		{
			if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
				&& piece_at(other_rank, file).is_opposing_color(king)) return true;
			break;
		}
	}
	// file descending (documentation same as above)
	for (int other_file = file - 1; other_file >= 0; --other_file)
	{
		if (piece_at(rank, other_file).is_occupied())
		{
			if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
				&& piece_at(rank, other_file).is_opposing_color(king)) return true;
			break;
		}
	}
	// file ascending (documentation same as above)
	for (int other_file = file + 1; other_file < 8; ++other_file)
	{
		if (piece_at(rank, other_file).is_occupied())
		{
			if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
				&& piece_at(rank, other_file).is_opposing_color(king)) return true;
			break;
		}
	}

	// iterate in all four diagonal directions to find a bishop or queen

	// search rank and file descending
	for (int offset = 1; offset < 8; ++offset)
	{
		// if the coordinates are in bounds
		if (!bounds_check(rank - offset, file - offset)) break;

		// if there is a piece here
		if (piece_at(rank - offset, file - offset).is_occupied())
		{
			// if the piece is a bishop/queen of the opposing color, the king is in check
			if ((piece_at(rank - offset, file - offset).is_bishop() || piece_at(rank - offset, file - offset).is_queen())
				&& piece_at(rank - offset, file - offset).is_opposing_color(king)) return true;
			break; // a piece is here, don't keep searching in this direction
		}
	}

	// search rank descending and file ascending (documentation same as above)
	for (int offset = 1; offset < 8; ++offset)
	{
		if (!bounds_check(rank - offset, file + offset)) break;

		if (piece_at(rank - offset, file + offset).is_occupied())
		{
			if ((piece_at(rank - offset, file + offset).is_bishop() || piece_at(rank - offset, file + offset).is_queen())
				&& piece_at(rank - offset, file + offset).is_opposing_color(king)) return true;
			break;
		}
	}

	// search rank ascending and file descending (documentation same as above)
	for (int offset = 1; offset < 8; ++offset)
	{
		if (!bounds_check(rank + offset, file - offset)) break;

		if (piece_at(rank + offset, file - offset).is_occupied())
		{
			if ((piece_at(rank + offset, file - offset).is_bishop() || piece_at(rank + offset, file - offset).is_queen())
				&& piece_at(rank + offset, file - offset).is_opposing_color(king)) return true;
			break;
		}
	}

	// search rank and file ascending (documentation same as above)
	for (int offset = 1; offset < 8; ++offset)
	{
		if (!bounds_check(rank + offset, file + offset)) break;

		if (piece_at(rank + offset, file + offset).is_occupied())
		{
			if ((piece_at(rank + offset, file + offset).is_bishop() || piece_at(rank + offset, file + offset).is_queen())
				&& piece_at(rank + offset, file + offset).is_opposing_color(king)) return true;
			break;
		}
	}

	// manually check eight locations for a knight
	if (bounds_check(rank - 2, file + 1) && piece_at(rank - 2, file + 1).is_knight() && piece_at(rank - 2, file + 1).is_opposing_color(king)) return true;
	if (bounds_check(rank - 1, file + 2) && piece_at(rank - 1, file + 2).is_knight() && piece_at(rank - 1, file + 2).is_opposing_color(king)) return true;
	if (bounds_check(rank + 1, file + 2) && piece_at(rank + 1, file + 2).is_knight() && piece_at(rank + 1, file + 2).is_opposing_color(king)) return true;
	if (bounds_check(rank + 2, file + 1) && piece_at(rank + 2, file + 1).is_knight() && piece_at(rank + 2, file + 1).is_opposing_color(king)) return true;
	if (bounds_check(rank + 2, file - 1) && piece_at(rank + 2, file - 1).is_knight() && piece_at(rank + 2, file - 1).is_opposing_color(king)) return true;
	if (bounds_check(rank + 1, file - 2) && piece_at(rank + 1, file - 2).is_knight() && piece_at(rank + 1, file - 2).is_opposing_color(king)) return true;
	if (bounds_check(rank - 1, file - 2) && piece_at(rank - 1, file - 2).is_knight() && piece_at(rank - 1, file - 2).is_opposing_color(king)) return true;
	if (bounds_check(rank - 2, file - 1) && piece_at(rank - 2, file - 1).is_knight() && piece_at(rank - 2, file - 1).is_opposing_color(king)) return true;

	// check if the white king is under attack by a black pawn
	if (king.is_white())
	{
		if ((bounds_check(rank - 1, file + 1) && piece_at(rank - 1, file + 1).is(color::black, piece::pawn)) ||
			(bounds_check(rank - 1, file - 1) && piece_at(rank - 1, file - 1).is(color::black, piece::pawn))) return true;
	}
	// check if the black king is under attack by a white pawn
	else if (king.is_black())
	{
		if ((bounds_check(rank + 1, file + 1) && piece_at(rank + 1, file + 1).is(color::white, piece::pawn)) ||
			(bounds_check(rank + 1, file - 1) && piece_at(rank + 1, file - 1).is(color::white, piece::pawn))) return true;
	}

	return false;
}

