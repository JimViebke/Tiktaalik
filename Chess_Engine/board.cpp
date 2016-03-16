
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
			if (board[rank][file].is_color(to_move))
			{
				// if this piece is a king
				if (board[rank][file].is_king())
				{
					find_king_moves(child_boards, rank, file);
				}
				else if (board[rank][file].is_rook())
				{
					find_rook_moves(child_boards, rank, file);
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
			if (board[rank][file].is_king())
			{
				// iterate over all adjacent squares to check for a king
				for (int rank_d = -1; rank_d <= 1; ++rank_d)
				{
					for (int file_d = -1; file_d <= 1; ++file_d)
					{
						if (rank_d == 0 && file_d == 0) continue; // don't examine the current square
						if (bounds_check(rank + rank_d, file + file_d) && board[rank + rank_d][file + file_d].is_king())
						{
							return false;
						}
					}
				}

				// iterate in all four vertical and horizontal directions to check for a rook

				// rank descending
				for (int other_rank = rank - 1; other_rank >= 0; --other_rank)
				{
					// if a square is found that is not empty
					if (board[other_rank][file].is_occupied())
					{
						// if the piece is a rook AND is hostile AND it is the hostile piece's turn, then the position is invalid
						if (board[other_rank][file].is_rook() && board[other_rank][file].is_opposing_color(board[rank][file]) && board[other_rank][file].is_color(to_move)) return false;
						break; // a piece was found in this direction, stop checking in this direction
					}
				}
				// rank ascending (documentation same as above)
				for (int other_rank = rank + 1; other_rank < 8; ++other_rank)
				{
					if (board[other_rank][file].is_occupied())
					{
						if (board[other_rank][file].is_rook() && board[other_rank][file].is_opposing_color(board[rank][file]) && board[other_rank][file].is_color(to_move)) return false;
						break;
					}
				}
				// file descending (documentation same as above)
				for (int other_file = file - 1; other_file >= 0; --other_file)
				{
					if (board[rank][other_file].is_occupied())
					{
						if (board[rank][other_file].is_rook() && board[rank][other_file].is_opposing_color(board[rank][file]) && board[rank][other_file].is_color(to_move)) return false;
						break;
					}
				}
				// file ascending (documentation same as above)
				for (int other_file = file + 1; other_file < 8; ++other_file)
				{
					if (board[rank][other_file].is_occupied())
					{
						if (board[rank][other_file].is_rook() && board[rank][other_file].is_opposing_color(board[rank][file]) && board[rank][other_file].is_color(to_move)) return false;
						break;
					}
				}

			} // end if is king
		}
	}

	return true;
}

void Board::find_king_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	// iterate over all adjacent squares
	for (int rank_d = -1; rank_d <= 1; ++rank_d)
	{
		for (int file_d = -1; file_d <= 1; ++file_d)
		{
			// if the square is not occupied by a friendly piece
			if (bounds_check(rank + rank_d, file + file_d) && !board[rank + rank_d][file + file_d].is_color(to_move))
			{
				child_boards.push_back(Board(*this, rank, file, rank + rank_d, file + file_d));
			}
		}
	}
}
void Board::find_rook_moves(std::list<Board> & child_boards, const int rank, const int file) const
{
	// rank descending
	for (int end_rank = rank - 1; end_rank >= 0; --end_rank)
	{
		if (!bounds_check(end_rank, file)) break; // out of bounds; don't keep iterating in this direction

		if (board[end_rank][file].is_empty()) // if the square is empty, the rook can move here
		{
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
			continue; // keep searching in the current direction
		}
		// if the rook has encountered an enemy piece
		else if (!board[end_rank][file].is_color(to_move))
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
		if (!bounds_check(end_rank, file)) break;

		if (board[end_rank][file].is_empty())
		{
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
			continue;
		}
		else if (!board[end_rank][file].is_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, end_rank, file));
			break;
		}
		else break;
	}

	// file descending (documentation same as above)
	for (int end_file = file - 1; end_file >= 0; --end_file)
	{
		if (!bounds_check(rank, end_file)) break;

		if (board[rank][end_file].is_empty())
		{
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
			continue;
		}
		else if (!board[rank][end_file].is_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
			break;
		}
		else break;
	}

	// file ascending (documentation same as above)
	for (int end_file = file + 1; end_file < 8; ++end_file)
	{
		if (!bounds_check(rank, end_file)) break;

		if (board[rank][end_file].is_empty())
		{
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
			continue;
		}
		else if (!board[rank][end_file].is_color(to_move))
		{
			child_boards.push_back(Board(*this, rank, file, rank, end_file));
			break;
		}
		else break;
	}
}
