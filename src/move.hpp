#pragma once

#include <vector>

#include "bitboard.hpp"
#include "board.hpp"
#include "node.hpp"
#include "position.hpp"
#include "capture.hpp"

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

	template<typename board_t>
	void make_move(position& child, const position& parent, const board_t& child_board)
	{
		// copy the parent position
		child = parent;

		const rank start_rank = child_board.start_rank();
		const file start_file = child_board.start_file();
		const rank end_rank = child_board.end_rank();
		const file end_file = child_board.end_file();

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
		const piece moved_piece = child_board.moved_piece();
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
	bool is_king_in_check(const position& position, const piece king, const rank rank, const file file)
	{
		if constexpr (check_type == check_type::do_all)
		{
			if (square_is_attacked_by_king(position, detail::king | other(king.get_color()), rank, file)) return true;
		}

		if (square_is_attacked_by_rook_or_queen(position, other(king.get_color()), rank, file)) return true;

		if (square_is_attacked_by_bishop_or_queen(position, other(king.get_color()), rank, file)) return true;

		if constexpr (check_type == check_type::do_knight_checks ||
					  check_type == check_type::opponent_move_unknown ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_knight(position, detail::knight | other(king.get_color()), rank, file)) return true;
		}

		if constexpr (check_type == check_type::do_pawn_checks ||
					  check_type == check_type::opponent_move_unknown ||
					  check_type == check_type::do_all)
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
	using is_king_in_check_fn = bool(*)(const position&, const piece, const rank, const file);

	template<typename nodes_t, typename parent_node_t>
	void find_pawn_moves(nodes_t& child_nodes, const parent_node_t& parent_node, const position& position, const rank rank, const file file,
						 const size_t king_index, is_king_in_check_fn check_fn)
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
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file, white_queen);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file, white_rook);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file, white_bishop);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file, white_knight);
					}
					else // the pawn is moving without promotion
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 6 && position.piece_at(5, file).is_empty() && position.piece_at(4, file).is_empty())
				{
					append_if_legal<move_type::pawn_two_squares>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, 4, file);
				}
				// check for captures
				if (bounds_check(file + 1) && position.piece_at(rank - 1, file + 1).is_occupied() && position.piece_at(rank - 1, file + 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 1, white_queen);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 1, white_rook);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 1, white_bishop);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 1, white_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && position.piece_at(rank - 1, file - 1).is_occupied() && position.piece_at(rank - 1, file - 1).is_black())
				{
					if (rank == 1) // if the pawn is on the second last rank
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 1, white_queen);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 1, white_rook);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 1, white_bishop);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 1, white_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 1);
					}
				}
				// check for en passant
				if (rank == 3)
				{
					if (parent_node.board.en_passant_file() == file - 1 && bounds_check(file - 1) && position.piece_at(rank, file - 1).is_pawn())
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 1);
					else if (parent_node.board.en_passant_file() == file + 1 && bounds_check(file + 1) && position.piece_at(rank, file + 1).is_pawn())
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 1);
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
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file, black_queen);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file, black_rook);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file, black_bishop);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file, black_knight);
					}
					else // the pawn is moving without promotion
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file);
					}
				}
				// check for moving forward two squares
				if (rank == 1 && position.piece_at(2, file).is_empty() && position.piece_at(3, file).is_empty())
				{
					append_if_legal<move_type::pawn_two_squares>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, 3, file);
				}
				// check for captures
				if (bounds_check(file + 1) && position.piece_at(rank + 1, file + 1).is_occupied() && position.piece_at(rank + 1, file + 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 1, black_queen);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 1, black_rook);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 1, black_bishop);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 1, black_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 1);
					}
				}
				if (bounds_check(file - 1) && position.piece_at(rank + 1, file - 1).is_occupied() && position.piece_at(rank + 1, file - 1).is_white())
				{
					if (rank == 6) // if the pawn is on the second last rank
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 1, black_queen);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 1, black_rook);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 1, black_bishop);
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 1, black_knight);
					}
					else // the pawn is capturing without promotion
					{
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 1);
					}
				}
				// check for en passant
				if (rank == 4)
				{
					if (parent_node.board.en_passant_file() == file - 1 && bounds_check(file - 1) && position.piece_at(rank, file - 1).is_pawn())
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 1);
					else if (parent_node.board.en_passant_file() == file + 1 && bounds_check(file + 1) && position.piece_at(rank, file + 1).is_pawn())
						append_if_legal<move_type::pawn_other>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 1);
				}
			}
		}
	}
	template<typename nodes_t, typename parent_node_t>
	void find_rook_moves(nodes_t& child_nodes, const parent_node_t& parent_node, const position& position, const rank rank, const file file,
						 const size_t king_index, is_king_in_check_fn check_fn)
	{
		constexpr color_t color_to_move = parent_node_t::color_to_move();

		// rank descending
		for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
		{
			if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

			if (position.piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
			{
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, end_rank, file);
				continue; // keep searching in the current direction
			}
			// if the rook has encountered an enemy piece
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				// the rook can capture...
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, end_rank, file);
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
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, end_rank, file);
				continue;
			}
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, end_rank, file);
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
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank, end_file);
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
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<move_type::rook>(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank, end_file);
				break;
			}
			else break;
		}
	}
	template<typename nodes_t, typename parent_node_t>
	void find_bishop_moves(nodes_t& child_nodes, const parent_node_t& parent_node, const position& position, const rank rank, const file file,
						   const size_t king_index, is_king_in_check_fn check_fn)
	{
		constexpr color_t color_to_move = parent_node_t::color_to_move();

		// working diagonally (rank and file descending)
		for (int offset = 1; offset < 8; ++offset)
		{
			// if the location is off of the board, stop searching in this direction
			if (!bounds_check(rank - offset, file - offset)) break;

			// if the square is empty
			if (position.piece_at(rank - offset, file - offset).is_empty())
			{
				// the bishop can move here
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - offset, file - offset);
				continue; // keep searching in this direction
			}
			// if the square is occupied by an enemy piece, the bishop can capture it
			else if (position.piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - offset, file - offset);
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
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - offset, file + offset);
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
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + offset, file - offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + offset, file - offset);
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
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + offset, file + offset);
				break;
			}
			else break;
		}
	}
	template<typename nodes_t, typename parent_node_t>
	void find_knight_moves(nodes_t& child_nodes, const parent_node_t& parent_node, const position& position, const rank rank, const file file,
						   const size_t king_index, is_king_in_check_fn check_fn)
	{
		constexpr color_t color_to_move = parent_node_t::color_to_move();

		if (bounds_check(rank - 2, file + 1) &&
			!(position.piece_at(rank - 2, file + 1).is_occupied() && position.piece_at(rank - 2, file + 1).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 2, file + 1);
		if (bounds_check(rank - 1, file + 2) &&
			!(position.piece_at(rank - 1, file + 2).is_occupied() && position.piece_at(rank - 1, file + 2).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file + 2);

		if (bounds_check(rank + 1, file + 2) &&
			!(position.piece_at(rank + 1, file + 2).is_occupied() && position.piece_at(rank + 1, file + 2).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file + 2);
		if (bounds_check(rank + 2, file + 1) &&
			!(position.piece_at(rank + 2, file + 1).is_occupied() && position.piece_at(rank + 2, file + 1).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 2, file + 1);

		if (bounds_check(rank + 2, file - 1) &&
			!(position.piece_at(rank + 2, file - 1).is_occupied() && position.piece_at(rank + 2, file - 1).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 2, file - 1);
		if (bounds_check(rank + 1, file - 2) &&
			!(position.piece_at(rank + 1, file - 2).is_occupied() && position.piece_at(rank + 1, file - 2).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank + 1, file - 2);

		if (bounds_check(rank - 1, file - 2) &&
			!(position.piece_at(rank - 1, file - 2).is_occupied() && position.piece_at(rank - 1, file - 2).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 1, file - 2);
		if (bounds_check(rank - 2, file - 1) &&
			!(position.piece_at(rank - 2, file - 1).is_occupied() && position.piece_at(rank - 2, file - 1).is_color(color_to_move)))
			append_if_legal(child_nodes, parent_node, position, king_index, check_fn, parent_node.board, position, rank, file, rank - 2, file - 1);
	}
	template<typename nodes_t, typename parent_node_t>
	void find_queen_moves(nodes_t& child_nodes, const parent_node_t& parent_node, const position& position, const rank rank, const file file,
						  const size_t king_index, is_king_in_check_fn check_fn)
	{
		find_rook_moves(child_nodes, parent_node, position, rank, file, king_index, check_fn);
		find_bishop_moves(child_nodes, parent_node, position, rank, file, king_index, check_fn);
	}
	template<typename nodes_t, typename parent_node_t>
	void find_king_moves(nodes_t& child_nodes, const parent_node_t& parent_node, const position& position, const rank rank, const file file)
	{
		constexpr color_t color_to_move = parent_node_t::color_to_move();

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
					append_if_legal<move_type::king>(child_nodes, parent_node, position, to_index(rank + rank_d, file + file_d), &is_king_in_check<check_type::do_all>,
													 parent_node.board, position, rank, file, rank + rank_d, file + file_d);
				}
			}
		}

		const piece& king = position.piece_at(rank, file);

		if ((king.is_white() && parent_node.board.white_can_castle_ks()) ||
			(king.is_black() && parent_node.board.black_can_castle_ks()))
		{
			// If white can castle kingside, we already know the king and rook are in place.
			if (position.piece_at(rank, file + 1).is_empty() && // Check if the squares in between are empty.
				position.piece_at(rank, file + 2).is_empty() &&
				!is_king_in_check(parent_node.board, position, king, rank, file)) // Check if the king is in check now...
			{
				chess::position temp{};
				make_move(temp, position, rank, file, rank, file + 1); // ...on his way...
				if (!is_king_in_check<check_type::do_all>(temp, king, rank, file + 1))
				{
					make_move(temp, position, rank, file, rank, file + 2);  // ...or at his destination.
					if (!is_king_in_check<check_type::do_all>(temp, king, rank, file + 2))
					{
						append_if_legal<move_type::king>(
							child_nodes, parent_node, position, to_index(rank, file + 2), &is_king_in_check<check_type::do_all>,
							parent_node.board, position, rank, file, rank, file + 2); // the board constructor detects a castle and moves both pieces
					}
				}
			}
		}

		if ((king.is_white() && parent_node.board.white_can_castle_qs()) || // (same logic as above)
			(king.is_black() && parent_node.board.black_can_castle_qs()))
		{
			if (position.piece_at(rank, file - 1).is_empty() &&
				position.piece_at(rank, file - 2).is_empty() &&
				position.piece_at(rank, file - 3).is_empty() && // we need to check that this square is empty for the rook to move through, but no check test is needed
				!is_king_in_check(parent_node.board, position, king, rank, file))
			{
				chess::position temp{};
				make_move(temp, position, rank, file, rank, file - 1);
				if (!is_king_in_check<check_type::do_all>(temp, king, rank, file - 1))
				{
					make_move(temp, position, rank, file, rank, file - 2);
					if (!is_king_in_check<check_type::do_all>(temp, king, rank, file - 2))
					{
						append_if_legal<move_type::king>(
							child_nodes, parent_node, position, to_index(rank, file - 2), &is_king_in_check<check_type::do_all>,
							parent_node.board, position, rank, file, rank, file - 2);
					}
				}
			}
		}
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
				return is_king_in_check<check_type::do_pawn_checks>(position, king, rank, file);
			else if (last_moved.is_knight())
				return is_king_in_check<check_type::do_knight_checks>(position, king, rank, file);
			else
				return is_king_in_check<check_type::skip_pawn_and_knight_checks>(position, king, rank, file);
		}

		// We don't know what move created this position, or it is not the king's color's turn to move; do all checks.
		return is_king_in_check<check_type::do_all>(position, king, rank, file);
	}

	template<color_t check_color>
	inline size_t find_king_index(const position& position)
	{
		// Scan for the position of the first set bit in the mask.
		// Assume that the board will always have a king of a given color.
		return _tzcnt_u64(get_bitboard_for<check_color | detail::king>(position));
	}

	// This is only used by the GUI.
	template<typename board_t>
	bool is_king_in_check(const board_t& board, const position& position, const color_t king_color)
	{
		size_t index = 0;
		if (king_color == white)
			index = find_king_index<white>(position);
		else
			index = find_king_index<black>(position);
		return is_king_in_check(board, position, position.piece_at(index), index / 8, index % 8);
	}

	template <move_type move_type = move_type::other, typename nodes_t, typename parent_node_t, typename... board_args>
	void append_if_legal(nodes_t& child_nodes, const parent_node_t& parent_node, const position& parent_position, const size_t king_index,
						 is_king_in_check_fn check_fn,
						 board_args... args)
	{
		using child_node_t = nodes_t::value_type;
		using child_board_t = child_node_t::board_t;
		constexpr piece king = detail::king | parent_node_t::color_to_move();

		const child_board_t child_board = child_board_t::template make_board<move_type>(std::forward<board_args>(args)...);

		chess::position child_position{};
		make_move(child_position, parent_position, child_board);

		// if the king is in check, bail now
		if (check_fn(child_position, king, king_index / 8, king_index % 8)) return;

		child_node_t child_node(child_board);

		// generate incremental static eval
		child_node.generate_incremental_static_eval(parent_position, parent_node.get_static_eval(), child_board);

		child_nodes.push_back(child_node);
	}

	template<typename board_t>
	piece get_last_moved_info(const board_t& board, rank& last_moved_end_rank, file& last_moved_end_file)
	{
		if (!board.has_move()) return empty;

		last_moved_end_rank = board.end_rank();
		last_moved_end_file = board.end_file();
		return board.moved_piece();
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

	template<typename node_t>
	auto& generate_child_boards(node_t& node, const position& position)
	{
		using board_t = typename node_t::board_t;

		static std::vector<typename node_t::other_node_t> child_nodes;
		child_nodes.clear();

		constexpr color_t color_to_move = board_t::color_to_move();

		rank last_moved_end_rank{};
		file last_moved_end_file{};
		const piece last_moved_piece = get_last_moved_info(node.board, last_moved_end_rank, last_moved_end_file);
		const size_t king_index = find_king_index<color_to_move>(position);

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is known to be attacking the king.
		// - When generating non-king moves, never check for a king.
		// - When generating king moves, do all checks.
		is_king_in_check_fn check_fn{};

		if (last_moved_piece.is_pawn() &&
			pawn_is_attacking<board_t::other_color()>(last_moved_end_rank, last_moved_end_file,
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
						find_pawn_moves(child_nodes, node, position, rank, file, king_index, check_fn);
					}
					else if (position.piece_at(rank, file).is_rook())
					{
						find_rook_moves(child_nodes, node, position, rank, file, king_index, check_fn);
					}
					else if (position.piece_at(rank, file).is_bishop())
					{
						find_bishop_moves(child_nodes, node, position, rank, file, king_index, check_fn);
					}
					else if (position.piece_at(rank, file).is_knight())
					{
						find_knight_moves(child_nodes, node, position, rank, file, king_index, check_fn);
					}
					else if (position.piece_at(rank, file).is_queen())
					{
						find_queen_moves(child_nodes, node, position, rank, file, king_index, check_fn);
					}
					else if (position.piece_at(rank, file).is_king())
					{
						find_king_moves(child_nodes, node, position, rank, file);
					}
				} // end if piece can move
			}
		}

		// If there are no legal moves, record the result
		if (child_nodes.size() == 0)
		{
			if (is_king_in_check(node.board, position, color_to_move))
				node.board.set_result((color_to_move == white) ? result::black_wins_by_checkmate : result::white_wins_by_checkmate);
			else
				node.board.set_result(result::draw_by_stalemate);
		}

		return child_nodes;
	}
}
