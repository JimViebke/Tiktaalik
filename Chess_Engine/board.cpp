
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
					// iterate over all adjacent squares
					for (int rank_d = -1; rank_d <= 1; ++rank_d)
					{
						for (int file_d = -1; file_d <= 1; ++file_d)
						{
							// if the square is not occupied by a friendly piece
							if (bounds_check(rank + rank_d, file + file_d) && !board[rank + rank_d][file + file_d].is_color(to_move))
							{
								child_boards.push_back(Board(board, rank, file, rank + rank_d, file + file_d));
							}
						}
					}
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
				// iterate over all adjacent squares
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
			} // end if is king
		}
	}

	return true;
}
