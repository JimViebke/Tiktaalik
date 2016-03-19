
/* Jim Viebke
Mar 14 2016 */

#include "board.h"

std::list<Board> Board::get_child_boards() const
{
	std::list<Board> child_boards;

	for (int rank = 0; rank < 8; ++rank)
	{
		for (int file = 0; file < 8; ++file)
		{
			// if this piece can move
			if (piece_at(rank, file).is_color(to_move))
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
	// for each square on the board
	for (int rank = 0; rank < 8; ++rank)
	{
		for (int file = 0; file < 8; ++file)
		{
			// if the piece is a king
			if (piece_at(rank, file).is_king())
			{
				// iterate over all adjacent squares to check for a king
				for (int rank_d = -1; rank_d <= 1; ++rank_d)
				{
					for (int file_d = -1; file_d <= 1; ++file_d)
					{
						if (rank_d == 0 && file_d == 0) continue; // don't examine the current square
						if (bounds_check(rank + rank_d, file + file_d) && piece_at(rank + rank_d, file + file_d).is_king())
						{
							return false;
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
						// if the piece is a rook AND is hostile AND it is the hostile piece's turn, then the position is invalid
						if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
							&& piece_at(other_rank, file).is_opposing_color(piece_at(rank, file))
							&& piece_at(other_rank, file).is_color(to_move)) return false;
						break; // a piece was found in this direction, stop checking in this direction
					}
				}
				// rank ascending (documentation same as above)
				for (int other_rank = rank + 1; other_rank < 8; ++other_rank)
				{
					if (piece_at(other_rank, file).is_occupied())
					{
						if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
							&& piece_at(other_rank, file).is_opposing_color(piece_at(rank, file)) && piece_at(other_rank, file).is_color(to_move)) return false;
						break;
					}
				}
				// file descending (documentation same as above)
				for (int other_file = file - 1; other_file >= 0; --other_file)
				{
					if (piece_at(rank, other_file).is_occupied())
					{
						if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
							&& piece_at(rank, other_file).is_opposing_color(piece_at(rank, file))
							&& piece_at(rank, other_file).is_color(to_move)) return false;
						break;
					}
				}
				// file ascending (documentation same as above)
				for (int other_file = file + 1; other_file < 8; ++other_file)
				{
					if (piece_at(rank, other_file).is_occupied())
					{
						if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
							&& piece_at(rank, other_file).is_opposing_color(piece_at(rank, file))
							&& piece_at(rank, other_file).is_color(to_move)) return false;
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
						// if the piece is a bishop of the opposing color and it is the opposing color's turn to move, the position is invalid, return false
						if ((piece_at(rank - offset, file - offset).is_bishop() || piece_at(rank - offset, file - offset).is_queen())
							&& piece_at(rank - offset, file - offset).is_opposing_color(piece_at(rank, file))
							&& piece_at(rank - offset, file - offset).is_color(to_move)) return false;
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
							&& piece_at(rank - offset, file + offset).is_opposing_color(piece_at(rank, file))
							&& piece_at(rank - offset, file + offset).is_color(to_move)) return false;
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
							&& piece_at(rank + offset, file - offset).is_opposing_color(piece_at(rank, file))
							&& piece_at(rank + offset, file - offset).is_color(to_move)) return false;
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
							&& piece_at(rank + offset, file + offset).is_opposing_color(piece_at(rank, file))
							&& piece_at(rank + offset, file + offset).is_color(to_move)) return false;
						break;
					}
				}

				// manually check eight locations for a knight
				if (bounds_check(rank - 1, file - 2) && piece_at(rank - 1, file - 2).is_knight() && piece_at(rank - 1, file - 2).is_color(to_move)) return false;
				if (bounds_check(rank - 1, file + 2) && piece_at(rank - 1, file + 2).is_knight() && piece_at(rank - 1, file + 2).is_color(to_move)) return false;
				if (bounds_check(rank + 1, file - 2) && piece_at(rank + 1, file - 2).is_knight() && piece_at(rank + 1, file - 2).is_color(to_move)) return false;
				if (bounds_check(rank + 1, file + 2) && piece_at(rank + 1, file + 2).is_knight() && piece_at(rank + 1, file + 2).is_color(to_move)) return false;
				if (bounds_check(rank - 2, file - 1) && piece_at(rank - 2, file - 1).is_knight() && piece_at(rank - 2, file - 1).is_color(to_move)) return false;
				if (bounds_check(rank - 2, file + 1) && piece_at(rank - 2, file + 1).is_knight() && piece_at(rank - 2, file + 1).is_color(to_move)) return false;
				if (bounds_check(rank + 2, file - 1) && piece_at(rank + 2, file - 1).is_knight() && piece_at(rank + 2, file - 1).is_color(to_move)) return false;
				if (bounds_check(rank + 2, file + 1) && piece_at(rank + 2, file + 1).is_knight() && piece_at(rank + 2, file + 1).is_color(to_move)) return false;

				// check if the white king is under attack by a black pawn ready to move
				if (piece_at(rank, file).is_white() && to_move != color::white)
				{
					if ((bounds_check(rank - 1, file + 1) && piece_at(rank - 1, file + 1).is(color::black, piece::pawn)) ||
						(bounds_check(rank - 1, file - 1) && piece_at(rank - 1, file - 1).is(color::black, piece::pawn))) return false;
				}
				// check if the black king is under attack by a white pawn ready to move
				else if (piece_at(rank, file).is_black() && to_move != color::black)
				{
					if ((bounds_check(rank + 1, file + 1) && piece_at(rank + 1, file + 1).is(color::white, piece::pawn)) ||
						(bounds_check(rank + 1, file - 1) && piece_at(rank + 1, file - 1).is(color::white, piece::pawn))) return false;
				}

			} // end if is king
		}
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
				child_boards.push_back(Board(*this, rank, file, rank - 1, file));
			// check for moving forward two squares
			if (rank == 6 && piece_at(5, file).is_empty() && piece_at(4, file).is_empty())
				child_boards.push_back(Board(*this, rank, file, 4, file));
			// check for captures
			if (bounds_check(file + 1) && piece_at(rank - 1, file + 1).is_black())
				child_boards.push_back(Board(*this, rank, file, rank - 1, file + 1));
			if (bounds_check(file - 1) && piece_at(rank - 1, file - 1).is_black())
				child_boards.push_back(Board(*this, rank, file, rank - 1, file - 1));
		}
	}
	else if (piece_at(rank, file).is_black())
	{
		if (bounds_check(rank + 1)) // only validate moving forwards once
		{
			// check for moving forward one square
			if (piece_at(rank + 1, file).is_empty())
				child_boards.push_back(Board(*this, rank, file, rank + 1, file));
			// check for moving forward two squares
			if (rank == 1 && piece_at(2, file).is_empty() && piece_at(3, file).is_empty())
				child_boards.push_back(Board(*this, rank, file, 3, file));
			// check for captures
			if (bounds_check(file + 1) && piece_at(rank + 1, file + 1).is_white())
				child_boards.push_back(Board(*this, rank, file, rank + 1, file + 1));
			if (bounds_check(file - 1) && piece_at(rank + 1, file - 1).is_white())
				child_boards.push_back(Board(*this, rank, file, rank + 1, file - 1));
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
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
			continue; // keep searching in the current direction
		}
		// if the rook has encountered an enemy piece
		else if (piece_at(end_rank, file).is_opposing_color(to_move))
		{
			// the rook can capture...
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
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
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
			continue;
		}
		else if (piece_at(end_rank, file).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
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
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
			continue;
		}
		else if (piece_at(rank, end_file).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
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
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
			continue;
		}
		else if (piece_at(rank, end_file).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
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
			child_boards.push_back(Board(*this, rank, file, rank - offset, file - offset));
			continue; // keep searching in this direction
		}
		// if the square is occupied by an enemy piece, the bishop can capture it
		else if (piece_at(rank - offset, file - offset).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank - offset, file - offset));
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
			child_boards.push_back(Board(*this, rank, file, rank - offset, file + offset));
			continue;
		}
		else if (piece_at(rank - offset, file + offset).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank - offset, file + offset));
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
			child_boards.push_back(Board(*this, rank, file, rank + offset, file - offset));
			continue;
		}
		else if (piece_at(rank + offset, file - offset).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank + offset, file - offset));
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
			child_boards.push_back(Board(*this, rank, file, rank + offset, file + offset));
			continue;
		}
		else if (piece_at(rank + offset, file + offset).is_opposing_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank + offset, file + offset));
			break;
		}
		else break;
	}
}
void Board::find_knight_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	if (bounds_check(rank - 1, file - 2) && !piece_at(rank - 1, file - 2).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank - 1, file - 2));
	if (bounds_check(rank - 1, file + 2) && !piece_at(rank - 1, file + 2).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank - 1, file + 2));

	if (bounds_check(rank + 1, file - 2) && !piece_at(rank + 1, file - 2).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank + 1, file - 2));
	if (bounds_check(rank + 1, file + 2) && !piece_at(rank + 1, file + 2).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank + 1, file + 2));

	if (bounds_check(rank - 2, file - 1) && !piece_at(rank - 2, file - 1).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank - 2, file - 1));
	if (bounds_check(rank - 2, file + 1) && !piece_at(rank - 2, file + 1).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank - 2, file + 1));

	if (bounds_check(rank + 2, file - 1) && !piece_at(rank + 2, file - 1).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank + 2, file - 1));
	if (bounds_check(rank + 2, file + 1) && !piece_at(rank + 2, file + 1).is_color(to_move))
		child_boards.push_back(Board(board, rank, file, rank + 2, file + 1));
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
			if (bounds_check(rank + rank_d, file + file_d) && piece_at(rank + rank_d, file + file_d).is_opposing_color(to_move))
			{
				child_boards.push_back(Board(*this, rank, file, rank + rank_d, file + file_d));
			}
		}
	}
}
