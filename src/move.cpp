
#include "bitboard.hpp"
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
			child.piece_at(end_idx + ((moving_color == white) ? 8 : -8)) = empty; // the captured pawn will behind the moving pawn
		}
		else if constexpr (move_type == move_type::castle_kingside)
		{
			constexpr uint32_t pieces = (empty << 24) + ((moving_color | king) << 16) + ((moving_color | rook) << 8) + empty;
			constexpr size_t write_idx = (moving_color == white) ? 60 : 4;
			*(uint32_t*)&child[write_idx] = pieces;
		}
		else if constexpr (move_type == move_type::castle_queenside)
		{
			constexpr uint32_t pieces = ((moving_color | rook) << 24) + ((moving_color | king) << 16) + (empty << 8) + empty;
			constexpr size_t write_idx = (moving_color == white) ? 56 : 0;
			*(uint32_t*)&child[write_idx] = pieces;
		}

		// Add the moved piece in all cases except castling.
		if constexpr (move_type != move_type::castle_kingside &&
					  move_type != move_type::castle_queenside)
		{
			if constexpr (moving_piece_type != pawn ||
						  move_type == move_type::pawn_two_squares ||
						  move_type == move_type::en_passant_capture)
			{
				child.piece_at(end_idx) = moving_piece_type | moving_color;
			}
			else
			{
				child.piece_at(end_idx) = child_board.moved_piece<moving_color>();
			}
		}

		// Clear the moved piece in all cases except kingside castling.
		if constexpr (move_type != move_type::castle_kingside)
		{
			child.piece_at(start_idx) = empty;
		}
	}

	template<color_t attacker_color>
	[[clang::always_inline]] bool square_is_attacked_by_pawn(const position& position, const size_t king_idx)
	{
		const bitboard opp_pawns = get_bitboard_for<attacker_color | pawn>(position);
		const bitboard king_bb = 1ull << king_idx;

		const bitboard checkers_to_lower_file = opp_pawns & pawn_capture_lower_file
			& ((attacker_color == white) ? king_bb << 9 : king_bb >> 7);
		const bitboard checkers_to_higher_file = opp_pawns & pawn_capture_higher_file
			& ((attacker_color == white) ? king_bb << 7 : king_bb >> 9);

		return checkers_to_lower_file | checkers_to_higher_file;
	}
	template<color_t attacker_color>
	[[clang::always_inline]] bool square_is_attacked_by_knight(const position& position, const size_t index)
	{
		const bitboard opp_knights = get_bitboard_for<attacker_color | knight>(position);
		return opp_knights & knight_movemasks[index];
	}
	template<color_t attacker_color>
	[[clang::always_inline]] bool square_is_attacked_by_king(const position& position, const size_t index)
	{
		const bitboard opp_king = get_bitboard_for<attacker_color | king>(position);
		return opp_king & king_movemasks[index];
	}

	// If the opponent isn't checking with a pawn, skip pawn checks.
	// If the opponent isn't checking with a knight, skip knight checks.
	enum class check_type
	{
		do_pawn_checks, // implies skipping knight and king checks
		do_knight_checks, // implies skipping pawn and king checks
		skip_pawn_and_knight_checks, // nominal case (only do sliding piece checks)

		do_all // do all checks
	};

	template<color_t king_color, check_type check_type>
	force_inline_toggle bool is_king_in_check(const position& position, const rank rank, const file file)
	{
		constexpr color_t opp_color = other_color(king_color);
		const size_t king_index = to_index(rank, file);

		if (is_attacked_by_sliding_piece<king_color>(position, 1ull << king_index)) return true;

		if constexpr (check_type == check_type::do_all)
		{
			if (square_is_attacked_by_king<opp_color>(position, king_index)) return true;
		}

		if constexpr (check_type == check_type::do_knight_checks ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_knight<opp_color>(position, king_index)) return true;
		}

		if constexpr (check_type == check_type::do_pawn_checks ||
					  check_type == check_type::do_all)
		{
			if (square_is_attacked_by_pawn<opp_color>(position, king_index)) return true;
		}

		return false;
	}
	using is_king_in_check_fn = bool(*)(const position&, const piece, const rank, const file);

	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
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

		if constexpr (gen_moves == gen_moves::all)
		{
			bitboard empty_squares = bitboards.empty;

			bitboard move_one_square = pawns & ((color_to_move == white) ? empty_squares << 8 : empty_squares >> 8);

			bitboard noncapture_promotions = move_one_square & promotion_start_file;
			move_one_square ^= noncapture_promotions;

			while (noncapture_promotions)
			{
				size_t start_idx = get_next_bit(noncapture_promotions);
				noncapture_promotions = clear_next_bit(noncapture_promotions);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				const rank rank = start_idx / 8;
				const file file = start_idx % 8;
				append_if_legal<color_to_move, check_type, pawn>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, rank + rank_delta, file, queen);
				append_if_legal<color_to_move, check_type, pawn>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, rank + rank_delta, file, knight);
				append_if_legal<color_to_move, check_type, pawn>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, rank + rank_delta, file, rook);
				append_if_legal<color_to_move, check_type, pawn>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, rank + rank_delta, file, bishop);
			}

			bitboard move_two_squares = move_one_square
				& (color_to_move == white ? rank_2 : rank_7)
				& ((color_to_move == white) ? empty_squares << 16 : empty_squares >> 16);

			while (move_two_squares)
			{
				size_t start_idx = get_next_bit(move_two_squares);
				move_two_squares = clear_next_bit(move_two_squares);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				const rank rank = start_idx / 8;
				const file file = start_idx % 8;
				append_if_legal<color_to_move, check_type, pawn, move_type::pawn_two_squares>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, rank + 2 * rank_delta, file);
			}

			while (move_one_square)
			{
				size_t start_idx = get_next_bit(move_one_square);
				move_one_square = clear_next_bit(move_one_square);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				const rank rank = start_idx / 8;
				const file file = start_idx % 8;
				append_if_legal<color_to_move, check_type, pawn>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, rank + rank_delta, file);
			}
		}

		bitboard capture_to_lower_file = pawns & pawn_capture_lower_file
			& ((color_to_move == white) ? opp_pieces << 9 : opp_pieces >> 7);
		bitboard capture_to_lower_file_promotion = capture_to_lower_file & promotion_start_file;
		capture_to_lower_file ^= capture_to_lower_file_promotion;

		while (capture_to_lower_file_promotion)
		{
			size_t start_idx = get_next_bit(capture_to_lower_file_promotion);
			capture_to_lower_file_promotion = clear_next_bit(capture_to_lower_file_promotion);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file - 1, queen);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file - 1, knight);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file - 1, rook);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file - 1, bishop);
		}

		while (capture_to_lower_file)
		{
			size_t start_idx = get_next_bit(capture_to_lower_file);
			capture_to_lower_file = clear_next_bit(capture_to_lower_file);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
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

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file + 1, queen);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file + 1, knight);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file + 1, rook);
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
				parent_idx, rank, file, rank + rank_delta, file + 1, bishop);
		}

		while (capture_to_higher_file)
		{
			size_t start_idx = get_next_bit(capture_to_higher_file);
			capture_to_higher_file = clear_next_bit(capture_to_higher_file);

			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

			const rank rank = start_idx / 8;
			const file file = start_idx % 8;
			append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, incremental_key,
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

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				const rank rank = start_idx / 8;
				const file file = start_idx % 8;
				append_if_legal<color_to_move, check_type, pawn, move_type::en_passant_capture>(
					out_index, parent_idx, king_index, started_in_check, incremental_key,
					parent_idx, rank, file, ep_capture_end_rank, ep_capture_file);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, piece_t piece_type = rook>
	force_inline_toggle void find_rook_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											 const size_t king_index, const bool started_in_check, const tt::key key)
	{
		static_assert(piece_type == rook || piece_type == queen);

		const position& position = positions[parent_idx];

		// rank descending
		for (chess::rank end_rank = rank - 1; end_rank >= 0; --end_rank)
		{
			if (!bounds_check(end_rank)) break; // out of bounds; don't keep iterating in this direction

			if (position.piece_at(end_rank, file).is_empty()) // if the square is empty, the rook can move here
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, end_rank, file);
				}
				continue; // keep searching in the current direction
			}
			// if the rook has encountered an enemy piece
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				// the rook can capture
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, end_rank, file);
			}
			break; // the rook cannot move into a friendly piece; stop searching this way
		}

		// rank ascending (same logic as above)
		for (chess::rank end_rank = rank + 1; end_rank < 8; ++end_rank)
		{
			if (!bounds_check(end_rank)) break;

			if (position.piece_at(end_rank, file).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, end_rank, file);
				}
				continue;
			}
			else if (position.piece_at(end_rank, file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, end_rank, file);
			}
			break;
		}

		// file descending (same logic as above)
		for (chess::file end_file = file - 1; end_file >= 0; --end_file)
		{
			if (!bounds_check(end_file)) break;

			if (position.piece_at(rank, end_file).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, rank, end_file);
				}
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, rank, end_file);
			}
			break;
		}

		// file ascending (same logic as above)
		for (chess::file end_file = file + 1; end_file < 8; ++end_file)
		{
			if (!bounds_check(end_file)) break;

			if (position.piece_at(rank, end_file).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, rank, end_file);
				}
				continue;
			}
			else if (position.piece_at(rank, end_file).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, rank, end_file);
			}
			break;
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, piece_t piece_type = bishop>
	force_inline_toggle void find_bishop_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											   const size_t king_index, const bool started_in_check, const tt::key key)
	{
		static_assert(piece_type == bishop || piece_type == queen);

		const position& position = positions[parent_idx];

		// working diagonally (rank and file descending)
		for (int offset = 1; offset < 8; ++offset)
		{
			// if the location is off of the board, stop searching in this direction
			if (!bounds_check(rank - offset, file - offset)) break;

			// if the square is empty
			if (position.piece_at(rank - offset, file - offset).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					// the bishop can move here
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, rank - offset, file - offset);
				}
				continue; // keep searching in this direction
			}
			// if the square is occupied by an enemy piece, the bishop can capture it
			else if (position.piece_at(rank - offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, rank - offset, file - offset);
			}
			break;
		}

		// working diagonally (rank descending and file ascending) (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank - offset, file + offset)) break;

			if (position.piece_at(rank - offset, file + offset).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, rank - offset, file + offset);
				}
				continue;
			}
			else if (position.piece_at(rank - offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, rank - offset, file + offset);
			}
			break;
		}

		// working diagonally (rank ascending and file descending) (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank + offset, file - offset)) break;

			if (position.piece_at(rank + offset, file - offset).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, rank + offset, file - offset);
				}
				continue;
			}
			else if (position.piece_at(rank + offset, file - offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, rank + offset, file - offset);
			}
			break;
		}

		// working diagonally (rank and file ascending) (same logic as above)
		for (int offset = 1; offset < 8; ++offset)
		{
			if (!bounds_check(rank + offset, file + offset)) break;

			if (position.piece_at(rank + offset, file + offset).is_empty())
			{
				if constexpr (gen_moves == gen_moves::all)
				{
					append_if_legal<color_to_move, check_type, piece_type>(
						out_index, parent_idx, king_index, started_in_check, key,
						parent_idx, rank, file, rank + offset, file + offset);
				}
				continue;
			}
			else if (position.piece_at(rank + offset, file + offset).is_opposing_color(color_to_move))
			{
				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, rank + offset, file + offset);
			}
			break;
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
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

			append_if_legal<color_to_move, check_type, knight, move_type::capture>(
				out_index, parent_idx, king_index, started_in_check, key,
				parent_idx, rank, file, target_square / 8, target_square % 8);
		}

		if constexpr (gen_moves == gen_moves::all)
		{
			while (noncaptures)
			{
				size_t target_square = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<color_to_move, check_type, knight>(
					out_index, parent_idx, king_index, started_in_check, key,
					parent_idx, rank, file, target_square / 8, target_square % 8);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void find_queen_moves(size_t& out_index, const size_t parent_idx, const rank rank, const file file,
											  const size_t king_index, const bool started_in_check, const tt::key key)
	{
		find_rook_moves<color_to_move, gen_moves, check_type, queen>(out_index, parent_idx, rank, file, king_index, started_in_check, key);
		find_bishop_moves<color_to_move, gen_moves, check_type, queen>(out_index, parent_idx, rank, file, king_index, started_in_check, key);
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void find_king_moves(size_t& out_index, const size_t parent_idx,
											 const size_t king_index, const bool started_in_check, const tt::key key)
	{
		const rank rank = king_index / 8;
		const file file = king_index % 8;

		const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | king][to_index(rank, file)];

		const position& position = positions[parent_idx];
		const bitboards bitboards = get_bitboards_for(position);
		const bitboard opp_pieces = (color_to_move == white) ? bitboards.black : bitboards.white;

		const bitboard king_movemask = king_movemasks[king_index];

		bitboard captures = king_movemask & opp_pieces;
		while (captures)
		{
			size_t end_idx = get_next_bit(captures);
			captures = clear_next_bit(captures);

			append_if_legal<color_to_move, check_type::do_all, king, move_type::capture>(
				out_index, parent_idx, end_idx, started_in_check, incremental_key,
				parent_idx, rank, file, end_idx / 8, end_idx % 8);
		}

		if constexpr (gen_moves != gen_moves::all) return;

		bitboard non_captures = king_movemask & bitboards.empty;
		while (non_captures)
		{
			size_t end_idx = get_next_bit(non_captures);
			non_captures = clear_next_bit(non_captures);

			append_if_legal<color_to_move, check_type::do_all, king>(
				out_index, parent_idx, end_idx, started_in_check, incremental_key,
				parent_idx, rank, file, end_idx / 8, end_idx % 8);
		}

		if (started_in_check) return; // A king cannot castle out of check.

		const board& board = boards[parent_idx];

		const bool can_castle_ks = (color_to_move == white) ? board.white_can_castle_ks() : board.black_can_castle_ks();

		constexpr uint32_t ks_pieces_mask = ((color_to_move | rook) << 24) + (empty << 16) + (empty << 8) + (color_to_move | king);
		constexpr size_t king_start_idx = (color_to_move == white) ? 60 : 4;
		const uint32_t ks_pieces = *(uint32_t*)(&position[king_start_idx]);

		if (can_castle_ks && ks_pieces == ks_pieces_mask)
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

		const bool can_castle_qs = (color_to_move == white) ? board.white_can_castle_qs() : board.black_can_castle_qs();

		constexpr uint32_t qs_pieces_mask = (empty << 24) + (empty << 16) + (empty << 8) + (color_to_move | rook);
		constexpr size_t qs_rook_start_index = (color_to_move == white) ? 56 : 0;
		const uint32_t qs_pieces = *(uint32_t*)(&position[qs_rook_start_index]);

		if (can_castle_qs && qs_pieces == qs_pieces_mask)
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

	template <color_t moving_color, check_type check_type, piece_t moving_piece_type,
		move_type move_type = move_type::other, typename... board_args>
	force_inline_toggle void append_if_legal(size_t& out_index, const size_t parent_idx,
											 const size_t king_index, const bool started_in_check, const tt::key key,
											 board_args&&... args)
	{
		constexpr color_t child_color = other_color(moving_color);

		board& child_board = boards[out_index];
		child_board = board::template make_board<child_color, moving_piece_type, move_type>(std::forward<board_args>(args)...);

		position& child_position = positions[out_index];
		const position& parent_position = positions[parent_idx];
		make_move<moving_color, moving_piece_type, move_type>(child_position, parent_position, child_board);

		// If the king is in check, return early. We could be in check if:
		// - We are trying to move our king, or
		// - we started in check, or
		// - the moving piece shared a rank, file, or diagonal with the king.
		// If none of these are the case (most of the time), we cannot possibly be in check, and
		// we can skip the expensive call to is_king_in_check().
		if (moving_piece_type == king ||
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

	template<color_t attacker_color>
	inline_toggle bool pawn_is_attacking(const rank attacker_rank, const file attacker_file,
										 const rank target_rank, const file target_file)
	{
		if (diff(attacker_file, target_file) != 1) return false;

		if constexpr (attacker_color == white)
			return target_rank == attacker_rank - 1;
		else
			return target_rank == attacker_rank + 1;
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

	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void generate_child_boards(size_t& end_idx, const size_t parent_idx,
												   const size_t king_index, const bool started_in_check,
												   const tt::key key)
	{
		find_pawn_moves<color_to_move, gen_moves, check_type>(
			end_idx, parent_idx, king_index, started_in_check, key);
		find_moves_for<color_to_move | knight>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_knight_moves<color_to_move, gen_moves, check_type>);
		find_moves_for<color_to_move | bishop>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_bishop_moves<color_to_move, gen_moves, check_type>);
		find_moves_for<color_to_move | rook>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_rook_moves<color_to_move, gen_moves, check_type>);
		find_moves_for<color_to_move | queen>(
			end_idx, parent_idx, king_index, started_in_check, key, &find_queen_moves<color_to_move, gen_moves, check_type>);
		find_king_moves<color_to_move, gen_moves, check_type>(
			end_idx, parent_idx, king_index, started_in_check, key);
	}

	template<color_t color_to_move, gen_moves gen_moves, bool perft>
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

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is known to be attacking the king.
		// - When generating non-king moves, never check for a king.
		// - When generating king moves, do all checks.

		const size_t king_index = find_king_index<color_to_move>(parent_position);

		const piece last_moved_piece = parent_board.moved_piece_without_color();
		const rank last_moved_end_rank = parent_board.get_end_rank();
		const file last_moved_end_file = parent_board.get_end_file();

		const size_t begin_idx = first_child_index(parent_idx);
		size_t end_idx = begin_idx;
		bool started_in_check = false;

		if (last_moved_piece.is_pawn() && pawn_is_attacking<opp_color>(
			last_moved_end_rank, last_moved_end_file, king_index / 8, king_index % 8))
		{
			started_in_check = true;
			generate_child_boards<color_to_move, gen_moves, check_type::do_pawn_checks>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}
		else if (last_moved_piece.is_knight() && knight_is_attacking(
			last_moved_end_rank, last_moved_end_file, king_index / 8, king_index % 8))
		{
			started_in_check = true;
			generate_child_boards<color_to_move, gen_moves, check_type::do_knight_checks>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}
		else // the nominal path
		{
			started_in_check = is_king_in_check<color_to_move, check_type::skip_pawn_and_knight_checks>(
				parent_position, king_index / 8, king_index % 8);
			generate_child_boards<color_to_move, gen_moves, check_type::skip_pawn_and_knight_checks>(
				end_idx, parent_idx, king_index, started_in_check, key);
		}

		// During quiescence, a "terminal" position is just a position without available captures.
		// During perft, we don't care about terminal evaluations.
		if constexpr (gen_moves == gen_moves::all && !perft)
		{
			// If there are no legal moves, record the result.
			if (end_idx == begin_idx)
			{
				if (started_in_check) // checkmate
					boards[parent_idx].set_eval((color_to_move == white) ? -eval::mate : eval::mate);
				else // stalemate
					boards[parent_idx].set_eval(0);
			}
		}

		return end_idx;
	}

	template size_t generate_child_boards<white, gen_moves::all>(const size_t);
	template size_t generate_child_boards<white, gen_moves::all, true>(const size_t);
	template size_t generate_child_boards<white, gen_moves::captures>(const size_t);

	template size_t generate_child_boards<black, gen_moves::all>(const size_t);
	template size_t generate_child_boards<black, gen_moves::all, true>(const size_t);
	template size_t generate_child_boards<black, gen_moves::captures>(const size_t);
}
