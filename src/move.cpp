
#include "bitboard.hpp"
#include "capture.hpp"
#include "config.hpp"
#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	// Only used in castling checks.
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
	[[clang::always_inline]] void make_move(position& child, const position& parent, const board& child_board)
	{
		// copy the parent position
		child = parent;

		const rank start_rank = child_board.get_start_rank();
		const file start_file = child_board.get_start_file();
		const rank end_rank = child_board.get_end_rank();
		const file end_file = child_board.get_end_file();

		const size_t start_idx = to_index(start_rank, start_file);
		const size_t end_idx = to_index(end_rank, end_file);

		if constexpr (move_type == move_type::capture)
		{
			// Record the position of the captured piece, so we don't generate sliding piece
			// attacks for it when searching for a check.
			captured_piece = (1ull << end_idx);
		}
		else if constexpr (move_type == move_type::en_passant_capture)
		{
			child.piece_at(start_rank, end_file) = empty; // the captured pawn will have the moving pawn's start rank and end file
		}
		else if constexpr (move_type == move_type::castle_kingside)
		{
			child.piece_at(start_rank, 5) = moving_color + rook;
			child.piece_at(start_rank, 7) = empty;
		}
		else if constexpr (move_type == move_type::castle_queenside)
		{
			child.piece_at(start_rank, 3) = moving_color + rook;
			child.piece_at(start_rank, 0) = empty;
		}

		if constexpr (moving_piece_type == king)
		{
			child.piece_at(end_idx) = moving_piece_type | moving_color;
		}
		else
		{
			child.piece_at(end_idx) = child_board.moved_piece<moving_color>();
		}

		child.piece_at(start_idx) = empty;
	}

	template<color_t attacker_color>
	inline_toggle bool square_is_attacked_by_pawn(const position& position,
												  const rank rank, const file file)
	{
		if constexpr (attacker_color == white)
		{
			if ((bounds_check(rank + 1, file + 1) && position.piece_at(rank + 1, file + 1).is(attacker_color | pawn)) ||
				(bounds_check(rank + 1, file - 1) && position.piece_at(rank + 1, file - 1).is(attacker_color | pawn))) return true;
		}
		else
		{
			if ((bounds_check(rank - 1, file + 1) && position.piece_at(rank - 1, file + 1).is(attacker_color | pawn)) ||
				(bounds_check(rank - 1, file - 1) && position.piece_at(rank - 1, file - 1).is(attacker_color | pawn))) return true;
		}

		return false;
	}
	template<color_t attacker_color>
	inline_toggle bool square_is_attacked_by_knight(const position& position, const rank rank, const file file)
	{
		return knight_attacks[to_index(rank, file)](position, attacker_color | knight);
	}
	template<color_t attacker_color>
	inline_toggle bool square_is_attacked_by_king(const position& position, const rank rank, const file file)
	{
		constexpr piece_t opp_king = attacker_color | king;

		// Check adjacent squares for a king.
		// Check rank first, because a king is likely on a top or bottom rank.
		if (bounds_check(rank - 1))
		{
			if (bounds_check(file - 1) && position.piece_at(rank - 1, file - 1).is(opp_king)) return true;
			if (position.piece_at(rank - 1, file).is(opp_king)) return true;
			if (bounds_check(file + 1) && position.piece_at(rank - 1, file + 1).is(opp_king)) return true;
		}

		if (bounds_check(file - 1) && position.piece_at(rank, file - 1).is(opp_king)) return true;
		if (bounds_check(file + 1) && position.piece_at(rank, file + 1).is(opp_king)) return true;

		if (bounds_check(rank + 1))
		{
			if (bounds_check(file - 1) && position.piece_at(rank + 1, file - 1).is(opp_king)) return true;
			if (position.piece_at(rank + 1, file).is(opp_king)) return true;
			if (bounds_check(file + 1) && position.piece_at(rank + 1, file + 1).is(opp_king)) return true;
		}

		return false;
	}

	template<color_t king_color>
	inline size_t find_king_index(const position& position)
	{
		// Scan for the position of the first set bit in the mask.
		// Assume that the board will always have a king of a given color.
		return get_next_bit(get_bitboard_for<king_color | king>(position));
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

	template<color_t king_color, check_type check_type>
	force_inline_toggle bool is_king_in_check(const position& position, const rank rank, const file file)
	{
		constexpr color_t opp_color = other_color(king_color);

		if (is_attacked_by_sliding_piece<king_color>(position, 1ull << to_index(rank, file))) return true;

		if constexpr (check_type == check_type::do_all)
		{
			if (square_is_attacked_by_king<opp_color>(position, rank, file)) return true;
		}

		if constexpr (check_type == check_type::do_knight_checks ||
					  check_type == check_type::opponent_move_unknown ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_knight<opp_color>(position, rank, file)) return true;
		}

		if constexpr (check_type == check_type::do_pawn_checks ||
					  check_type == check_type::opponent_move_unknown ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_pawn<opp_color>(position, rank, file)) return true;
		}

		return false;
	}
	using is_king_in_check_fn = bool(*)(const position&, const piece, const rank, const file);

	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void find_pawn_moves(size_t& out_index, const size_t parent_idx,
											 const size_t king_index, const bool started_in_check, const tt::key key)
	{
		constexpr int rank_delta = (color_to_move == white) ? -1 : 1;
		constexpr bitboard promotion_start_file = (color_to_move == white) ? rank_7 : rank_2;
		constexpr bitboard ep_capture_start_rank = (color_to_move == white) ? rank_5 : rank_4;
		constexpr size_t ep_capture_end_rank = (color_to_move == white) ? 2 : 5;

		const position& position = positions[parent_idx];

		const bitboard pawns = get_bitboard_for<color_to_move | pawn>(position);

		const bitboards bitboards = get_bitboards_for(position);
		const bitboard opp_pieces = (color_to_move == white) ? bitboards.black : bitboards.white;
		bitboard empty_squares = bitboards.empty;

		bitboard move_one_square = pawns & ((color_to_move == white) ? empty_squares << 8 : empty_squares >> 8);

		bitboard noncapture_promotions = move_one_square & promotion_start_file;
		move_one_square ^= noncapture_promotions;

		while (noncapture_promotions)
		{
			size_t start_idx = get_next_bit(noncapture_promotions);
			noncapture_promotions = clear_next_bit(noncapture_promotions);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn>(out_index, parent_idx, king_index, started_in_check, incremental_key,
															 parent_idx, rank, file, rank + rank_delta, file, queen);
			append_if_legal<color_to_move, check_type, pawn>(out_index, parent_idx, king_index, started_in_check, incremental_key,
															 parent_idx, rank, file, rank + rank_delta, file, knight);
			append_if_legal<color_to_move, check_type, pawn>(out_index, parent_idx, king_index, started_in_check, incremental_key,
															 parent_idx, rank, file, rank + rank_delta, file, rook);
			append_if_legal<color_to_move, check_type, pawn>(out_index, parent_idx, king_index, started_in_check, incremental_key,
															 parent_idx, rank, file, rank + rank_delta, file, bishop);
		}

		bitboard move_two_squares = move_one_square
			& (color_to_move == white ? rank_2 : rank_7)
			& ((color_to_move == white) ? empty_squares << 16 : empty_squares >> 16);

		while (move_two_squares)
		{
			size_t start_idx = get_next_bit(move_two_squares);
			move_two_squares = clear_next_bit(move_two_squares);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::pawn_two_squares>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																						  parent_idx, rank, file, rank + 2 * rank_delta, file);
		}

		while (move_one_square)
		{
			size_t start_idx = get_next_bit(move_one_square);
			move_one_square = clear_next_bit(move_one_square);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn>(out_index, parent_idx, king_index, started_in_check, incremental_key,
															 parent_idx, rank, file, rank + rank_delta, file);
		}

		bitboard capture_to_lower_file = pawns & pawn_capture_lower_file
			& ((color_to_move == white) ? opp_pieces << 9 : opp_pieces >> 7);
		bitboard capture_to_lower_file_promotion = capture_to_lower_file & promotion_start_file;
		capture_to_lower_file ^= capture_to_lower_file_promotion;

		while (capture_to_lower_file_promotion)
		{
			size_t start_idx = get_next_bit(capture_to_lower_file_promotion);
			capture_to_lower_file_promotion = clear_next_bit(capture_to_lower_file_promotion);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file - 1, queen);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file - 1, knight);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file - 1, rook);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file - 1, bishop);
		}

		while (capture_to_lower_file)
		{
			size_t start_idx = get_next_bit(capture_to_lower_file);
			capture_to_lower_file = clear_next_bit(capture_to_lower_file);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file - 1);
		}

		bitboard capture_to_higher_file = pawns & pawn_capture_higher_file
			& ((color_to_move == white) ? opp_pieces << 7 : opp_pieces >> 9);
		bitboard capture_to_higher_file_promotion = capture_to_higher_file & promotion_start_file;
		capture_to_higher_file ^= capture_to_higher_file_promotion;

		while (capture_to_higher_file_promotion)
		{
			size_t start_idx = get_next_bit(capture_to_higher_file_promotion);
			capture_to_higher_file_promotion = clear_next_bit(capture_to_higher_file_promotion);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file + 1, queen);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file + 1, knight);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file + 1, rook);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file + 1, bishop);
		}

		while (capture_to_higher_file)
		{
			size_t start_idx = get_next_bit(capture_to_higher_file);
			capture_to_higher_file = clear_next_bit(capture_to_higher_file);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(out_index, parent_idx, king_index, started_in_check, incremental_key,
																				 parent_idx, rank, file, rank + rank_delta, file + 1);
		}

		const file ep_capture_file = boards[parent_idx].get_en_passant_file();

		if (ep_capture_file != empty)
		{
			bitboard ep_capturers = pawns & ep_capture_start_rank & (ep_capture_mask << (ep_capture_file + (color_to_move == white ? 0 : 8)));

			while (ep_capturers) // 0-2
			{
				size_t start_idx = get_next_bit(ep_capturers);
				ep_capturers = clear_next_bit(ep_capturers);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[start_idx][color_to_move | pawn];

				const rank rank = start_idx / 8;
				const file file = start_idx % 8;
				append_if_legal<color_to_move, check_type, pawn, move_type::en_passant_capture>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, ep_capture_end_rank, ep_capture_file);
			}
		}
	}
	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void find_rook_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											 const size_t king_index, const bool started_in_check, const tt::key key)
	{
		const position& position = positions[parent_idx];

		// rank descending
		for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
		{
			if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

			if (position.piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
			{
				append_if_legal<color_to_move, check_type, rook>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, end_rank, file);
				continue; // keep searching in the current direction
			}
			// if the rook has encountered an enemy piece
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				// the rook can capture...
				append_if_legal<color_to_move, check_type, rook, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, end_rank, file);
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
				append_if_legal<color_to_move, check_type, rook>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, end_rank, file);
				continue;
			}
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, rook, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, end_rank, file);
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
				append_if_legal<color_to_move, check_type, rook>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, rook, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank, end_file);
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
				append_if_legal<color_to_move, check_type, rook>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank, end_file);
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, rook, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank, end_file);
				break;
			}
			else break;
		}
	}
	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void find_bishop_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											   const size_t king_index, const bool started_in_check, const tt::key key)
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
				append_if_legal<color_to_move, check_type>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank - offset, file - offset);
				continue; // keep searching in this direction
			}
			// if the square is occupied by an enemy piece, the bishop can capture it
			else if (position.piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, other_piece, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank - offset, file - offset);
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
				append_if_legal<color_to_move, check_type>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank - offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, other_piece, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank - offset, file + offset);
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
				append_if_legal<color_to_move, check_type>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank + offset, file - offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, other_piece, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank + offset, file - offset);
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
				append_if_legal<color_to_move, check_type>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank + offset, file + offset);
				continue;
			}
			else if (position.piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, other_piece, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key, parent_idx, rank, file, rank + offset, file + offset);
				break;
			}
			else break;
		}
	}
	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void find_knight_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											   const size_t king_index, const bool started_in_check, const tt::key key)
	{
		const bitboards bitboards = get_bitboards_for(positions[parent_idx]);
		const bitboard opp_pieces = (color_to_move == white) ? bitboards.black : bitboards.white;

		const size_t knight_index = to_index(rank, file);
		const bitboard moves = knight_movemasks[knight_index];

		bitboard captures = moves & opp_pieces;
		bitboard noncaptures = moves & bitboards.empty;

		while (captures)
		{
			size_t target_square = get_next_bit(captures);
			captures = clear_next_bit(captures);

			append_if_legal<color_to_move, check_type, other_piece, move_type::capture>(out_index, parent_idx, king_index, started_in_check, key,
																						parent_idx, rank, file, target_square / 8, target_square % 8);
		}

		while (noncaptures)
		{
			size_t target_square = get_next_bit(noncaptures);
			noncaptures = clear_next_bit(noncaptures);

			append_if_legal<color_to_move, check_type>(out_index, parent_idx, king_index, started_in_check, key,
													   parent_idx, rank, file, target_square / 8, target_square % 8);
		}
	}
	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void find_queen_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											  const size_t king_index, const bool started_in_check, const tt::key key)
	{
		find_rook_moves<color_to_move, check_type>(out_index, parent_idx, rank, file, king_index, started_in_check, key);
		find_bishop_moves<color_to_move, check_type>(out_index, parent_idx, rank, file, king_index, started_in_check, key);
	}
	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void find_king_moves(size_t& out_index, const size_t parent_idx,
											 const size_t king_index, const bool started_in_check, const tt::key key)
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
						append_if_legal<color_to_move, check_type::do_all, king>(
							out_index, parent_idx, to_index(rank + rank_d, file + file_d), started_in_check, incremental_key,
							parent_idx, rank, file, rank + rank_d, file + file_d);
					}
					else if (!position.piece_at(rank + rank_d, file + file_d).is_color(color_to_move))
					{
						append_if_legal<color_to_move, check_type::do_all, king, move_type::capture>(
							out_index, parent_idx, to_index(rank + rank_d, file + file_d), started_in_check, incremental_key,
							parent_idx, rank, file, rank + rank_d, file + file_d);
					}
				}
			}
		}

		if (started_in_check) return; // A king cannot castle out of check.

		const board& board = boards[parent_idx];

		if ((color_to_move == white && board.white_can_castle_ks()) ||
			(color_to_move == black && board.black_can_castle_ks()))
		{
			// If a player can castle kingside, the king and rook must (still) be in place.
			if (position.piece_at(rank, file + 1).is_empty() && // Check that the passed-through squares are empty.
				position.piece_at(rank, file + 2).is_empty())
			{
				chess::position temp{};
				make_move(temp, position, rank, file, rank, file + 1);
				// Check that the king would not be moving through check.
				if (!is_king_in_check<color_to_move, check_type::do_all>(temp, rank, file + 1))
				{
					append_if_legal<color_to_move, check_type::do_all, king, move_type::castle_kingside>(
						out_index, parent_idx, to_index(rank, file + 2), started_in_check, incremental_key,
						parent_idx, rank, file, rank, file + 2); // the board constructor detects a castle and moves both pieces
				}
			}
		}

		if ((color_to_move == white && board.white_can_castle_qs()) || // (same logic as above)
			(color_to_move == black && board.black_can_castle_qs()))
		{
			if (position.piece_at(rank, file - 1).is_empty() &&
				position.piece_at(rank, file - 2).is_empty() &&
				position.piece_at(rank, file - 3).is_empty())
			{
				chess::position temp{};
				make_move(temp, position, rank, file, rank, file - 1);
				if (!is_king_in_check<color_to_move, check_type::do_all>(temp, rank, file - 1))
				{
					append_if_legal<color_to_move, check_type::do_all, king, move_type::castle_queenside>(
						out_index, parent_idx, to_index(rank, file - 2), started_in_check, incremental_key,
						parent_idx, rank, file, rank, file - 2);
				}
			}
		}
	}

	force_inline_toggle bool moving_piece_might_have_been_pinned(const size_t king_index,
																 const size_t, const rank start_rank, const file start_file,
																 const rank, const file)
	{
		const bitboard king_check_mask = bishop_movemasks[king_index] | rook_movemasks[king_index];
		const bitboard piece_mask = 1ull << to_index(start_rank, start_file);
		// If a piece left a square that shared a rank, file, or diagonal with the king, it might have been pinned.
		return (king_check_mask & piece_mask) != 0;
	}
	force_inline_toggle bool moving_piece_might_have_been_pinned(const size_t king_index,
																 const size_t parent_idx, const rank start_rank, const file start_file,
																 const rank end_rank, const file end_file, const piece)
	{
		return moving_piece_might_have_been_pinned(king_index, parent_idx, start_rank, start_file, end_rank, end_file);
	}

	template <color_t moving_color, check_type check_type, piece_t moving_piece_type = other_piece,
		move_type move_type = move_type::other, typename... board_args>
	force_inline_toggle void append_if_legal(size_t& out_index, const size_t parent_idx,
											 const size_t king_index, const bool started_in_check, const tt::key key,
											 board_args&&... args)
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

		// If the king is in check, return early. We could be in check if:
		// - We are trying to move our king, or
		// - the move is an en-passant capture, or
		// - we started in check, or
		// - the moving piece shared a rank, file, or diagonal with the king.
		// If none of these are the case (most of the time), we cannot possibly be in check, and
		// we can skip the expensive call to is_king_in_check().
		if (moving_piece_type == king ||
			move_type == move_type::en_passant_capture ||
			started_in_check ||
			moving_piece_might_have_been_pinned(king_index, std::forward<board_args>(args)...))
		{
			if (is_king_in_check<moving_color, check_type>(child_position, king_index / 8, king_index % 8)) return;
		}
		else if constexpr (move_type == move_type::capture)
		{
			captured_piece = 0;
		}

		++out_index;

		child_board.update_key_and_eval<moving_color, moving_piece_type, move_type>(parent_position, boards[parent_idx], key);

		if constexpr (config::verify_incremental_key)
			if (child_board.get_key() != generate_key(child_board, child_position, child_color))
				std::cout << "Incremental and generated keys mismatch in append_if_legal\n";

		if constexpr (config::verify_incremental_eval)
			if (child_board.get_eval() != child_position.evaluate_position())
				std::cout << "Incremental and generated evals mismatch in append_if_legal\n";
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
	inline_toggle bool pawn_is_attacking(const rank attack_rank, const file attack_file,
										 const rank target_rank, const file target_file)
	{
		if (diff(attack_file, target_file) != 1) return false;

		if constexpr (attacking_color == white)
			return target_rank == attack_rank - 1;
		else
			return target_rank == attack_rank + 1;
	}

	inline_toggle bool knight_is_attacking(const rank attack_rank, const file attack_file,
										   const rank target_rank, const file target_file)
	{
		const auto rank_diff = diff(attack_rank, target_rank);
		const auto file_diff = diff(attack_file, target_file);

		if (rank_diff == 1) return file_diff == 2;
		if (rank_diff == 2) return file_diff == 1;
		return false;
	}

	template<color_t color_to_move, check_type check_type>
	force_inline_toggle void generate_child_boards(size_t& end_idx, const size_t parent_idx,
												   const size_t king_index, const bool started_in_check,
												   const tt::key key)
	{
		find_pawn_moves<color_to_move, check_type>(
			end_idx, parent_idx, king_index, started_in_check, key);
		find_moves_for<color_to_move | knight>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_knight_moves<color_to_move, check_type>);
		find_moves_for<color_to_move | bishop>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_bishop_moves<color_to_move, check_type>);
		find_moves_for<color_to_move | rook>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_rook_moves<color_to_move, check_type>);
		find_moves_for<color_to_move | queen>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_queen_moves<color_to_move, check_type>);
		find_king_moves<color_to_move, check_type>(
			end_idx, parent_idx, king_index, started_in_check, key);
	}

	template<color_t color_to_move, bool perft>
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

		constexpr color_t opp_color = other_color(color_to_move);
		set_up_qb_and_qr_bitboards_for<opp_color>(parent_position);

		rank last_moved_end_rank{};
		file last_moved_end_file{};
		const piece last_moved_piece = get_last_moved_info<color_to_move>(parent_board, last_moved_end_rank, last_moved_end_file);
		const size_t king_index = find_king_index<color_to_move>(parent_position);

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is known to be attacking the king.
		// - When generating non-king moves, never check for a king.
		// - When generating king moves, do all checks.

		const size_t begin_idx = first_child_index(parent_idx);
		size_t end_idx = begin_idx;
		bool started_in_check = false;

		if (last_moved_piece.is_pawn() && pawn_is_attacking<opp_color>(
			last_moved_end_rank, last_moved_end_file, king_index / 8, king_index % 8))
		{
			started_in_check = true;
			generate_child_boards<color_to_move, check_type::do_pawn_checks>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}
		else if (last_moved_piece.is_knight() && knight_is_attacking(
			last_moved_end_rank, last_moved_end_file, king_index / 8, king_index % 8))
		{
			started_in_check = true;
			generate_child_boards<color_to_move, check_type::do_knight_checks>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}
		else if (!last_moved_piece.is_empty()) // the nominal path
		{
			started_in_check = is_king_in_check<color_to_move, check_type::skip_pawn_and_knight_checks>(
				parent_position, king_index / 8, king_index % 8);
			generate_child_boards<color_to_move, check_type::skip_pawn_and_knight_checks>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}
		else
		{
			// Very occasionally (ie, at the original root), we do not know what the last move was.
			// Fall back to unconstrained move generation.
			started_in_check = is_king_in_check<color_to_move, check_type::opponent_move_unknown>(
				parent_position, king_index / 8, king_index % 8);
			generate_child_boards<color_to_move, check_type::opponent_move_unknown>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}

		if constexpr (!perft)
		{
			// If there are no legal moves, record the result
			if (end_idx == begin_idx)
			{
				boards[parent_idx].set_terminal();

				if (started_in_check) // checkmate
					boards[parent_idx].set_eval((color_to_move == white) ? -eval::mate : eval::mate);
				else // stalemate
					boards[parent_idx].set_eval(0);
			}
		}

		return end_idx;
	}

	template size_t generate_child_boards<white>(const size_t);
	template size_t generate_child_boards<white, true>(const size_t);
	template size_t generate_child_boards<black>(const size_t);
	template size_t generate_child_boards<black, true>(const size_t);
}
