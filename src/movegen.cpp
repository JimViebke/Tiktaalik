
#include "bitboard.hpp"
#include "config.hpp"
#include "movegen.hpp"
#include "transposition_table.hpp"

namespace chess
{
	template <color moving_color, bool quiescing, bool perft, check_type check_type, bool started_in_check, piece piece,
	    move_type move_type = move_type::other, chess::piece promoted_piece = empty>
	force_inline_toggle static void append_if_legal(size_t& end_idx, const board& parent_board, const bitboard blockers,
	    const size_t king_start_idx, const tt_key key, const bitboard from, const bitboard to,
	    const move_info& move_info)
	{
		board& child_board = boards[end_idx];
		chess::piece captured_piece{};
		child_board.copy_make_bitboards<moving_color, perft, piece, move_type, promoted_piece>(
		    parent_board, from, to, captured_piece);

		// If this move would leave us in check, return early. A move might leave us in check if:
		// - We started in check, or
		// - the moving piece is a king, or
		// - the move is an en passant capture, or
		// - the moving piece was a blocker (ie, had line of sight to the king).
		// If none of these are the case (most of the time), skip check detection.
		if (started_in_check || piece == king || move_type == move_type::en_passant_capture || from & blockers)
		{
			const size_t king_idx = (piece != king) ? king_start_idx : get_next_bit_index(to);
			if (in_check<moving_color, check_type>(child_board, king_idx)) return;
		}

		child_board.copy_make_board<moving_color, quiescing, perft, piece, move_type, promoted_piece>(
		    parent_board, key, from, to, captured_piece, move_info);

		++end_idx;

		if constexpr (config::verify_key_phase_eval)
			child_board.verify_key_phase_eval<!quiescing>(other_color(moving_color));
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check>
	force_inline_toggle static void find_pawn_moves(size_t& end_idx, const board& parent_board, const bitboard blockers,
	    const size_t king_idx, const tt_key key, const move_info& move_info)
	{
		constexpr bitboard promotion_start_file = (moving_color == white) ? rank_7 : rank_2;

		const bitboards& parent_bbs = parent_board.get_bitboards();
		const bitboard pawns = parent_bbs.get<moving_color, pawn>();

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::captures)
		{
			const bitboard opp_pieces = parent_bbs.get<other_color(moving_color)>();

			bitboard capture_to_lower_file =
			    pawns & pawn_capture_lower_file & ((moving_color == white) ? opp_pieces << 9 : opp_pieces >> 7);
			bitboard capture_to_lower_file_promotion = capture_to_lower_file & promotion_start_file;
			capture_to_lower_file ^= capture_to_lower_file_promotion;

			while (capture_to_lower_file_promotion)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file_promotion);
				const bitboard start = get_next_bit(capture_to_lower_file_promotion);
				capture_to_lower_file_promotion = clear_next_bit(capture_to_lower_file_promotion);

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, queen>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, knight>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, rook>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, bishop>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7, move_info);
			}

			while (capture_to_lower_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file);
				const bitboard start = get_next_bit(capture_to_lower_file);
				capture_to_lower_file = clear_next_bit(capture_to_lower_file);

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture>(end_idx,
				    parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 9 : start << 7, move_info);
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

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, queen>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, knight>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, rook>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture, bishop>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9, move_info);
			}

			while (capture_to_higher_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_higher_file);
				const bitboard start = get_next_bit(capture_to_higher_file);
				capture_to_higher_file = clear_next_bit(capture_to_higher_file);

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::capture>(end_idx,
				    parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 7 : start << 9, move_info);
			}

			if (parent_board.can_capture_ep())
			{
				constexpr bitboard ep_capture_start_rank = (moving_color == white) ? rank_5 : rank_4;
				const file ep_file = parent_board.get_move().get_end_file();

				bitboard ep_capturers =
				    pawns & ep_capture_start_rank & (ep_capture_mask << (ep_file + (moving_color == white ? 0 : 8)));

				while (ep_capturers) // 0-2
				{
					size_t start_idx = get_next_bit_index(ep_capturers);
					const bitboard start = get_next_bit(ep_capturers);
					ep_capturers = clear_next_bit(ep_capturers);

					tt_key incremental_key{};
					if constexpr (!quiescing && !perft)
					{
						incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
					}

					append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn,
					    move_type::en_passant_capture>(end_idx, parent_board, blockers, king_idx, incremental_key,
					    start, (1ull << ((moving_color == white) ? 16 : 40)) << ep_file, move_info);
				}
			}
		}

		if constexpr (gen_moves == gen_moves::all || gen_moves == gen_moves::noncaptures)
		{
			const bitboard empty_squares = parent_bbs.empty();

			bitboard move_one_square = pawns & ((moving_color == white) ? empty_squares << 8 : empty_squares >> 8);
			bitboard noncapture_promotions = move_one_square & promotion_start_file;
			move_one_square ^= noncapture_promotions;

			while (noncapture_promotions)
			{
				const size_t start_idx = get_next_bit_index(noncapture_promotions);
				const bitboard start = get_next_bit(noncapture_promotions);
				noncapture_promotions = clear_next_bit(noncapture_promotions);

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, queen>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, knight>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, rook>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8, move_info);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn, move_type::other, bishop>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 8 : start << 8, move_info);
			}

			bitboard move_two_squares = move_one_square & (moving_color == white ? rank_2 : rank_7) &
			                            ((moving_color == white) ? empty_squares << 16 : empty_squares >> 16);

			while (move_two_squares)
			{
				const size_t start_idx = get_next_bit_index(move_two_squares);
				const bitboard start = get_next_bit(move_two_squares);
				move_two_squares = clear_next_bit(move_two_squares);

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn,
				    move_type::pawn_two_squares>(end_idx, parent_board, blockers, king_idx, incremental_key, start,
				    (moving_color == white) ? start >> 16 : start << 16, move_info);
			}

			while (move_one_square)
			{
				const size_t start_idx = get_next_bit_index(move_one_square);
				const bitboard start = get_next_bit(move_one_square);
				move_one_square = clear_next_bit(move_one_square);

				tt_key incremental_key{};
				if constexpr (!quiescing && !perft)
				{
					incremental_key = key ^ piece_square_key<moving_color, pawn>(start_idx);
				}

				append_if_legal<moving_color, quiescing, perft, check_type, in_check, pawn>(end_idx, parent_board,
				    blockers, king_idx, incremental_key, start, (moving_color == white) ? start >> 8 : start << 8,
				    move_info);
			}
		}
	}

	template <color moving_color, bool perft>
	force_inline_toggle static void find_castle_moves(
	    size_t& end_idx, const board& parent_board, tt_key key, const move_info& move_info)
	{
		constexpr size_t king_start_idx = (moving_color == white) ? 60 : 4;

		if constexpr (!perft)
		{
			key ^= piece_square_key<moving_color, king>(king_start_idx);
		}

		const bitboards& parent_bbs = parent_board.get_bitboards();

		const bool can_castle_ks =
		    (moving_color == white) ? parent_board.white_can_castle_ks() : parent_board.black_can_castle_ks();
		constexpr bitboard ks_castle_bits = 0b01100000uz << ((moving_color == white) ? 56 : 0);

		if (can_castle_ks && (parent_bbs.occupied() & ks_castle_bits) == 0u)
		{
			// Check that the king would not be moving through check.
			if (!in_check<moving_color>(parent_board, king_start_idx + 1))
			{
				append_if_legal<moving_color, false, perft, check_type::all, false, king, move_type::castle_kingside>(
				    end_idx, parent_board, bitboard{}, king_start_idx + 2, key, 1ull << king_start_idx,
				    1ull << (king_start_idx + 2), move_info);
			}
		}

		const bool can_castle_qs =
		    (moving_color == white) ? parent_board.white_can_castle_qs() : parent_board.black_can_castle_qs();
		constexpr bitboard qs_castle_bits = 0b00001110uz << ((moving_color == white) ? 56 : 0);

		if (can_castle_qs && (parent_bbs.occupied() & qs_castle_bits) == 0u)
		{
			if (!in_check<moving_color>(parent_board, king_start_idx - 1))
			{
				append_if_legal<moving_color, false, perft, check_type::all, false, king, move_type::castle_queenside>(
				    end_idx, parent_board, bitboard{}, king_start_idx - 2, key, 1ull << king_start_idx,
				    1ull << (king_start_idx - 2), move_info);
			}
		}
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check,
	    piece piece>
	force_inline_toggle static void find_moves_for(size_t& end_idx, const board& parent_board, const bitboard blockers,
	    const size_t king_idx, const tt_key key, const move_info& move_info)
	{
		static_assert(piece != pawn);

		const bitboards& parent_bbs = parent_board.get_bitboards();
		bitboard pieces = parent_bbs.get<moving_color, piece>();

		while (piece == king || pieces)
		{
			const size_t piece_idx = (piece != king) ? get_next_bit_index(pieces) : king_idx;

			tt_key incremental_key{};
			if constexpr (!quiescing && !perft)
			{
				// XOR the key for the leaving piece once for all of its moves.
				incremental_key = key ^ piece_square_key<moving_color, piece>(piece_idx);
			}

			bitboard moves{};
			if constexpr (piece == knight)
				moves = knight_attack_masks[piece_idx];
			else if constexpr (piece == king)
				moves = king_attack_masks[piece_idx];
			else
				moves = get_slider_moves<piece>(parent_bbs, piece_idx);

			const bitboard from = (piece != king) ? get_next_bit(pieces) : pieces;
			pieces = clear_next_bit(pieces);

			bitboard captures = moves & parent_bbs.get<other_color(moving_color)>();
			while (captures && (gen_moves == gen_moves::captures || gen_moves == gen_moves::all))
			{
				const bitboard to = get_next_bit(captures);
				captures = clear_next_bit(captures);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, piece, move_type::capture>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, from, to, move_info);
			}

			bitboard noncaptures = moves & parent_bbs.empty();
			while (noncaptures && (gen_moves == gen_moves::noncaptures || gen_moves == gen_moves::all))
			{
				const bitboard to = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);
				append_if_legal<moving_color, quiescing, perft, check_type, in_check, piece>(
				    end_idx, parent_board, blockers, king_idx, incremental_key, from, to, move_info);
			}

			if constexpr (piece == king) return;
		}
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft, check_type check_type, bool in_check>
	force_inline_toggle static void find_moves(size_t& end_idx, const board& parent_board, const bitboard blockers,
	    const size_t king_idx, const tt_key key, const move_info& move_info)
	{
		find_pawn_moves<moving_color, gen_moves, quiescing, perft, check_type, in_check>(
		    end_idx, parent_board, blockers, king_idx, key, move_info);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, knight>(
		    end_idx, parent_board, blockers, king_idx, key, move_info);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, bishop>(
		    end_idx, parent_board, blockers, king_idx, key, move_info);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, rook>(
		    end_idx, parent_board, blockers, king_idx, key, move_info);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type, in_check, queen>(
		    end_idx, parent_board, blockers, king_idx, key, move_info);
		find_moves_for<moving_color, gen_moves, quiescing, perft, check_type::all, false, king>(
		    end_idx, parent_board, blockers, king_idx, key, move_info);

		if constexpr (!in_check && !quiescing && (gen_moves == gen_moves::all || gen_moves == gen_moves::noncaptures))
		{
			find_castle_moves<moving_color, perft>(end_idx, parent_board, key, move_info);
		}
	}

	template <color moving_color, gen_moves gen_moves, bool quiescing, bool perft>
	size_t generate_child_boards(const size_t parent_idx)
	{
		const board& parent_board = boards[parent_idx];

		tt_key key{};
		if constexpr (!quiescing && !perft)
		{
			key = parent_board.get_key() ^ black_to_move_key();

			// Remove any ep capture rights from the key.
			if (parent_board.can_capture_ep())
			{
				key ^= en_passant_key(parent_board.get_move().get_end_file());
			}
		}

		constexpr color opp_color = other_color(moving_color);
		const bitboards& parent_bbs = parent_board.get_bitboards();
		const bitboard opp_king = parent_bbs.get<opp_color, king>();
		const size_t opp_king_idx = get_next_bit_index(opp_king);

		// Simplify detection of moves that give check during move generation by noting
		// squares from which different types of pieces could attack the opponent's king.
		move_info move_info;
		move_info.opp_king_idx = opp_king_idx;
		move_info.pawn_check_squares =
		    (pawn_capture_lower_file & (moving_color == white ? opp_king << 9 : opp_king >> 7)) |
		    (pawn_capture_higher_file & (moving_color == white ? opp_king << 7 : opp_king >> 9));
		move_info.knight_check_squares = knight_attack_masks[opp_king_idx];
		const bitboard bishop_check_squares = get_slider_moves<bishop>(parent_bbs, opp_king_idx);
		const bitboard rook_check_squares = get_slider_moves<rook>(parent_bbs, opp_king_idx);
		move_info.discovery_blockers = (bishop_check_squares | rook_check_squares) & parent_bbs.get<moving_color>();
		move_info.bishop_check_squares = bishop_check_squares;
		move_info.rook_check_squares = rook_check_squares;
		const bitboard queens = parent_bbs.get<moving_color, queen>();
		const bitboard bishops = parent_bbs.get<moving_color, bishop>();
		move_info.bishops_and_queens = bishops | queens;
		const bitboard rooks = parent_bbs.get<moving_color, rook>();
		move_info.rooks_and_queens = rooks | queens;

		// Filter which types of checks we need to look for during move generation,
		// based on which piece (if any) is attacking the king.

		const piece last_moved_piece = parent_board.get_moved_piece();
		const size_t king_idx = get_next_bit_index(parent_bbs.get<moving_color, king>());
		size_t end_idx = first_child_index(parent_idx);

		if (last_moved_piece == pawn && square_is_attacked_by_pawn<opp_color>(parent_bbs, king_idx))
		{
			find_moves<moving_color, gen_moves, quiescing, perft, check_type::pawn, true>(
			    end_idx, parent_board, bitboard{}, king_idx, key, move_info);
		}
		else if (last_moved_piece == knight && square_is_attacked_by_knight<opp_color>(parent_bbs, king_idx))
		{
			find_moves<moving_color, gen_moves, quiescing, perft, check_type::knight, true>(
			    end_idx, parent_board, bitboard{}, king_idx, key, move_info);
		}
		else // the nominal path
		{
			if (parent_board.in_check())
			{
				find_moves<moving_color, gen_moves, quiescing, perft, check_type::sliders, true>(
				    end_idx, parent_board, bitboard{}, king_idx, key, move_info);
			}
			else
			{
				const bitboard blockers = get_blockers<moving_color>(parent_bbs);
				find_moves<moving_color, gen_moves, quiescing, perft, check_type::sliders, false>(
				    end_idx, parent_board, blockers, king_idx, key, move_info);
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
