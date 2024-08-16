
#include "bitboard.hpp"
#include "config.hpp"
#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	template <color moving_color, bool quiescing, bool perft, check_type check_type, bool in_check, piece piece,
	    move_type move_type = move_type::other, chess::piece promoted_piece = empty>
	force_inline_toggle static void append_if_legal(size_t& end_idx, const size_t parent_idx,
	    const bitboards& bitboards, const bitboard blockers, const size_t king_idx,

	    const tt_key key, const bitboard from, const bitboard to)
	{
		board& child_board = boards[end_idx];
		const board& parent_board = boards[parent_idx];
		chess::piece captured_piece;
		child_board.copy_make_bitboards<moving_color, quiescing, perft, piece, move_type, promoted_piece>(
		    parent_board, from, to, captured_piece);

		// If this move would leave us in check, return early. A move might leave us in check if:
		// - We started in check, or
		// - the moving piece is a king, or
		// - the move is an en passant capture, or
		// - the moving piece was a blocker (ie, had line of sight to the king).
		// If none of these are the case (most of the time), skip is_king_in_check().
		if (in_check || piece == king || move_type == move_type::en_passant_capture || from & blockers)
		{
			if (is_king_in_check<moving_color, check_type>(child_board.get_bitboards(), king_idx)) return;
		}

		child_board.copy_make_board<moving_color, quiescing, perft, piece, move_type, promoted_piece>(
		    parent_board, key, from, to, captured_piece);

		++end_idx;

		if constexpr (config::verify_key_and_eval) child_board.verify_key_and_eval(other_color(moving_color));
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check>
	force_inline_toggle static void find_pawn_moves(size_t& end_idx, const size_t parent_idx,
	    const bitboards& bitboards, const bitboard blockers, const size_t king_idx, const tt_key key)
	{
		constexpr bitboard promotion_start_file = (moving_color == white) ? rank_7 : rank_2;

		const bitboard pawns = bitboards.get<moving_color, pawn>();

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::captures)
		{
			const bitboard opp_pieces = bitboards.get<other_color(moving_color)>();

			bitboard capture_to_lower_file =
			    pawns & pawn_capture_lower_file & ((moving_color == white) ? opp_pieces << 9 : opp_pieces >> 7);
			bitboard capture_to_lower_file_promotion = capture_to_lower_file & promotion_start_file;
			capture_to_lower_file ^= capture_to_lower_file_promotion;

			while (capture_to_lower_file_promotion)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file_promotion);
				const bitboard start = get_next_bit(capture_to_lower_file_promotion);
				capture_to_lower_file_promotion = clear_next_bit(capture_to_lower_file_promotion);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, queen>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, knight>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, rook>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, bishop>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7);
			}

			while (capture_to_lower_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file);
				const bitboard start = get_next_bit(capture_to_lower_file);
				capture_to_lower_file = clear_next_bit(capture_to_lower_file);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture>(end_idx,
				    parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7);
			}

			bitboard capture_to_higher_file =
			    pawns & pawn_capture_higher_file & ((moving_color == white) ? opp_pieces << 7 : opp_pieces >> 9);
			bitboard capture_to_higher_file_promotion = capture_to_higher_file & promotion_start_file;
			capture_to_higher_file ^= capture_to_higher_file_promotion;

			while (capture_to_higher_file_promotion)
			{
				const size_t start_idx = get_next_bit_index(capture_to_higher_file_promotion);
				const bitboard start = get_next_bit(capture_to_higher_file_promotion);
				capture_to_higher_file_promotion = clear_next_bit(capture_to_higher_file_promotion);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, queen>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, knight>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, rook>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, bishop>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9);
			}

			while (capture_to_higher_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_higher_file);
				const bitboard start = get_next_bit(capture_to_higher_file);
				capture_to_higher_file = clear_next_bit(capture_to_higher_file);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture>(end_idx,
				    parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9);
			}

			const file ep_file = boards[parent_idx].get_en_passant_file();

			if (ep_file != no_ep_file)
			{
				constexpr bitboard ep_capture_start_rank = (moving_color == white) ? rank_5 : rank_4;

				bitboard ep_capturers =
				    pawns & ep_capture_start_rank & (ep_capture_mask << (ep_file + (moving_color == white ? 0 : 8)));

				while (ep_capturers) // 0-2
				{
					size_t start_idx = get_next_bit_index(ep_capturers);
					const bitboard start = get_next_bit(ep_capturers);
					ep_capturers = clear_next_bit(ep_capturers);

					const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

					append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn,
					    move_type::en_passant_capture>(end_idx, parent_idx, bitboards, blockers, king_idx,
					    incremental_key, start, (1ull << ((moving_color == white) ? 16 : 40)) << ep_file);
				}
			}
		}

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::noncaptures)
		{
			const bitboard empty_squares = bitboards.empty();

			bitboard move_one_square = pawns & ((moving_color == white) ? empty_squares << 8 : empty_squares >> 8);
			bitboard noncapture_promotions = move_one_square & promotion_start_file;
			move_one_square ^= noncapture_promotions;

			while (noncapture_promotions)
			{
				const size_t start_idx = get_next_bit_index(noncapture_promotions);
				const bitboard start = get_next_bit(noncapture_promotions);
				noncapture_promotions = clear_next_bit(noncapture_promotions);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, queen>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, knight>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, rook>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, bishop>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8);
			}

			bitboard move_two_squares = move_one_square & (moving_color == white ? rank_2 : rank_7) &
			                            ((moving_color == white) ? empty_squares << 16 : empty_squares >> 16);

			while (move_two_squares)
			{
				const size_t start_idx = get_next_bit_index(move_two_squares);
				const bitboard start = get_next_bit(move_two_squares);
				move_two_squares = clear_next_bit(move_two_squares);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn,
				    move_type::pawn_two_squares>(end_idx, parent_idx, bitboards, blockers, king_idx, incremental_key,
				    start, (moving_color == white) ? start >> 16 : start << 16);
			}

			while (move_one_square)
			{
				const size_t start_idx = get_next_bit_index(move_one_square);
				const bitboard start = get_next_bit(move_one_square);
				move_one_square = clear_next_bit(move_one_square);

				const tt_key incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn>(end_idx, parent_idx,
				    bitboards, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8);
			}
		}
	}
	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check>
	force_inline_toggle static void find_knight_moves(size_t& end_idx, const size_t parent_idx,
	    const bitboards& bitboards, const bitboard blockers, const size_t knight_idx, const size_t king_idx,
	    const tt_key key)
	{
		const bitboard moves = knight_attack_masks[knight_idx];

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::captures)
		{
			bitboard captures = moves & bitboards.get<other_color(moving_color)>();
			while (captures)
			{
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, knight, move_type::capture>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, key, 1ull << knight_idx, end);
			}
		}

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::noncaptures)
		{
			bitboard noncaptures = moves & bitboards.empty();
			while (noncaptures)
			{
				const bitboard end = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, knight>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, key, 1ull << knight_idx, end);
			}
		}
	}
	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check,
	    piece piece>
	force_inline_toggle static void find_slider_moves(size_t& end_idx, const size_t parent_idx,
	    const bitboards& bitboards, const bitboard blockers, const size_t slider_idx, const size_t king_idx,
	    const tt_key key)
	{
		static_assert(piece == bishop || piece == rook || piece == queen);

		const bitboard moves = get_slider_moves<piece>(bitboards, slider_idx);

		if constexpr (gen_moves == gen_moves::captures || gen_moves == gen_moves::all)
		{
			bitboard captures = moves & bitboards.get<other_color(moving_color)>();
			while (captures)
			{
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, piece, move_type::capture>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, key, 1ull << slider_idx, end);
			}
		}

		if constexpr (gen_moves == gen_moves::noncaptures || gen_moves == gen_moves::all)
		{
			bitboard noncaptures = moves & bitboards.empty();
			while (noncaptures)
			{
				const bitboard end = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, piece>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, key, 1ull << slider_idx, end);
			}
		}
	}
	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check>
	force_inline_toggle static void find_king_moves(size_t& end_idx, const size_t parent_idx,
	    const bitboards& bitboards, const bitboard blockers, const size_t king_idx, const tt_key key)
	{
		const tt_key incremental_key = key ^ piece_square_key<moving_color, king>(king_idx);

		const bitboard moves = king_attack_masks[king_idx];

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::captures)
		{
			bitboard captures = moves & bitboards.get<other_color(moving_color)>();
			while (captures)
			{
				size_t new_king_idx = get_next_bit_index(captures);
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<moving_color, quiescing, perft, check_type::all, false, king, move_type::capture>(
				    end_idx, parent_idx, bitboards, blockers, new_king_idx, incremental_key, 1ull << king_idx, end);
			}
		}

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::noncaptures)
		{
			bitboard non_captures = moves & bitboards.empty();
			while (non_captures)
			{
				size_t new_king_idx = get_next_bit_index(non_captures);
				const bitboard end = get_next_bit(non_captures);
				non_captures = clear_next_bit(non_captures);

				append_if_legal<moving_color, quiescing, perft, check_type::all, false, king>(
				    end_idx, parent_idx, bitboards, blockers, new_king_idx, incremental_key, 1ull << king_idx, end);
			}

			if constexpr (in_check) return; // A king cannot castle out of check.

			const board& board = boards[parent_idx];
			constexpr size_t king_start_idx = (moving_color == white) ? 60 : 4;

			const bool can_castle_ks =
			    (moving_color == white) ? board.white_can_castle_ks() : board.black_can_castle_ks();
			constexpr bitboard ks_castle_bits = 0b01100000uz << ((moving_color == white) ? 56 : 0);
			if (can_castle_ks && (bitboards.occupied() & ks_castle_bits) == 0u)
			{
				// Check that the king would not be moving through check.
				if (!is_king_in_check<moving_color, check_type::all>(bitboards, king_start_idx + 1))
				{
					append_if_legal<moving_color, quiescing, perft, check_type::all, false, king,
					    move_type::castle_kingside>(end_idx, parent_idx, bitboards, blockers, king_start_idx + 2,
					    incremental_key, 1ull << king_start_idx, 1ull << (king_start_idx + 2));
				}
			}

			const bool can_castle_qs =
			    (moving_color == white) ? board.white_can_castle_qs() : board.black_can_castle_qs();
			constexpr bitboard qs_castle_bits = 0b00001110uz << ((moving_color == white) ? 56 : 0);
			if (can_castle_qs && (bitboards.occupied() & qs_castle_bits) == 0u)
			{
				if (!is_king_in_check<moving_color, check_type::all>(bitboards, king_start_idx - 1))
				{
					append_if_legal<moving_color, quiescing, perft, check_type::all, false, king,
					    move_type::castle_queenside>(end_idx, parent_idx, bitboards, blockers, king_start_idx - 2,
					    incremental_key, 1ull << king_start_idx, 1ull << (king_start_idx - 2));
				}
			}
		}
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check,
	    piece piece>
	force_inline_toggle static void find_moves_for(size_t& end_idx, const size_t parent_idx, const bitboards& bitboards,
	    const bitboard blockers, const size_t king_idx, const tt_key key)
	{
		static_assert(piece == knight || piece == bishop || piece == rook || piece == queen);

		bitboard pieces = bitboards.get<moving_color, piece>();

		while (pieces)
		{
			const size_t piece_idx = get_next_bit_index(pieces);
			pieces = clear_next_bit(pieces);

			// XOR the key for the leaving piece once for all of its moves.
			const tt_key incremental_key = key ^ piece_square_key<moving_color, piece>(piece_idx);

			if constexpr (piece == knight)
			{
				find_knight_moves<moving_color, gen_moves, quiescing, perft, check_type, in_check>(
				    end_idx, parent_idx, bitboards, blockers, piece_idx, king_idx, incremental_key);
			}
			else
			{
				find_slider_moves<moving_color, gen_moves, quiescing, perft, check_type, in_check, piece>(
				    end_idx, parent_idx, bitboards, blockers, piece_idx, king_idx, incremental_key);
			}
		}
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check>
	force_inline_toggle static void find_moves(size_t& end_idx, const size_t parent_idx, const bitboards& bitboards,
	    const bitboard blockers, const size_t king_idx, const tt_key key)
	{
		find_pawn_moves<moving_color, gen_moves, quiescing, perft, check_type, in_check>(
		    end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, knight>(
		    end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, bishop>(
		    end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, rook>(
		    end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, queen>(
		    end_idx, parent_idx, bitboards, blockers, king_idx, key);
		find_king_moves<moving_color, gen_moves, quiescing, perft, check_type, in_check>(
		    end_idx, parent_idx, bitboards, blockers, king_idx, key);
	}

	size_t nq_nodes{0};
	size_t q_nodes{0};

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft>
	size_t generate_child_boards(const size_t parent_idx)
	{
		const board& parent_board = boards[parent_idx];
		tt_key key = parent_board.get_key() ^ black_to_move_key();

		const file parent_ep_file = parent_board.get_en_passant_file();
		if (parent_ep_file != no_ep_file)
		{
			key ^= en_passant_key(parent_ep_file);
		}

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is known to be attacking the king.
		// - When generating non-king moves, never check for a king.
		// - When generating king moves, do all checks.

		const piece last_moved_piece = parent_board.get_moved_piece();
		constexpr color opp_color = other_color(moving_color);
		const bitboards& bitboards = parent_board.get_bitboards();
		const size_t king_idx = get_next_bit_index(bitboards.get<moving_color, king>());
		size_t end_idx = first_child_index(parent_idx);

		if (last_moved_piece == pawn && square_is_attacked_by_pawn<opp_color>(bitboards, king_idx))
		{
			find_moves<moving_color, gen_moves, quiescing, perft, check_type::pawn, true>(
			    end_idx, parent_idx, bitboards, bitboard{}, king_idx, key);
		}
		else if (last_moved_piece == knight && square_is_attacked_by_knight<opp_color>(bitboards, king_idx))
		{
			find_moves<moving_color, gen_moves, quiescing, perft, check_type::knight, true>(
			    end_idx, parent_idx, bitboards, bitboard{}, king_idx, key);
		}
		else // the nominal path
		{
			if (is_king_in_check<moving_color, check_type::sliders>(bitboards, king_idx))
			{
				find_moves<moving_color, gen_moves, quiescing, perft, check_type::sliders, true>(
				    end_idx, parent_idx, bitboards, bitboard{}, king_idx, key);
			}
			else
			{
				const bitboard blockers = get_blockers<moving_color>(bitboards);
				find_moves<moving_color, gen_moves, quiescing, perft, check_type::sliders, false>(
				    end_idx, parent_idx, bitboards, blockers, king_idx, key);
			}
		}

		return end_idx;
	}

	template size_t generate_child_boards<white, gen_moves::all>(const size_t);
	template size_t generate_child_boards<white, gen_moves::captures>(const size_t);
	template size_t generate_child_boards<white, gen_moves::noncaptures>(const size_t);

	template size_t generate_child_boards<black, gen_moves::all>(const size_t);
	template size_t generate_child_boards<black, gen_moves::captures>(const size_t);
	template size_t generate_child_boards<black, gen_moves::noncaptures>(const size_t);

	// For quiescence search:
	template size_t generate_child_boards<white, gen_moves::captures, true>(const size_t);
	template size_t generate_child_boards<black, gen_moves::captures, true>(const size_t);

	// For perft:
	template size_t generate_child_boards<white, gen_moves::all, false, true>(const size_t);
	template size_t generate_child_boards<black, gen_moves::all, false, true>(const size_t);
}
