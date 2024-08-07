
#include "bitboard.hpp"
#include "config.hpp"
#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, bool started_in_check>
	force_inline_toggle void find_pawn_moves(size_t& out_idx, const size_t parent_idx,
											 const bitboards& bitboards, const bitboard blockers,
											 const size_t king_idx, const tt::key key)
	{
		constexpr bitboard promotion_start_file = (color_to_move == white) ? rank_7 : rank_2;

		const bitboard pawns = bitboards.get<color_to_move, pawn>();

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::captures)
		{
			const bitboard opp_pieces = bitboards.get<other_color(color_to_move)>();

			bitboard capture_to_lower_file = pawns & pawn_capture_lower_file
				& ((color_to_move == white) ? opp_pieces << 9 : opp_pieces >> 7);
			bitboard capture_to_lower_file_promotion = capture_to_lower_file & promotion_start_file;
			capture_to_lower_file ^= capture_to_lower_file_promotion;

			while (capture_to_lower_file_promotion)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file_promotion);
				const bitboard start = get_next_bit(capture_to_lower_file_promotion);
				capture_to_lower_file_promotion = clear_next_bit(capture_to_lower_file_promotion);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, queen>(
					out_idx, parent_idx, bitboards, blockers, king_idx,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, knight>(
					out_idx, parent_idx, bitboards, blockers, king_idx,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, rook>(
					out_idx, parent_idx, bitboards, blockers, king_idx,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, bishop>(
					out_idx, parent_idx, bitboards, blockers, king_idx,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
			}

			while (capture_to_lower_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file);
				const bitboard start = get_next_bit(capture_to_lower_file);
				capture_to_lower_file = clear_next_bit(capture_to_lower_file);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture>(
					out_idx, parent_idx, bitboards, blockers, king_idx,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
			}

			bitboard capture_to_higher_file = pawns & pawn_capture_higher_file
				& ((color_to_move == white) ? opp_pieces << 7 : opp_pieces >> 9);
			bitboard capture_to_higher_file_promotion = capture_to_higher_file & promotion_start_file;
			capture_to_higher_file ^= capture_to_higher_file_promotion;

			while (capture_to_higher_file_promotion)
			{
				const size_t start_idx = get_next_bit_index(capture_to_higher_file_promotion);
				const bitboard start = get_next_bit(capture_to_higher_file_promotion);
				capture_to_higher_file_promotion = clear_next_bit(capture_to_higher_file_promotion);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, queen>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 7 : start << 9);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, knight>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 7 : start << 9);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, rook>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 7 : start << 9);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture, bishop>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 7 : start << 9);
			}

			while (capture_to_higher_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_higher_file);
				const bitboard start = get_next_bit(capture_to_higher_file);
				capture_to_higher_file = clear_next_bit(capture_to_higher_file);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::capture>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 7 : start << 9);
			}

			const file ep_capture_file = boards[parent_idx].get_en_passant_file();

			if (ep_capture_file != empty)
			{
				constexpr bitboard ep_capture_start_rank = (color_to_move == white) ? rank_5 : rank_4;
				bitboard ep_capturers = pawns & ep_capture_start_rank & (ep_capture_mask << (ep_capture_file + (color_to_move == white ? 0 : 8)));

				while (ep_capturers) // 0-2
				{
					size_t start_idx = get_next_bit_index(ep_capturers);
					const bitboard start = get_next_bit(ep_capturers);
					ep_capturers = clear_next_bit(ep_capturers);

					const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

					append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::en_passant_capture>(
						out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
						start, (1ull << ((color_to_move == white) ? 16 : 40)) << ep_capture_file);
				}
			}
		}

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::noncaptures)
		{
			const bitboard empty_squares = bitboards.empty();
			bitboard move_one_square = pawns & ((color_to_move == white) ? empty_squares << 8 : empty_squares >> 8);

			bitboard noncapture_promotions = move_one_square & promotion_start_file;
			move_one_square ^= noncapture_promotions;

			while (noncapture_promotions)
			{
				const size_t start_idx = get_next_bit_index(noncapture_promotions);
				const bitboard start = get_next_bit(noncapture_promotions);
				noncapture_promotions = clear_next_bit(noncapture_promotions);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::other, queen>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 8 : start << 8);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::other, knight>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 8 : start << 8);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::other, rook>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 8 : start << 8);
				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::other, bishop>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 8 : start << 8);
			}

			bitboard move_two_squares = move_one_square
				& (color_to_move == white ? rank_2 : rank_7)
				& ((color_to_move == white) ? empty_squares << 16 : empty_squares >> 16);

			while (move_two_squares)
			{
				const size_t start_idx = get_next_bit_index(move_two_squares);
				const bitboard start = get_next_bit(move_two_squares);
				move_two_squares = clear_next_bit(move_two_squares);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn, move_type::pawn_two_squares>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 16 : start << 16);
			}

			while (move_one_square)
			{
				const size_t start_idx = get_next_bit_index(move_one_square);
				const bitboard start = get_next_bit(move_one_square);
				move_one_square = clear_next_bit(move_one_square);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, started_in_check, pawn>(
					out_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
					start, (color_to_move == white) ? start >> 8 : start << 8);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, bool started_in_check>
	force_inline_toggle void find_knight_moves(size_t& out_idx, const size_t parent_idx,
											   const bitboards& bitboards, const bitboard blockers,
											   const size_t knight_idx,
											   const size_t king_idx, const tt::key key)
	{
		const bitboard moves = knight_attack_masks[knight_idx];

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::captures)
		{
			bitboard captures = moves & bitboards.get<other_color(color_to_move)>();
			while (captures)
			{
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<color_to_move, check_type, started_in_check, knight, move_type::capture>(
					out_idx, parent_idx, bitboards, blockers, king_idx, key,
					1ull << knight_idx, end);
			}
		}

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::noncaptures)
		{
			bitboard noncaptures = moves & bitboards.empty();
			while (noncaptures)
			{
				const bitboard end = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<color_to_move, check_type, started_in_check, knight>(
					out_idx, parent_idx, bitboards, blockers, king_idx, key,
					1ull << knight_idx, end);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, bool started_in_check, piece_t piece_type>
	force_inline_toggle void find_slider_moves(size_t& out_idx, const size_t parent_idx,
											   const bitboards& bitboards, const bitboard blockers,
											   const size_t slider_idx,
											   const size_t king_idx, const tt::key key)
	{
		static_assert(piece_type == bishop || piece_type == rook || piece_type == queen);

		const bitboard moves = get_slider_moves<piece_type>(bitboards, slider_idx);

		if constexpr (gen_moves == gen_moves::captures ||
					  gen_moves == gen_moves::all)
		{
			bitboard captures = moves & bitboards.get<other_color(color_to_move)>();
			while (captures)
			{
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<color_to_move, check_type, started_in_check, piece_type, move_type::capture>(
					out_idx, parent_idx, bitboards, blockers, king_idx, key,
					1ull << slider_idx, end);
			}
		}

		if constexpr (gen_moves == gen_moves::noncaptures ||
					  gen_moves == gen_moves::all)
		{
			bitboard noncaptures = moves & bitboards.empty();
			while (noncaptures)
			{
				const bitboard end = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<color_to_move, check_type, started_in_check, piece_type>(
					out_idx, parent_idx, bitboards, blockers, king_idx, key,
					1ull << slider_idx, end);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, bool started_in_check>
	force_inline_toggle void find_king_moves(size_t& out_idx, const size_t parent_idx,
											 const bitboards& bitboards, const bitboard blockers,
											 const size_t king_idx, const tt::key key)
	{
		const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | king][king_idx];

		const bitboard moves = king_attack_masks[king_idx];

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::captures)
		{
			bitboard captures = moves & bitboards.get<other_color(color_to_move)>();
			while (captures)
			{
				size_t end_idx = get_next_bit_index(captures);
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<color_to_move, check_type::all, false, king, move_type::capture>(
					out_idx, parent_idx, bitboards, blockers, end_idx, incremental_key,
					1ull << king_idx, end);
			}
		}

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::noncaptures)
		{
			bitboard non_captures = moves & bitboards.empty();
			while (non_captures)
			{
				size_t end_idx = get_next_bit_index(non_captures);
				const bitboard end = get_next_bit(non_captures);
				non_captures = clear_next_bit(non_captures);

				append_if_legal<color_to_move, check_type::all, false, king>(
					out_idx, parent_idx, bitboards, blockers, end_idx, incremental_key,
					1ull << king_idx, end);
			}

			if constexpr (started_in_check) return; // A king cannot castle out of check.

			const board& board = boards[parent_idx];
			constexpr size_t king_start_idx = (color_to_move == white) ? 60 : 4;

			const bool can_castle_ks = (color_to_move == white) ? board.white_can_castle_ks() : board.black_can_castle_ks();
			constexpr bitboard ks_castle_bits = 0b01100000uz << ((color_to_move == white) ? 56 : 0);
			if (can_castle_ks && (bitboards.occupied() & ks_castle_bits) == 0)
			{
				// Check that the king would not be moving through check.
				if (!is_king_in_check<color_to_move, check_type::all>(bitboards, king_start_idx + 1))
				{
					append_if_legal<color_to_move, check_type::all, false, king, move_type::castle_kingside>(
						out_idx, parent_idx, bitboards, blockers, king_start_idx + 2, incremental_key,
						1ull << king_start_idx, 1ull << (king_start_idx + 2));
				}
			}

			const bool can_castle_qs = (color_to_move == white) ? board.white_can_castle_qs() : board.black_can_castle_qs();
			constexpr bitboard qs_castle_bits = 0b00001110uz << ((color_to_move == white) ? 56 : 0);
			if (can_castle_qs && (bitboards.occupied() & qs_castle_bits) == 0)
			{
				if (!is_king_in_check<color_to_move, check_type::all>(bitboards, king_start_idx - 1))
				{
					append_if_legal<color_to_move, check_type::all, false, king, move_type::castle_queenside>(
						out_idx, parent_idx, bitboards, blockers, king_start_idx - 2, incremental_key,
						1ull << king_start_idx, 1ull << (king_start_idx - 2));
				}
			}
		}
	}

	template <color_t moving_color, check_type check_type, bool started_in_check, piece_t moving_piece_type,
		move_type move_type = move_type::other, piece_t promotion_type = empty, typename... board_args>
	force_inline_toggle void append_if_legal(size_t& out_idx, const size_t parent_idx,
											 const bitboards& bitboards, const bitboard blockers,
											 const size_t king_idx, const tt::key key,
											 const bitboard start, const bitboard end)
	{
		constexpr color_t child_color = other_color(moving_color);

		board& child_board = boards[out_idx];
		piece_t captured_piece;
		child_board.update_bitboards<child_color, moving_piece_type, move_type, promotion_type>(parent_idx, start, end, captured_piece);

		// If this move would leave us in check, return early. A move might leave us in check if:
		// - We started in check, or
		// - the moving piece is a king, or
		// - the move is an en passant capture, or
		// - the moving piece was a blocker (ie, had line of sight to the king).
		// If none of these are the case (most of the time), skip is_king_in_check().
		if (started_in_check || moving_piece_type == king || move_type == move_type::en_passant_capture || start & blockers)
		{
			if (is_king_in_check<moving_color, check_type>(child_board.get_bitboards(), king_idx)) return;
		}

		board::make_board<child_color, moving_piece_type, move_type, promotion_type>(out_idx, parent_idx, start, end);

		child_board.update_key_and_eval<moving_color, moving_piece_type, move_type, promotion_type>(
			boards[parent_idx], key, start, end, captured_piece);

		++out_idx;

		if constexpr (config::verify_key_and_eval)
			child_board.verify_key_and_eval(child_color);
	}

	template<color_t color_to_move, piece_t piece_type, gen_moves gen_moves, check_type check_type, bool started_in_check>
	force_inline_toggle void find_moves_for(size_t& out_idx, const size_t parent_idx,
											const bitboards& bitboards, const bitboard blockers,
											const size_t king_idx, const tt::key key)
	{
		static_assert(piece_type == knight ||
					  piece_type == bishop ||
					  piece_type == rook ||
					  piece_type == queen);

		bitboard pieces = bitboards.get<color_to_move, piece_type>();

		while (pieces)
		{
			const size_t piece_idx = get_next_bit_index(pieces);
			pieces = clear_next_bit(pieces);

			// XOR the key for the leaving piece once for all of its moves
			const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | piece_type][piece_idx];

			if constexpr (piece_type == knight)
			{
				find_knight_moves<color_to_move, gen_moves, check_type, started_in_check>(
					out_idx, parent_idx, bitboards, blockers, piece_idx, king_idx, incremental_key);
			}
			else
			{
				find_slider_moves<color_to_move, gen_moves, check_type, started_in_check, piece_type>(
					out_idx, parent_idx, bitboards, blockers, piece_idx, king_idx, incremental_key);
			}
		}
	}

	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, bool started_in_check>
	force_inline_toggle void generate_child_boards(size_t& end_idx, const size_t parent_idx,
												   const bitboards& bitboards, const bitboard blockers,
												   const size_t king_idx, const tt::key key)
	{
		find_pawn_moves<color_to_move, gen_moves, check_type, started_in_check>(
			end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<color_to_move, knight, gen_moves, check_type, started_in_check>(
			end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<color_to_move, bishop, gen_moves, check_type, started_in_check>(
			end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<color_to_move, rook, gen_moves, check_type, started_in_check>(
			end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<color_to_move, queen, gen_moves, check_type, started_in_check>(
			end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_king_moves<color_to_move, gen_moves, check_type, started_in_check>(
			end_idx, parent_idx, bitboards, blockers, king_idx, key);
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

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is known to be attacking the king.
		// - When generating non-king moves, never check for a king.
		// - When generating king moves, do all checks.

		const piece last_moved_piece = parent_board.moved_piece_without_color();
		constexpr color_t opp_color = other_color(color_to_move);
		const bitboards& bitboards = parent_board.get_bitboards();
		const size_t king_idx = get_next_bit_index(bitboards.get<color_to_move, king>());
		size_t end_idx = first_child_index(parent_idx);

		if (last_moved_piece.is_pawn() &&
			square_is_attacked_by_pawn<opp_color>(bitboards, king_idx))
		{
			generate_child_boards<color_to_move, gen_moves, check_type::pawn, true>(
				end_idx, parent_idx, bitboards, bitboard{}, king_idx, key);
		}
		else if (last_moved_piece.is_knight() &&
				 square_is_attacked_by_knight<opp_color>(bitboards, king_idx))
		{
			generate_child_boards<color_to_move, gen_moves, check_type::knight, true>(
				end_idx, parent_idx, bitboards, bitboard{}, king_idx, key);
		}
		else // the nominal path
		{
			if (is_king_in_check<color_to_move, check_type::sliders>(bitboards, king_idx))
			{
				generate_child_boards<color_to_move, gen_moves, check_type::sliders, true>(
					end_idx, parent_idx, bitboards, bitboard{}, king_idx, key);
			}
			else
			{
				const bitboard blockers = get_blockers<color_to_move>(bitboards);
				generate_child_boards<color_to_move, gen_moves, check_type::sliders, false>(
					end_idx, parent_idx, bitboards, blockers, king_idx, key);
			}
		}

		return end_idx;
	}

	template size_t generate_child_boards<white, gen_moves::all>(const size_t);
	template size_t generate_child_boards<white, gen_moves::captures>(const size_t);
	template size_t generate_child_boards<white, gen_moves::noncaptures>(const size_t);
	template size_t generate_child_boards<white, gen_moves::all, true>(const size_t);

	template size_t generate_child_boards<black, gen_moves::all>(const size_t);
	template size_t generate_child_boards<black, gen_moves::captures>(const size_t);
	template size_t generate_child_boards<black, gen_moves::noncaptures>(const size_t);
	template size_t generate_child_boards<black, gen_moves::all, true>(const size_t);
}
