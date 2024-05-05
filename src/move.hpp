#pragma once

#include <vector>

#include "position.hpp"
#include "capture.hpp"

namespace chess
{
	template<typename board_t>
	void make_move(position& child, const position& parent, const board_t& parent_board)
	{
		// copy the parent position
		child = parent;

		const rank start_rank = parent_board.start_rank();
		const file start_file = parent_board.start_file();
		const rank end_rank = parent_board.end_rank();
		const file end_file = parent_board.end_file();

		const size_t start_idx = to_index(start_rank, start_file);
		const size_t end_idx = to_index(end_rank, end_file);

		// check for en passant captures
		if (parent.piece_at(start_idx).is_pawn() &&
			start_file != end_file && // if a pawn captures...
			parent.piece_at(end_rank, end_file).is_empty()) // ...into an empty square, it must be an en passant capture
		{
			child.piece_at(start_rank, end_file) = empty; // the captured pawn will have the moving pawn's start rank and end file
		}
		// if a king is moving...
		else if (parent.piece_at(start_idx).is_king())
		{
			// ...and it is castling
			if (abs(file{ start_file - end_file }.value()) > 1)
			{
				if (start_file < end_file) // kingside castle
				{
					child.piece_at(start_rank, 5) = parent.piece_at(start_rank, 7);
					child.piece_at(start_rank, 7) = empty;
				}
				else // queenside castle
				{
					child.piece_at(start_rank, 3) = parent.piece_at(start_rank, 0);
					child.piece_at(start_rank, 0) = empty;
				}
			}
		}

		// handle the move, using parent_board.moved_piece to handle promotion at the same time
		const piece moved_piece = parent_board.moved_piece();
		if (!moved_piece.is_empty())
		{
			child.piece_at(end_idx) = moved_piece;
		}
		else
		{
			child.piece_at(end_idx) = parent.piece_at(start_idx);
		}

		child.piece_at(start_idx) = empty;
	}

	template<typename boards_t, typename board_t>
	void find_pawn_moves(boards_t& child_boards, const board_t& board, const position& position, const rank rank, const file file)
	{
		if (position.piece_at(rank, file).is_white())
		{
			if (bounds_check(rank - 1)) // only validate moving forwards once
			{
				// check for moving forward one square
				if (position.piece_at(rank - 1, file).is_empty())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(board, position, rank, file, rank - 1, file, white_queen);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file, white_rook);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file, white_bishop);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file, white_knight);
					}
					else // the pawn is moving without promotion
					{
						child_boards.emplace_back(board, position, rank, file, rank - 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 6 && position.piece_at(5, file).is_empty() && position.piece_at(4, file).is_empty())
				{
					child_boards.emplace_back(board, position, rank, file, 4, file);
				}
				// check for captures
				if (bounds_check(file + 1) && position.piece_at(rank - 1, file + 1).is_occupied() && position.piece_at(rank - 1, file + 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(board, position, rank, file, rank - 1, file + 1, white_queen);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file + 1, white_rook);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file + 1, white_bishop);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file + 1, white_knight);
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(board, position, rank, file, rank - 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && position.piece_at(rank - 1, file - 1).is_occupied() && position.piece_at(rank - 1, file - 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(board, position, rank, file, rank - 1, file - 1, white_queen);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file - 1, white_rook);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file - 1, white_bishop);
						child_boards.emplace_back(board, position, rank, file, rank - 1, file - 1, white_knight);
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(board, position, rank, file, rank - 1, file - 1);
					}
				}
				// check for en passant
				if (rank == 3)
				{
					if (board.en_passant_file() == file - 1 && bounds_check(file - 1) && position.piece_at(rank, file - 1).is_pawn())
						child_boards.emplace_back(board, position, rank, file, rank - 1, file - 1);
					else if (board.en_passant_file() == file + 1 && bounds_check(file + 1) && position.piece_at(rank, file + 1).is_pawn())
						child_boards.emplace_back(board, position, rank, file, rank - 1, file + 1);
				}
			}
		}
		else if (position.piece_at(rank, file).is_black())
		{
			if (bounds_check(rank + 1)) // only validate moving forwards once
			{
				// check for moving forward one square
				if (position.piece_at(rank + 1, file).is_empty())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(board, position, rank, file, rank + 1, file, black_queen);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file, black_rook);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file, black_bishop);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file, black_knight);
					}
					else // the pawn is moving without promotion
					{
						child_boards.emplace_back(board, position, rank, file, rank + 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 1 && position.piece_at(2, file).is_empty() && position.piece_at(3, file).is_empty())
				{
					child_boards.emplace_back(board, position, rank, file, 3, file);
				}
				// check for captures
				if (bounds_check(file + 1) && position.piece_at(rank + 1, file + 1).is_occupied() && position.piece_at(rank + 1, file + 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(board, position, rank, file, rank + 1, file + 1, black_queen);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file + 1, black_rook);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file + 1, black_bishop);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file + 1, black_knight);
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(board, position, rank, file, rank + 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && position.piece_at(rank + 1, file - 1).is_occupied() && position.piece_at(rank + 1, file - 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						child_boards.emplace_back(board, position, rank, file, rank + 1, file - 1, black_queen);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file - 1, black_rook);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file - 1, black_bishop);
						child_boards.emplace_back(board, position, rank, file, rank + 1, file - 1, black_knight);
					}
					else // the pawn is capturing without promotion
					{
						child_boards.emplace_back(board, position, rank, file, rank + 1, file - 1);
					}
				}
				// check for en passant
				if (rank == 4)
				{
					if (board.en_passant_file() == file - 1 && bounds_check(file - 1) && position.piece_at(rank, file - 1).is_pawn())
						child_boards.emplace_back(board, position, rank, file, rank + 1, file - 1);
					else if (board.en_passant_file() == file + 1 && bounds_check(file + 1) && position.piece_at(rank, file + 1).is_pawn())
						child_boards.emplace_back(board, position, rank, file, rank + 1, file + 1);
				}
			}
		}
	}
	template<typename boards_t, typename board_t>
	void find_rook_moves(boards_t& child_boards, const board_t& board, const position& position, const rank rank, const file file)
	{
		constexpr color_t color_to_move = board_t::color_to_move();

		// rank descending
		for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
		{
			if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

			if (position.piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
			{
				child_boards.emplace_back(board, position, rank, file, end_rank, file);
				continue; // keep searching in the current direction
			}
			// if the rook has encountered an enemy piece
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				// the rook can capture...
				child_boards.emplace_back(board, position, rank, file, end_rank, file);
				break; // ...but the rook cannot keep moving
			}
			else break; // the rook cannot move into a friendly piece; stop searching this way
		}

		// rank ascending (same logic as above)
		for (chess::rank end_rank = rank + 1; end_rank < 8; ++end_rank)
		{
			if (!bounds_check(end_rank)) break;

			if (position.piece_at(end_rank, file).is_empty())
			{
				child_boards.emplace_back(board, position, rank, file, end_rank, file);
				continue;
			}
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, end_rank, file);
				break;
			}
			else break;
		}

		// file descending (same logic as above)
		for (chess::file end_file = file - 1; end_file >= 0; --end_file)
		{
			if (!bounds_check(end_file)) break;

			if (position.piece_at(rank, end_file).is_empty())
			{
				child_boards.emplace_back(board, position, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, rank, end_file);
				break;
			}
			else break;
		}

		// file ascending (same logic as above)
		for (chess::file end_file = file + 1; end_file < 8; ++end_file)
		{
			if (!bounds_check(end_file)) break;

			if (position.piece_at(rank, end_file).is_empty())
			{
				child_boards.emplace_back(board, position, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, rank, end_file);
				break;
			}
			else break;
		}
	}
	template<typename boards_t, typename board_t>
	void find_bishop_moves(boards_t& child_boards, const board_t& board, const position& position, const rank rank, const file file)
	{
		constexpr color_t color_to_move = board_t::color_to_move();

		// working diagonally (rank and file descending)
		for (int offset = 1; offset < 8; ++offset)
		{
			// if the location is off of the board, stop searching in this direction
			if (!bounds_check(rank - offset, file - offset)) break;

			// if the square is empty
			if (position.piece_at(rank - offset, file - offset).is_empty())
			{
				// the bishop can move here
				child_boards.emplace_back(board, position, rank, file, rank - offset, file - offset);
				continue; // keep searching in this direction
			}
			// if the square is occupied by an enemy piece, the bishop can capture it
			else if (position.piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, rank - offset, file - offset);
				// the bishop made a capture, stop searching in this direction
				break;
			}
			// else, the square is occupied by a friendly piece, stop searching in this direction
			else break;
		}

		// working diagonally (rank descending and file ascending) (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank - offset, file + offset)) break;

			if (position.piece_at(rank - offset, file + offset).is_empty())
			{
				child_boards.emplace_back(board, position, rank, file, rank - offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, rank - offset, file + offset);
				break;
			}
			else break;
		}

		// working diagonally (rank ascending and file descending) (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank + offset, file - offset)) break;

			if (position.piece_at(rank + offset, file - offset).is_empty())
			{
				child_boards.emplace_back(board, position, rank, file, rank + offset, file - offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, rank + offset, file - offset);
				break;
			}
			else break;
		}

		// working diagonally (rank and file ascending) (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank + offset, file + offset)) break;

			if (position.piece_at(rank + offset, file + offset).is_empty())
			{
				child_boards.emplace_back(board, position, rank, file, rank + offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
			{
				child_boards.emplace_back(board, position, rank, file, rank + offset, file + offset);
				break;
			}
			else break;
		}
	}
	template<typename boards_t, typename board_t>
	void find_knight_moves(boards_t& child_boards, const board_t& board, const position& position, const rank rank, const file file)
	{
		constexpr color_t color_to_move = board_t::color_to_move();

		if (bounds_check(rank - 2, file + 1) && !(position.piece_at(rank - 2, file + 1).is_occupied() && position.piece_at(rank - 2, file + 1).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank - 2, file + 1);
		if (bounds_check(rank - 1, file + 2) && !(position.piece_at(rank - 1, file + 2).is_occupied() && position.piece_at(rank - 1, file + 2).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank - 1, file + 2);

		if (bounds_check(rank + 1, file + 2) && !(position.piece_at(rank + 1, file + 2).is_occupied() && position.piece_at(rank + 1, file + 2).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank + 1, file + 2);
		if (bounds_check(rank + 2, file + 1) && !(position.piece_at(rank + 2, file + 1).is_occupied() && position.piece_at(rank + 2, file + 1).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank + 2, file + 1);

		if (bounds_check(rank + 2, file - 1) && !(position.piece_at(rank + 2, file - 1).is_occupied() && position.piece_at(rank + 2, file - 1).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank + 2, file - 1);
		if (bounds_check(rank + 1, file - 2) && !(position.piece_at(rank + 1, file - 2).is_occupied() && position.piece_at(rank + 1, file - 2).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank + 1, file - 2);

		if (bounds_check(rank - 1, file - 2) && !(position.piece_at(rank - 1, file - 2).is_occupied() && position.piece_at(rank - 1, file - 2).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank - 1, file - 2);
		if (bounds_check(rank - 2, file - 1) && !(position.piece_at(rank - 2, file - 1).is_occupied() && position.piece_at(rank - 2, file - 1).is_color(color_to_move)))
			child_boards.emplace_back(board, position, rank, file, rank - 2, file - 1);
	}
	template<typename boards_t, typename board_t>
	void find_queen_moves(boards_t& child_boards, const board_t& board, const position& position, const rank rank, const file file)
	{
		find_rook_moves(child_boards, board, position, rank, file);
		find_bishop_moves(child_boards, board, position, rank, file);
	}
	template<typename boards_t, typename board_t>
	void find_king_moves(boards_t& child_boards, const board_t& board, const position& position, const rank rank, const file file)
	{
		constexpr color_t color_to_move = board_t::color_to_move();

		// iterate over all adjacent squares
		for (int rank_d = -1; rank_d <= 1; ++rank_d)
		{
			for (int file_d = -1; file_d <= 1; ++file_d)
			{
				// if the square is not occupied by a friendly piece
				if (bounds_check(rank + rank_d, file + file_d) &&
					!(position.piece_at(rank + rank_d, file + file_d).is_occupied() &&
					  position.piece_at(rank + rank_d, file + file_d).is_color(color_to_move)))
				{
					child_boards.emplace_back(board, position, rank, file, rank + rank_d, file + file_d);
				}
			}
		}

		const piece& king = position.piece_at(rank, file);

		using other_board_t = board_t::other_board_t;

		if ((king.is_white() && board.white_can_castle_ks()) ||
			(king.is_black() && board.black_can_castle_ks()))
		{
			if (// If white can castle to kingside, then we already know the king and rook are in place.
				// Check if the squares in between are empty.
				position.piece_at(rank, file + 1).is_empty() && // Check if the squares in between are empty.
				position.piece_at(rank, file + 2).is_empty() &&
				!is_king_in_check(board, position, king, rank, file)) // check if the king is in check now...
			{
				chess::position temp{};

				const other_board_t on_the_way_board{ board, position, rank, file, rank, file + 1 }; // ...on his way...
				make_move(temp, position, on_the_way_board);
				if (!is_king_in_check(on_the_way_board, temp, king, rank, file + 1))
				{
					const other_board_t destination_board{ board, position, rank, file, rank, file + 2 }; // ...or at his destination.
					make_move(temp, position, destination_board);
					if (!is_king_in_check(destination_board, temp, king, rank, file + 2))
					{
						child_boards.emplace_back(board, position, rank, file, rank, file + 2); // the board constructor will detect a castle and move the rook as well
					}
				}
			}
		}

		if ((king.is_white() && board.white_can_castle_qs()) || // (same logic as above)
			(king.is_black() && board.black_can_castle_qs()))
		{
			if (position.piece_at(rank, file - 1).is_empty() &&
				position.piece_at(rank, file - 2).is_empty() &&
				position.piece_at(rank, file - 3).is_empty() && // we need to check that this square is empty for the rook to move through, but no check test is needed
				!is_king_in_check(board, position, king, rank, file))
			{
				chess::position temp{};

				const other_board_t on_the_way_board{ board, position, rank, file, rank, file - 1 };
				make_move(temp, position, on_the_way_board);
				if (!is_king_in_check(on_the_way_board, temp, king, rank, file - 1))
				{
					const other_board_t destination_board{ board, position, rank, file, rank, file - 2 };
					make_move(temp, position, destination_board);
					if (!is_king_in_check(destination_board, temp, king, rank, file - 2))
					{
						child_boards.emplace_back(board, position, rank, file, rank, file - 2);
					}
				}
			}
		}
	}

	template<color_t attacking_pawn_color>
	inline bool square_is_attacked_by_pawn(const position& position, const rank rank, const file file)
	{
		if constexpr (attacking_pawn_color == white)
		{
			if ((bounds_check(rank + 1, file + 1) && position.piece_at(rank + 1, file + 1).is(white_pawn)) ||
				(bounds_check(rank + 1, file - 1) && position.piece_at(rank + 1, file - 1).is(white_pawn))) return true;
		}
		else
		{
			if ((bounds_check(rank - 1, file + 1) && position.piece_at(rank - 1, file + 1).is(black_pawn)) ||
				(bounds_check(rank - 1, file - 1) && position.piece_at(rank - 1, file - 1).is(black_pawn))) return true;
		}

		return false;
	}
	inline bool square_is_attacked_by_knight(const position& position, const piece attacking_knight, const rank rank, const file file)
	{
		return knight_attacks[to_index(rank, file)](position, attacking_knight);
	}
	inline bool square_is_attacked_by_bishop_or_queen(const position& position, const color_t attacker_color, const rank rank, const file file)
	{
		// iterate in all four diagonal directions to find a bishop or queen

		// search rank and file descending
		for (int offset = 1; offset < 8; ++offset)
		{
			// if the coordinates are in bounds
			if (!bounds_check(rank - offset, file - offset)) break;

			// if there is a piece here
			if (position.piece_at(rank - offset, file - offset).is_occupied())
			{
				// if the piece is a bishop/queen of the opposing color, the king is in check
				if ((position.piece_at(rank - offset, file - offset).is_bishop() || position.piece_at(rank - offset, file - offset).is_queen())
					&& position.piece_at(rank - offset, file - offset).is_color(attacker_color)) return true;
				break; // a piece is here, don't keep searching in this direction
			}
		}

		// search rank descending and file ascending (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank - offset, file + offset)) break;

			if (position.piece_at(rank - offset, file + offset).is_occupied())
			{
				if ((position.piece_at(rank - offset, file + offset).is_bishop() || position.piece_at(rank - offset, file + offset).is_queen())
					&& position.piece_at(rank - offset, file + offset).is_color(attacker_color)) return true;
				break;
			}
		}

		// search rank ascending and file descending (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank + offset, file - offset)) break;

			if (position.piece_at(rank + offset, file - offset).is_occupied())
			{
				if ((position.piece_at(rank + offset, file - offset).is_bishop() || position.piece_at(rank + offset, file - offset).is_queen())
					&& position.piece_at(rank + offset, file - offset).is_color(attacker_color)) return true;
				break;
			}
		}

		// search rank and file ascending (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank + offset, file + offset)) break;

			if (position.piece_at(rank + offset, file + offset).is_occupied())
			{
				if ((position.piece_at(rank + offset, file + offset).is_bishop() || position.piece_at(rank + offset, file + offset).is_queen())
					&& position.piece_at(rank + offset, file + offset).is_color(attacker_color)) return true;
				break;
			}
		}

		return false;
	}
	inline bool square_is_attacked_by_rook_or_queen(const position& position, const color_t attacker_color, const rank rank, const file file)
	{
		// iterate in all four vertical and horizontal directions to check for a rook or queen (these loops only look within bounds)

		// rank descending
		for (auto other_rank = rank - 1; other_rank >= 0; --other_rank)
		{
			// if a square is found that is not empty
			if (position.piece_at(other_rank, file).is_occupied())
			{
				// if the piece is a rook/queen AND is hostile, the king is in check
				if ((position.piece_at(other_rank, file).is_rook() || position.piece_at(other_rank, file).is_queen())
					&& position.piece_at(other_rank, file).is_color(attacker_color)) return true;
				break; // a piece was found in this direction, stop checking in this direction
			}
		}
		// rank ascending (same logic as above)
		for (auto other_rank = rank + 1; other_rank < 8; ++other_rank)
		{
			if (position.piece_at(other_rank, file).is_occupied())
			{
				if ((position.piece_at(other_rank, file).is_rook() || position.piece_at(other_rank, file).is_queen())
					&& position.piece_at(other_rank, file).is_color(attacker_color)) return true;
				break;
			}
		}
		// file descending (same logic as above)
		for (auto other_file = file - 1; other_file >= 0; --other_file)
		{
			if (position.piece_at(rank, other_file).is_occupied())
			{
				if ((position.piece_at(rank, other_file).is_rook() || position.piece_at(rank, other_file).is_queen())
					&& position.piece_at(rank, other_file).is_color(attacker_color)) return true;
				break;
			}
		}
		// file ascending (same logic as above)
		for (auto other_file = file + 1; other_file < 8; ++other_file)
		{
			if (position.piece_at(rank, other_file).is_occupied())
			{
				if ((position.piece_at(rank, other_file).is_rook() || position.piece_at(rank, other_file).is_queen())
					&& position.piece_at(rank, other_file).is_color(attacker_color)) return true;
				break;
			}
		}

		return false;
	}
	inline bool square_is_attacked_by_king(const position& position, const piece attacking_king, const rank rank, const file file)
	{
		// Check adjacent squares for a king.
		// Check rank first, because a king is likely on a top or bottom rank.
		if (bounds_check(rank - 1))
		{
			if (bounds_check(file - 1) && position.piece_at(rank - 1, file - 1).is(attacking_king)) return true;
			if (position.piece_at(rank - 1, file).is(attacking_king)) return true;
			if (bounds_check(file + 1) && position.piece_at(rank - 1, file + 1).is(attacking_king)) return true;
		}

		if (bounds_check(file - 1) && position.piece_at(rank, file - 1).is(attacking_king)) return true;
		if (bounds_check(file + 1) && position.piece_at(rank, file + 1).is(attacking_king)) return true;

		if (bounds_check(rank + 1))
		{
			if (bounds_check(file - 1) && position.piece_at(rank + 1, file - 1).is(attacking_king)) return true;
			if (position.piece_at(rank + 1, file).is(attacking_king)) return true;
			if (bounds_check(file + 1) && position.piece_at(rank + 1, file + 1).is(attacking_king)) return true;
		}

		return false;
	}

	template<bool do_pawn_checks, bool do_knight_checks, bool do_king_checks>
	bool is_king_in_check(const position& position, const piece king, const rank rank, const file file)
	{
		if constexpr (do_king_checks)
		{
			if (square_is_attacked_by_king(position, detail::king | other(king.get_color()), rank, file)) return true;
		}

		if (square_is_attacked_by_rook_or_queen(position, other(king.get_color()), rank, file)) return true;

		if (square_is_attacked_by_bishop_or_queen(position, other(king.get_color()), rank, file)) return true;

		if constexpr (do_knight_checks)
		{
			if (square_is_attacked_by_knight(position, detail::knight | other(king.get_color()), rank, file)) return true;
		}

		if constexpr (do_pawn_checks)
		{
			if (king.is_white())
			{
				if (square_is_attacked_by_pawn<black>(position, rank, file)) return true;
			}
			else
			{
				if (square_is_attacked_by_pawn<white>(position, rank, file)) return true;
			}
		}

		return false;
	}

	template<typename board_t>
	bool is_king_in_check(const board_t& board, const position& position, const piece king, const rank rank, const file file)
	{
		// If the last move is known, and a player is starting their turn, some piece checks can be skipped.
		if (board.has_move() && king.is_color(board_t::color_to_move()))
		{
			const piece last_moved = board.last_moved_piece(position);

			// Only look for pawn/knight/king checks if the last moved piece is a pawn/knight/king.
			if (last_moved.is_pawn())
				return is_king_in_check<true, false, false>(position, king, rank, file);
			else if (last_moved.is_knight())
				return is_king_in_check<false, true, false>(position, king, rank, file);
			else if (last_moved.is_king())
				return is_king_in_check<false, false, true>(position, king, rank, file);
		}

		// We don't know what move created this position, or it is not the king's color's turn to move; do all checks.
		return is_king_in_check<true, true, true>(position, king, rank, file);
	}

	template<typename board_t>
	bool is_king_in_check(const board_t& board, const position& position, const color_t check_color)
	{
		static_assert(sizeof(piece) == 1);

		// load the 64 bytes of the board into two ymm registers
		uint256_t ymm0 = _mm256_loadu_si256((uint256_t*)position._position.data() + 0);
		uint256_t ymm1 = _mm256_loadu_si256((uint256_t*)(position._position.data() + 32));

		// broadcast the target piece (king | color) to all positions of a vector
		const uint256_t target_mask = _mm256_set1_epi8(detail::king | check_color);

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
			return is_king_in_check(board, position, position.piece_at(index), index / 8, index % 8);
		}

		return false;
	}

	template<typename board_t>
	bool is_valid_position(const board_t& board, const position& position)
	{
		// It is fine if the player to move was placed into check.
		// The color that just moved must not be in check.
		return !is_king_in_check(board, position, board_t::other_color());
	}

	template<typename board_t>
	auto& generate_child_boards(board_t& board, const position& position)
	{
		static std::vector<typename board_t::other_board_t> child_boards;

		constexpr color_t color_to_move = board_t::color_to_move();

		child_boards.clear();

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				// if this piece can move
				if (position.piece_at(rank, file).is_occupied() &&
					position.piece_at(rank, file).is_color(color_to_move))
				{
					if (position.piece_at(rank, file).is_pawn())
					{
						find_pawn_moves(child_boards, board, position, rank, file);
					}
					else if (position.piece_at(rank, file).is_rook())
					{
						find_rook_moves(child_boards, board, position, rank, file);
					}
					else if (position.piece_at(rank, file).is_bishop())
					{
						find_bishop_moves(child_boards, board, position, rank, file);
					}
					else if (position.piece_at(rank, file).is_knight())
					{
						find_knight_moves(child_boards, board, position, rank, file);
					}
					else if (position.piece_at(rank, file).is_queen())
					{
						find_queen_moves(child_boards, board, position, rank, file);
					}
					else if (position.piece_at(rank, file).is_king())
					{
						find_king_moves(child_boards, board, position, rank, file);
					}
				} // end if piece can move
			}
		}

		remove_invalid_boards(child_boards, position);

		// If there are no legal moves, record the result
		if (child_boards.size() == 0)
		{
			if (is_king_in_check(board, position, color_to_move))
				board.set_result((color_to_move == white) ? result::black_wins_by_checkmate : result::white_wins_by_checkmate);
			else
				board.set_result(result::draw_by_stalemate);
		}

		return child_boards;
	}

	template<typename board_list>
	void remove_invalid_boards(board_list& boards, const position& parent_position)
	{
		static board_list valid_boards;

		position child_position{};

		for (auto& child_board : boards)
		{
			// re-create the child position so we can use it to validate the child board
			make_move(child_position, parent_position, child_board);

			if (is_valid_position(child_board, child_position))
			{
				valid_boards.push_back(child_board);
			}
		}

		boards.swap(valid_boards);
		valid_boards.clear();
	}
}
