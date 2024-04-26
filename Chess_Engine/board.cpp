
/* Jim Viebke
Mar 14 2016 */

#include <iostream>

#include "board.hpp"
#include "capture.hpp"

namespace chess
{
	// Simple, nonvalidating FEN parser
	Board::Board(const std::string& fen)
	{
		auto position_it = _position.begin();
		auto fen_it = fen.cbegin();

		// Edited from Wikipedia:

		// A FEN record contains six fields. The separator between fields is a space. The fields are:

		// 1. Piece placement (from white's perspective). Each rank is described, starting with rank 8 and ending with rank 1; within each rank, the contents of each square are described from file "a" through file "h". Following the Standard Algebraic Notation (SAN), each piece is identified by a single letter taken from the standard English names (pawn = "P", knight = "N", bishop = "B", rook = "R", queen = "Q" and king = "K"). White pieces are designated using upper-case letters ("PNBRQK") while black pieces use lowercase ("pnbrqk"). Empty squares are noted using digits 1 through 8 (the number of empty squares), and "/" separates ranks.

		for (char c = *fen_it++; c != ' '; c = *fen_it++)
		{
			switch (c)
			{
				case 'P': *position_it++ = white_pawn; break;
				case 'N': *position_it++ = white_knight; break;
				case 'B': *position_it++ = white_bishop; break;
				case 'R': *position_it++ = white_rook; break;
				case 'Q': *position_it++ = white_queen; break;
				case 'K': *position_it++ = white_king; break;
				case 'p': *position_it++ = black_pawn; break;
				case 'n': *position_it++ = black_knight; break;
				case 'b': *position_it++ = black_bishop; break;
				case 'r': *position_it++ = black_rook; break;
				case 'q': *position_it++ = black_queen; break;
				case 'k': *position_it++ = black_king; break;

				default:
				{
					if (c >= '1' && c <= '8')
					{
						position_it += (size_t(c) - '0'); // advance N squares
					}
				}
			}
		}

		// 2. Active color. "w" means White moves next, "b" means Black.

		color_to_move = (*fen_it == 'w') ? white : black;
		fen_it += 2; // step past color and space

		// 3. Castling availability. If neither side can castle, this is "-". Otherwise, this has one or more letters: "K" (White can castle kingside), "Q" (White can castle queenside), "k" (Black can castle kingside), and/or "q" (Black can castle queenside).

		white_can_castle_k_s = false;
		white_can_castle_q_s = false;
		black_can_castle_k_s = false;
		black_can_castle_q_s = false;
		for (char c = *fen_it++; c != ' '; c = *fen_it++)
		{
			switch (c)
			{
				case 'K': white_can_castle_k_s = true; break;
				case 'k': white_can_castle_q_s = true; break;
				case 'Q': black_can_castle_k_s = true; break;
				case 'q': black_can_castle_q_s = true; break;
				default: break;
			}
		}

		// 4. En passant target square in algebraic notation. If there's no en passant target square, this is "-". If a pawn has just made a two-square move, this is the position "behind" the pawn. This is recorded regardless of whether there is a pawn in position to make an en passant capture.

		if (*fen_it != '-')
		{
			en_passant_flag = *fen_it++ - 'a'; // convert letter file a-h to index 0-7
			++fen_it; // step past the rank
		}
		++fen_it; // step past the space

		// 5. Halfmove clock. This is the number of halfmoves since the last capture or pawn advance. This is used to determine if a draw can be claimed under the fifty-move rule.

		const auto halfmove_clock_end = std::find(fen_it, fen.cend(), ' ');
		std::from_chars(fen_it._Ptr, halfmove_clock_end._Ptr, fifty_move_rule);

		// 6. Fullmove number. The number of the full move. It starts at 1, and is incremented after Black's move.

		// (ignored)
	}

	void Board::print_board(const Board& board, const unsigned& offset)
	{
		for (rank rank = 0; rank < 8; ++rank)
		{
			// add indent
			for (unsigned i = 0; i < offset * 9; ++i) std::cout << ' ';

			for (file file = 0; file < 8; ++file)
			{
				const piece& piece = board.piece_at(rank, file);

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
	void Board::print_board(const board_list& boards)
	{
		for (rank rank = 0; rank < 8; ++rank)
		{
			// render this rank for each board
			for (board_list::const_iterator it = boards.cbegin(); it != boards.cend(); ++it)
			{
				for (file file = 0; file < 8; ++file)
				{
					const piece& piece = it->piece_at(rank, file);

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

	Board::board_list& Board::generate_child_boards()
	{
		static board_list child_boards;

		child_boards.clear();

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				// if this piece can move
				if (piece_at(rank, file).is_occupied() && piece_at(rank, file).is_color(color_to_move))
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

		if (child_boards.size() == 0)
		{
			if (white_to_move() && is_king_in_check(white))
			{
				_result = result::black_wins_by_checkmate;
			}
			else if (black_to_move() && is_king_in_check(black))
			{
				_result = result::white_wins_by_checkmate;
			}
			else
			{
				_result = result::draw_by_stalemate;
			}
		}

		return child_boards;
	}

	void Board::remove_invalid_boards(board_list& boards)
	{
		static board_list valid_boards;

		for (auto& board : boards)
		{
			if (board.is_valid_position())
			{
				valid_boards.push_back(board);
			}
		}

		boards.swap(valid_boards);
		valid_boards.clear();
	}

	bool Board::is_valid_position() const
	{
		// It is fine if the moving player was placed into check.
		// The color that just moved must not be in check.
		return !is_king_in_check(other_color(color_to_move));
	}

	// these are in the same order that they are called in the generation function, which acts in order of probable piece frequency

	void Board::find_pawn_moves(board_list& child_boards, const rank rank, const file file) const
	{
		if (piece_at(rank, file).is_white())
		{
			if (bounds_check(rank - 1)) // only validate moving forwards once
			{
				// check for moving forward one square
				if (piece_at(rank - 1, file).is_empty())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(*this, rank, file, rank - 1, file, piece(piece::white | piece::queen));
						child_boards.emplace_back(*this, rank, file, rank - 1, file, piece(piece::white | piece::rook));
						child_boards.emplace_back(*this, rank, file, rank - 1, file, piece(piece::white | piece::bishop));
						child_boards.emplace_back(*this, rank, file, rank - 1, file, piece(piece::white | piece::knight));
					}
					else // the pawn is moving without promotion
					{
						child_boards.emplace_back(*this, rank, file, rank - 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 6 && piece_at(5, file).is_empty() && piece_at(4, file).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, 4, file);
				}
				// check for captures
				if (bounds_check(file + 1) && piece_at(rank - 1, file + 1).is_occupied() && piece_at(rank - 1, file + 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, piece(piece::white | piece::queen));
						child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, piece(piece::white | piece::rook));
						child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, piece(piece::white | piece::bishop));
						child_boards.emplace_back(*this, rank, file, rank - 1, file + 1, piece(piece::white | piece::knight));
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(*this, rank, file, rank - 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && piece_at(rank - 1, file - 1).is_occupied() && piece_at(rank - 1, file - 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, piece(piece::white | piece::queen));
						child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, piece(piece::white | piece::rook));
						child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, piece(piece::white | piece::bishop));
						child_boards.emplace_back(*this, rank, file, rank - 1, file - 1, piece(piece::white | piece::knight));
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(*this, rank, file, rank - 1, file - 1);
					}
				}
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
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(*this, rank, file, rank + 1, file, piece(piece::black | piece::queen));
						child_boards.emplace_back(*this, rank, file, rank + 1, file, piece(piece::black | piece::rook));
						child_boards.emplace_back(*this, rank, file, rank + 1, file, piece(piece::black | piece::bishop));
						child_boards.emplace_back(*this, rank, file, rank + 1, file, piece(piece::black | piece::knight));
					}
					else // the pawn is moving without promotion
					{
						child_boards.emplace_back(*this, rank, file, rank + 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 1 && piece_at(2, file).is_empty() && piece_at(3, file).is_empty())
				{
					child_boards.emplace_back(*this, rank, file, 3, file);
				}
				// check for captures
				if (bounds_check(file + 1) && piece_at(rank + 1, file + 1).is_occupied() && piece_at(rank + 1, file + 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, piece(piece::black | piece::queen));
						child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, piece(piece::black | piece::rook));
						child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, piece(piece::black | piece::bishop));
						child_boards.emplace_back(*this, rank, file, rank + 1, file + 1, piece(piece::black | piece::knight));
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(*this, rank, file, rank + 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && piece_at(rank + 1, file - 1).is_occupied() && piece_at(rank + 1, file - 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, piece(piece::black | piece::queen));
						child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, piece(piece::black | piece::rook));
						child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, piece(piece::black | piece::bishop));
						child_boards.emplace_back(*this, rank, file, rank + 1, file - 1, piece(piece::black | piece::knight));
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(*this, rank, file, rank + 1, file - 1);
					}
				}
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
	void Board::find_rook_moves(board_list& child_boards, const rank rank, const file file) const
	{
		// rank descending
		for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
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
		for (chess::rank end_rank = rank + 1; end_rank < 8; ++end_rank)
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
		for (chess::file end_file = file - 1; end_file >= 0; --end_file)
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
		for (chess::file end_file = file + 1; end_file < 8; ++end_file)
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
	void Board::find_bishop_moves(board_list& child_boards, const rank rank, const file file) const
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
	void Board::find_knight_moves(board_list& child_boards, const rank rank, const file file) const
	{
		if (bounds_check(rank - 2, file + 1) && !(piece_at(rank - 2, file + 1).is_occupied() && piece_at(rank - 2, file + 1).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank - 2, file + 1);
		if (bounds_check(rank - 1, file + 2) && !(piece_at(rank - 1, file + 2).is_occupied() && piece_at(rank - 1, file + 2).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank - 1, file + 2);

		if (bounds_check(rank + 1, file + 2) && !(piece_at(rank + 1, file + 2).is_occupied() && piece_at(rank + 1, file + 2).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank + 1, file + 2);
		if (bounds_check(rank + 2, file + 1) && !(piece_at(rank + 2, file + 1).is_occupied() && piece_at(rank + 2, file + 1).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank + 2, file + 1);

		if (bounds_check(rank + 2, file - 1) && !(piece_at(rank + 2, file - 1).is_occupied() && piece_at(rank + 2, file - 1).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank + 2, file - 1);
		if (bounds_check(rank + 1, file - 2) && !(piece_at(rank + 1, file - 2).is_occupied() && piece_at(rank + 1, file - 2).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank + 1, file - 2);

		if (bounds_check(rank - 1, file - 2) && !(piece_at(rank - 1, file - 2).is_occupied() && piece_at(rank - 1, file - 2).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank - 1, file - 2);
		if (bounds_check(rank - 2, file - 1) && !(piece_at(rank - 2, file - 1).is_occupied() && piece_at(rank - 2, file - 1).is_color(color_to_move)))
			child_boards.emplace_back(*this, rank, file, rank - 2, file - 1);
	}
	void Board::find_queen_moves(board_list& child_boards, const rank rank, const file file) const
	{
		find_rook_moves(child_boards, rank, file);
		find_bishop_moves(child_boards, rank, file);
	}
	void Board::find_king_moves(board_list& child_boards, const rank rank, const file file) const
	{
		// iterate over all adjacent squares
		for (int rank_d = -1; rank_d <= 1; ++rank_d)
		{
			for (int file_d = -1; file_d <= 1; ++file_d)
			{
				// if the square is not occupied by a friendly piece
				if (bounds_check(rank + rank_d, file + file_d) &&
					!(piece_at(rank + rank_d, file + file_d).is_occupied() &&
					  piece_at(rank + rank_d, file + file_d).is_color(color_to_move)))
				{
					child_boards.emplace_back(*this, rank, file, rank + rank_d, file + file_d);
				}
			}
		}

		const piece& king = piece_at(rank, file);

		if ((king.is_white() && white_can_castle_k_s) ||
			(king.is_black() && black_can_castle_k_s))
		{
			if (// If white can castle to kingside, then we already know the king and rook are in place.
				// Check if the squares in between are empty. 
				piece_at(rank, file + 1).is_empty() && // Check if the squares in between are empty.
				piece_at(rank, file + 2).is_empty() &&
				!is_king_in_check(king, rank, file) && // check if the king is in check now...			
				!Board(*this, rank, file, rank, file + 1).is_king_in_check(king, rank, file + 1) && // ...on his way...
				!Board(*this, rank, file, rank, file + 2).is_king_in_check(king, rank, file + 2)) // ...or at his destination.
			{
				child_boards.emplace_back(*this, rank, file, rank, file + 2); // the board constructor will detect a castle and move the rook as well
			}
		}

		if ((king.is_white() && white_can_castle_q_s) || // documentation same as above
			(king.is_black() && black_can_castle_q_s))
		{
			if (piece_at(rank, file - 1).is_empty() &&
				piece_at(rank, file - 2).is_empty() &&
				piece_at(rank, file - 3).is_empty() && // we need to check that this square is empty for the rook to move through, but no check test is needed
				!is_king_in_check(king, rank, file) &&
				!Board(*this, rank, file, rank, file - 1).is_king_in_check(king, rank, file - 1) &&
				!Board(*this, rank, file, rank, file - 2).is_king_in_check(king, rank, file - 2))
			{
				child_boards.emplace_back(*this, rank, file, rank, file - 2);
			}
		}
	}

	bool Board::is_king_in_check(const color_t check_color) const
	{
		static_assert(sizeof(piece) == 1);

		// load the 64 bytes of the board into two ymm registers
		uint256_t ymm0 = _mm256_loadu_si256((uint256_t*)_position.data() + 0);
		uint256_t ymm1 = _mm256_loadu_si256((uint256_t*)(_position.data() + 32));

		// broadcast the target piece (king | color) to all positions of a vector
		const uint256_t target_mask = _mm256_set1_epi8(piece(piece::king | check_color).get_piece());

		// find the matching byte
		ymm0 = _mm256_cmpeq_epi8(ymm0, target_mask);
		ymm1 = _mm256_cmpeq_epi8(ymm1, target_mask);
		// extract 2x 32-bit bitmasks
		const uint64_t mask_low = _mm256_movemask_epi8(ymm0);
		const uint64_t mask_high = _mm256_movemask_epi8(ymm1);
		// merge 2x 32-bit bitmasks to 1x 64-bit bitmask
		const uint64_t mask = (mask_high << 32) | mask_low;

		// Scan for the position of the first set bit in the mask.
		// tzcnt returns the width of the type if a bit is not found.
		size_t index = _tzcnt_u64(mask);

		if (index < 64)
		{
			return is_king_in_check(_position[index], index / 8, index % 8);
		}

		return false;
	}

	bool Board::is_king_in_check(const piece king, const rank rank, const file file) const
	{
		bool do_pawn_checks = false;
		bool do_knight_checks = false;
		bool do_king_checks = false;

		// If the last move is known, and a player is starting their turn, some piece checks can be skipped.
		if (board_has_move() && king.is_color(color_to_move))
		{
			const piece last_moved = last_moved_piece();

			// Only look for pawn checks if the last moved piece is a pawn.
			if (last_moved.is_pawn())
				do_pawn_checks = true;
			// Only look for knight checks if the last moved piece is a knight.
			else if (last_moved.is_knight())
				do_knight_checks = true;
			// Only look for king checks if the last moved piece is a king.
			else if (last_moved.is_king())
				do_king_checks = true;
		}
		else
		{
			// We don't know what move created this position, or it is not the king's color's turn to move; do all checks.
			do_pawn_checks = true;
			do_knight_checks = true;
			do_king_checks = true;
		}

		if (do_king_checks)
		{
			// Check adjacent squares for a king.
			// Check rank first, because a king is likely on a top or bottom rank.
			if (bounds_check(rank - 1))
			{
				if (bounds_check(file - 1) && piece_at(rank - 1, file - 1).is_king()) return true;
				if (piece_at(rank - 1, file).is_king()) return true;
				if (bounds_check(file + 1) && piece_at(rank - 1, file + 1).is_king()) return true;
			}

			if (bounds_check(file - 1) && piece_at(rank, file - 1).is_king()) return true;
			if (bounds_check(file + 1) && piece_at(rank, file + 1).is_king()) return true;

			if (bounds_check(rank + 1))
			{
				if (bounds_check(file - 1) && piece_at(rank + 1, file - 1).is_king()) return true;
				if (piece_at(rank + 1, file).is_king()) return true;
				if (bounds_check(file + 1) && piece_at(rank + 1, file + 1).is_king()) return true;
			}
		}

		// iterate in all four vertical and horizontal directions to check for a rook or queen (these loops only look within bounds)

		// rank descending
		for (auto other_rank = rank - 1; other_rank >= 0; --other_rank)
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
		for (auto other_rank = rank + 1; other_rank < 8; ++other_rank)
		{
			if (piece_at(other_rank, file).is_occupied())
			{
				if ((piece_at(other_rank, file).is_rook() || piece_at(other_rank, file).is_queen())
					&& piece_at(other_rank, file).is_opposing_color(king)) return true;
				break;
			}
		}
		// file descending (documentation same as above)
		for (auto other_file = file - 1; other_file >= 0; --other_file)
		{
			if (piece_at(rank, other_file).is_occupied())
			{
				if ((piece_at(rank, other_file).is_rook() || piece_at(rank, other_file).is_queen())
					&& piece_at(rank, other_file).is_opposing_color(king)) return true;
				break;
			}
		}
		// file ascending (documentation same as above)
		for (auto other_file = file + 1; other_file < 8; ++other_file)
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

		if (do_knight_checks)
		{
			const piece_t opposing_knight = other_color(king.get_color()) | piece::knight;
			if (knight_attacks[to_index(rank, file)](_position, opposing_knight)) return true;
		}

		// check if the white king is under attack by a black pawn
		if (do_pawn_checks)
		{
			if (king.is_white())
			{
				if ((bounds_check(rank - 1, file + 1) && piece_at(rank - 1, file + 1).is(piece::black | piece::pawn)) ||
					(bounds_check(rank - 1, file - 1) && piece_at(rank - 1, file - 1).is(piece::black | piece::pawn))) return true;
			}
			// check if the black king is under attack by a white pawn
			else if (king.is_black())
			{
				if ((bounds_check(rank + 1, file + 1) && piece_at(rank + 1, file + 1).is(piece::white | piece::pawn)) ||
					(bounds_check(rank + 1, file - 1) && piece_at(rank + 1, file - 1).is(piece::white | piece::pawn))) return true;
			}
		}

		return false;
	}

}
