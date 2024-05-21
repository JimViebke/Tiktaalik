#pragma once

#include <vector>

#include "bitboard.hpp"
#include "board.hpp"
#include "capture.hpp"
#include "config.hpp"
#include "position.hpp"
#include "transposition_table.hpp"

namespace chess
{
	inline void make_move(position& child, const position& parent,
						  const rank start_rank, const file start_file, const rank end_rank, const file end_file)
	{
		child = parent;

		const size_t start_idx = to_index(start_rank, start_file);
		const size_t end_idx = to_index(end_rank, end_file);

		child.piece_at(end_idx) = parent.piece_at(start_idx);
		child.piece_at(start_idx) = empty;
	}

	template<color_t moving_color, piece_t moving_piece_type, move_type move_type>
	void make_move(position& child, const position& parent, const board& child_board)
	{
		// copy the parent position
		child = parent;

		const rank start_rank = child_board.get_start_rank();
		const file start_file = child_board.get_start_file();
		const rank end_rank = child_board.get_end_rank();
		const file end_file = child_board.get_end_file();

		const size_t start_idx = to_index(start_rank, start_file);
		const size_t end_idx = to_index(end_rank, end_file);

		if constexpr (move_type == move_type::en_passant_capture)
		{
			child.piece_at(start_rank, end_file) = empty; // the captured pawn will have the moving pawn's start rank and end file
		}
		else if (move_type == move_type::castle_kingside)
		{
			child.piece_at(start_rank, 5) = moving_color + rook;
			child.piece_at(start_rank, 7) = empty;
		}
		else if (move_type == move_type::castle_queenside)
		{
			child.piece_at(start_rank, 3) = moving_color + rook;
			child.piece_at(start_rank, 0) = empty;
		}

		child.piece_at(end_idx) = child_board.moved_piece<moving_color>();
		child.piece_at(start_idx) = empty;
	}

	template<color_t child_color>
	void make_move(position& child, const position& parent, const board& child_board)
	{
		// copy the parent position
		child = parent;

		const rank start_rank = child_board.get_start_rank();
		const file start_file = child_board.get_start_file();
		const rank end_rank = child_board.get_end_rank();
		const file end_file = child_board.get_end_file();

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
			if (diff(start_file, end_file) > 1)
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

		// handle the move, using child_board.moved_piece to handle promotion at the same time
		const piece moved_piece = child_board.moved_piece<other_color(child_color)>();
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

	template<color_t check_color>
	inline size_t find_king_index(const position& position)
	{
		// Scan for the position of the first set bit in the mask.
		// Assume that the board will always have a king of a given color.
		return get_next_bit(get_bitboard_for<check_color | king>(position));
	}

	// If the opponent isn't checking with a pawn, skip pawn checks.
	// If the opponent isn't checking with a knight, skip knight checks.
	enum class check_type
	{
		do_pawn_checks, // implies skipping knight and king checks
		do_knight_checks, // implies skipping pawn and king checks
		skip_pawn_and_knight_checks, // nominal case (only do sliding piece checks)

		opponent_move_unknown, // do all checks except king checks
		do_all // do all checks
	};

	template<check_type check_type>
	bool is_king_in_check(const position& position, const piece king_piece, const rank rank, const file file)
	{
		if constexpr (check_type == check_type::do_all)
		{
			if (square_is_attacked_by_king(position, king | other_color(king_piece.get_color()), rank, file)) return true;
		}

		if (square_is_attacked_by_rook_or_queen(position, other_color(king_piece.get_color()), rank, file)) return true;

		if (square_is_attacked_by_bishop_or_queen(position, other_color(king_piece.get_color()), rank, file)) return true;

		if constexpr (check_type == check_type::do_knight_checks ||
					  check_type == check_type::opponent_move_unknown ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_knight(position, knight | other_color(king_piece.get_color()), rank, file)) return true;
		}

		if constexpr (check_type == check_type::do_pawn_checks ||
					  check_type == check_type::opponent_move_unknown ||
					  check_type == check_type::do_all)
		{
			if (king_piece.is_white())
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
	using is_king_in_check_fn = bool(*)(const position&, const piece, const rank, const file);

	// This is only used by the GUI.
	inline bool is_king_in_check(const position& position, const color_t king_color)
	{
		size_t index = 0;
		if (king_color == white)
			index = find_king_index<white>(position);
		else
			index = find_king_index<black>(position);
		return is_king_in_check<check_type::do_all>(position, position.piece_at(index), index / 8, index % 8);
	}

	template<color_t color_to_move>
	void find_pawn_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
						 const size_t king_index, const tt::key key, is_king_in_check_fn check_fn)
	{
		const position& position = positions[parent_idx];
		const board& board = boards[parent_idx];

		if constexpr (color_to_move == white)
		{
			if (bounds_check(rank - 1)) // only validate moving forwards once
			{
				// check for moving forward one square
				if (position.piece_at(rank - 1, file).is_empty())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file, white_queen);
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file, white_rook);
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file, white_bishop);
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file, white_knight);
					}
					else // the pawn is moving without promotion
					{
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 6 && position.piece_at(5, file).is_empty() && position.piece_at(4, file).is_empty())
				{
					append_if_legal<color_to_move, pawn, move_type::pawn_two_squares>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, 4, file);
				}
				// check for captures
				if (bounds_check(file + 1) && position.piece_at(rank - 1, file + 1).is_occupied() && position.piece_at(rank - 1, file + 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file + 1, white_queen);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file + 1, white_rook);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file + 1, white_bishop);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file + 1, white_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && position.piece_at(rank - 1, file - 1).is_occupied() && position.piece_at(rank - 1, file - 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file - 1, white_queen);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file - 1, white_rook);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file - 1, white_bishop);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file - 1, white_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file - 1);
					}
				}
				// check for en passant captures
				if (rank == 3)
				{
					if (board.get_en_passant_file() == file - 1 && bounds_check(file - 1) && position.piece_at(rank, file - 1).is_pawn())
						append_if_legal<color_to_move, pawn, move_type::en_passant_capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file - 1);
					else if (board.get_en_passant_file() == file + 1 && bounds_check(file + 1) && position.piece_at(rank, file + 1).is_pawn())
						append_if_legal<color_to_move, pawn, move_type::en_passant_capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - 1, file + 1);
				}
			}
		}
		else
		{
			if (bounds_check(rank + 1)) // only validate moving forwards once
			{
				// check for moving forward one square
				if (position.piece_at(rank + 1, file).is_empty())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file, black_queen);
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file, black_rook);
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file, black_bishop);
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file, black_knight);
					}
					else // the pawn is moving without promotion
					{
						append_if_legal<color_to_move, pawn, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 1 && position.piece_at(2, file).is_empty() && position.piece_at(3, file).is_empty())
				{
					append_if_legal<color_to_move, pawn, move_type::pawn_two_squares>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, 3, file);
				}
				// check for captures
				if (bounds_check(file + 1) && position.piece_at(rank + 1, file + 1).is_occupied() && position.piece_at(rank + 1, file + 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file + 1, black_queen);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file + 1, black_rook);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file + 1, black_bishop);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file + 1, black_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && position.piece_at(rank + 1, file - 1).is_occupied() && position.piece_at(rank + 1, file - 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file - 1, black_queen);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file - 1, black_rook);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file - 1, black_bishop);
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file - 1, black_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<color_to_move, pawn, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file - 1);
					}
				}
				// check for en passant captures
				if (rank == 4)
				{
					if (board.get_en_passant_file() == file - 1 && bounds_check(file - 1) && position.piece_at(rank, file - 1).is_pawn())
						append_if_legal<color_to_move, pawn, move_type::en_passant_capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file - 1);
					else if (board.get_en_passant_file() == file + 1 && bounds_check(file + 1) && position.piece_at(rank, file + 1).is_pawn())
						append_if_legal<color_to_move, pawn, move_type::en_passant_capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + 1, file + 1);
				}
			}
		}
	}
	template<color_t color_to_move>
	void find_rook_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
						 const size_t king_index, const tt::key key, is_king_in_check_fn check_fn)
	{
		const position& position = positions[parent_idx];

		// rank descending
		for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
		{
			if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

			if (position.piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
			{
				append_if_legal<color_to_move, rook, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, end_rank, file);
				continue; // keep searching in the current direction
			}
			// if the rook has encountered an enemy piece
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				// the rook can capture...
				append_if_legal<color_to_move, rook, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, end_rank, file);
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
				append_if_legal<color_to_move, rook, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, end_rank, file);
				continue;
			}
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, rook, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, end_rank, file);
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
				append_if_legal<color_to_move, rook, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, rook, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank, end_file);
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
				append_if_legal<color_to_move, rook, move_type::other>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, rook, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank, end_file);
				break;
			}
			else break;
		}
	}
	template<color_t color_to_move>
	void find_bishop_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
						   const size_t king_index, const tt::key key, is_king_in_check_fn check_fn)
	{
		const position& position = positions[parent_idx];

		// working diagonally (rank and file descending)
		for (int offset = 1; offset < 8; ++offset)
		{
			// if the location is off of the board, stop searching in this direction
			if (!bounds_check(rank - offset, file - offset)) break;

			// if the square is empty
			if (position.piece_at(rank - offset, file - offset).is_empty())
			{
				// the bishop can move here
				append_if_legal<color_to_move>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - offset, file - offset);
				continue; // keep searching in this direction
			}
			// if the square is occupied by an enemy piece, the bishop can capture it
			else if (position.piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, other_piece, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - offset, file - offset);
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
				append_if_legal<color_to_move>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, other_piece, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank - offset, file + offset);
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
				append_if_legal<color_to_move>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + offset, file - offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, other_piece, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + offset, file - offset);
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
				append_if_legal<color_to_move>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, other_piece, move_type::capture>(out_index, parent_idx, king_index, key, check_fn, parent_idx, rank, file, rank + offset, file + offset);
				break;
			}
			else break;
		}
	}
	template<color_t color_to_move>
	void find_knight_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
						   const size_t king_index, const tt::key key, is_king_in_check_fn check_fn)
	{
		const bitboards bitboards = get_bitboards_for(positions[parent_idx]);

		const bitboard our_pieces = (color_to_move == white) ? bitboards.white : bitboards.black;
		const bitboard opp_pieces = (color_to_move == white) ? bitboards.black : bitboards.white;

		const size_t knight_index = to_index(rank, file);

		const bitboard moves = knight_movemasks[knight_index];

		bitboard captures = moves & opp_pieces;
		const bitboard blockers = moves & our_pieces;
		bitboard noncaptures = moves ^ captures ^ blockers;

		while (captures)
		{
			size_t target_square = get_next_bit(captures);
			captures = clear_next_bit(captures);

			append_if_legal<color_to_move, other_piece, move_type::capture>(out_index, parent_idx, king_index, key, check_fn,
																			parent_idx, rank, file, target_square / 8, target_square % 8);
		}

		while (noncaptures)
		{
			size_t target_square = get_next_bit(noncaptures);
			noncaptures = clear_next_bit(noncaptures);

			append_if_legal<color_to_move>(out_index, parent_idx, king_index, key, check_fn,
										   parent_idx, rank, file, target_square / 8, target_square % 8);
		}
	}
	template<color_t color_to_move>
	void find_queen_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
						  const size_t king_index, const tt::key key, is_king_in_check_fn check_fn)
	{
		find_rook_moves<color_to_move>(out_index, parent_idx, rank, file, king_index, key, check_fn);
		find_bishop_moves<color_to_move>(out_index, parent_idx, rank, file, king_index, key, check_fn);
	}
	template<color_t color_to_move>
	void find_king_moves(size_t& out_index, const size_t parent_idx,
						 const size_t king_index, const tt::key key, is_king_in_check_fn check_fn)
	{
		const position& position = positions[parent_idx];

		const rank rank = king_index / 8;
		const file file = king_index % 8;

		const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[to_index(rank, file)][color_to_move | king];

		// iterate over all adjacent squares
		for (int rank_d = -1; rank_d <= 1; ++rank_d)
		{
			for (int file_d = -1; file_d <= 1; ++file_d)
			{
				if (bounds_check(rank + rank_d, file + file_d))
				{
					if (position.piece_at(rank + rank_d, file + file_d).is_empty())
					{
						append_if_legal<color_to_move, king, move_type::other>(
							out_index, parent_idx, to_index(rank + rank_d, file + file_d), incremental_key, &is_king_in_check<check_type::do_all>,
							parent_idx, rank, file, rank + rank_d, file + file_d);
					}
					else if (!position.piece_at(rank + rank_d, file + file_d).is_color(color_to_move))
					{
						append_if_legal<color_to_move, king, move_type::capture>(
							out_index, parent_idx, to_index(rank + rank_d, file + file_d), incremental_key, &is_king_in_check<check_type::do_all>,
							parent_idx, rank, file, rank + rank_d, file + file_d);
					}
				}
			}
		}

		const piece& king_piece = position.piece_at(king_index);

		const board& board = boards[parent_idx];

		if ((king_piece.is_white() && board.white_can_castle_ks()) ||
			(king_piece.is_black() && board.black_can_castle_ks()))
		{
			// If white can castle kingside, we already know the king and rook are in place.
			if (position.piece_at(rank, file + 1).is_empty() && // Check if the squares in between are empty.
				position.piece_at(rank, file + 2).is_empty() &&
				!check_fn(position, king_piece, rank, file)) // Check if the king is in check now...
			{
				chess::position temp{};
				make_move(temp, position, rank, file, rank, file + 1); // ...on his way.
				if (!is_king_in_check<check_type::do_all>(temp, king_piece, rank, file + 1))
				{
					append_if_legal<color_to_move, king, move_type::castle_kingside>(
						out_index, parent_idx, to_index(rank, file + 2), incremental_key, &is_king_in_check<check_type::do_all>,
						parent_idx, rank, file, rank, file + 2); // the board constructor detects a castle and moves both pieces
				}
			}
		}

		if ((king_piece.is_white() && board.white_can_castle_qs()) || // (same logic as above)
			(king_piece.is_black() && board.black_can_castle_qs()))
		{
			if (position.piece_at(rank, file - 1).is_empty() &&
				position.piece_at(rank, file - 2).is_empty() &&
				position.piece_at(rank, file - 3).is_empty() && // we need to check that this square is empty for the rook to move through, but no check test is needed
				!check_fn(position, king_piece, rank, file))
			{
				chess::position temp{};
				make_move(temp, position, rank, file, rank, file - 1);
				if (!is_king_in_check<check_type::do_all>(temp, king_piece, rank, file - 1))
				{
					append_if_legal<color_to_move, king, move_type::castle_queenside>(
						out_index, parent_idx, to_index(rank, file - 2), incremental_key, &is_king_in_check<check_type::do_all>,
						parent_idx, rank, file, rank, file - 2);
				}
			}
		}
	}

	template <color_t moving_color, piece_t moving_piece_type = other_piece, move_type move_type = move_type::other, typename... board_args>
	void append_if_legal(size_t& out_index, const size_t parent_idx, const size_t king_index, const tt::key key,
						 is_king_in_check_fn check_fn, board_args&&... args)
	{
		static_assert(moving_piece_type == pawn ||
					  moving_piece_type == rook ||
					  moving_piece_type == king ||
					  moving_piece_type == other_piece);

		constexpr color_t child_color = other_color(moving_color);

		board& child_board = boards[out_index];
		child_board = board::template make_board<child_color, moving_piece_type, move_type>(std::forward<board_args>(args)...);

		position& child_position = positions[out_index];
		const position& parent_position = positions[parent_idx];
		make_move<moving_color, moving_piece_type, move_type>(child_position, parent_position, child_board);

		// if the king is in check, bail now
		constexpr piece king_piece = king | moving_color;
		if (check_fn(child_position, king_piece, king_index / 8, king_index % 8)) return;

		++out_index;

		child_board.update_key_and_static_eval<moving_color, moving_piece_type, move_type>(parent_position, boards[parent_idx], key);

		if constexpr (config::verify_incremental_zobrist_key)
			if (child_board.get_key() != generate_key(child_board, child_position, child_color))
				std::cout << "Incremental and generated zobrist key mismatch in append_if_legal\n";

		if constexpr (config::verify_incremental_static_eval)
			if (child_board.get_static_eval() != child_position.evaluate_position())
				std::cout << "Incremental and generated static evals mismatch in append_if_legal\n";
	}

	template<color_t color_to_move>
	piece get_last_moved_info(const board& board, rank& last_moved_end_rank, file& last_moved_end_file)
	{
		if (!board.has_move()) return empty; // todo: remove me

		last_moved_end_rank = board.get_end_rank();
		last_moved_end_file = board.get_end_file();
		return board.moved_piece<other_color(color_to_move)>();
	}

	template<color_t attacking_color>
	inline bool pawn_is_attacking(const rank attack_rank, const file attack_file,
								  const rank target_rank, const file target_file)
	{
		if (diff(attack_file, target_file) != 1) return false;

		if constexpr (attacking_color == white)
			return target_rank == attack_rank - 1;
		else
			return target_rank == attack_rank + 1;
	}

	inline bool knight_is_attacking(const rank attack_rank, const file attack_file,
									const rank target_rank, const file target_file)
	{
		const auto rank_diff = diff(attack_rank, target_rank);
		const auto file_diff = diff(attack_file, target_file);

		if (rank_diff == 1) return file_diff == 2;
		if (rank_diff == 2) return file_diff == 1;
		return false;
	}

	template<color_t color_to_move, bool perft = false>
	size_t generate_child_boards(const size_t parent_idx)
	{
		const board& parent_board = boards[parent_idx];
		tt::key key = parent_board.get_key() ^ tt::z_keys.black_to_move;

		const file parent_en_passant_file = parent_board.get_en_passant_file();
		if (parent_en_passant_file != empty)
		{
			key ^= tt::z_keys.en_passant_keys[parent_en_passant_file];
		}

		const position& parent_position = positions[parent_idx];

		rank last_moved_end_rank{};
		file last_moved_end_file{};
		const piece last_moved_piece = get_last_moved_info<color_to_move>(parent_board, last_moved_end_rank, last_moved_end_file);
		const size_t king_index = find_king_index<color_to_move>(parent_position);

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is known to be attacking the king.
		// - When generating non-king moves, never check for a king.
		// - When generating king moves, do all checks.
		is_king_in_check_fn check_fn{};

		if (last_moved_piece.is_pawn() &&
			pawn_is_attacking<other_color(color_to_move)>(last_moved_end_rank, last_moved_end_file,
														  king_index / 8, king_index % 8))
		{
			check_fn = &is_king_in_check<check_type::do_pawn_checks>;
		}
		else if (last_moved_piece.is_knight() &&
				 knight_is_attacking(last_moved_end_rank, last_moved_end_file,
									 king_index / 8, king_index % 8))
		{
			check_fn = &is_king_in_check<check_type::do_knight_checks>;
		}
		else if (!last_moved_piece.is_empty()) // the nominal path
		{
			check_fn = &is_king_in_check<check_type::skip_pawn_and_knight_checks>;
		}
		else
		{
			// Very occasionally (ie, at the original root), we do not know what the last move was.
			// Fall back to unconstrained move generation.
			check_fn = &is_king_in_check<check_type::opponent_move_unknown>;
		}

		const size_t begin_idx = first_child_index(parent_idx);
		size_t end_idx = begin_idx;

		find_moves_for<color_to_move | pawn>(end_idx, parent_idx, king_index, key, &find_pawn_moves<color_to_move>, check_fn);
		find_moves_for<color_to_move | knight>(end_idx, parent_idx, king_index, key, &find_knight_moves<color_to_move>, check_fn);
		find_moves_for<color_to_move | bishop>(end_idx, parent_idx, king_index, key, &find_bishop_moves<color_to_move>, check_fn);
		find_moves_for<color_to_move | rook>(end_idx, parent_idx, king_index, key, &find_rook_moves<color_to_move>, check_fn);
		find_moves_for<color_to_move | queen>(end_idx, parent_idx, king_index, key, &find_queen_moves<color_to_move>, check_fn);
		find_king_moves<color_to_move>(end_idx, parent_idx, king_index, key, check_fn);

		if constexpr (!perft)
		{
			// If there are no legal moves, record the result
			if (end_idx == begin_idx)
			{
				if (check_fn(parent_position, parent_position.piece_at(king_index), king_index / 8, king_index % 8))
					boards[parent_idx].set_result((color_to_move == white) ? result::black_wins_by_checkmate : result::white_wins_by_checkmate);
				else
					boards[parent_idx].set_result(result::draw_by_stalemate);
			}
		}

		return end_idx;
	}
}
