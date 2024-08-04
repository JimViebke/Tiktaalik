
#include "bitboard.hpp"
#include "config.hpp"
#include "move.hpp"
#include "transposition_table.hpp"

namespace chess
{
	template<color_t moving_color, piece_t moving_piece_type, move_type move_type, piece_t promotion_type = empty>
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

		if constexpr (move_type == move_type::en_passant_capture)
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
			constexpr piece_t piece_type = (promotion_type == empty) ? moving_piece_type : promotion_type;
			child.piece_at(end_idx) = piece_type | moving_color;
		}

		// Clear the moved piece in all cases except kingside castling.
		if constexpr (move_type != move_type::castle_kingside)
		{
			child.piece_at(start_idx) = empty;
		}
	}

	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void find_pawn_moves(size_t& out_index, const size_t parent_idx, const bitboards& bitboards,
											 const size_t king_index, const bool started_in_check, const tt::key key)
	{
		constexpr bitboard promotion_start_file = (color_to_move == white) ? rank_7 : rank_2;

		const bitboard pawns = bitboards.pawns & bitboards.get<color_to_move>();

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

				append_if_legal<color_to_move, check_type, pawn, move_type::capture, queen>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
				append_if_legal<color_to_move, check_type, pawn, move_type::capture, knight>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
				append_if_legal<color_to_move, check_type, pawn, move_type::capture, rook>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
				append_if_legal<color_to_move, check_type, pawn, move_type::capture, bishop>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 9 : start << 7);
			}

			while (capture_to_lower_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_lower_file);
				const bitboard start = get_next_bit(capture_to_lower_file);
				capture_to_lower_file = clear_next_bit(capture_to_lower_file);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
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

				append_if_legal<color_to_move, check_type, pawn, move_type::capture, queen>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 7 : start << 9);
				append_if_legal<color_to_move, check_type, pawn, move_type::capture, knight>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 7 : start << 9);
				append_if_legal<color_to_move, check_type, pawn, move_type::capture, rook>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 7 : start << 9);
				append_if_legal<color_to_move, check_type, pawn, move_type::capture, bishop>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 7 : start << 9);
			}

			while (capture_to_higher_file)
			{
				const size_t start_idx = get_next_bit_index(capture_to_higher_file);
				const bitboard start = get_next_bit(capture_to_higher_file);
				capture_to_higher_file = clear_next_bit(capture_to_higher_file);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, pawn, move_type::capture>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 7 : start << 9);
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

					append_if_legal<color_to_move, check_type, pawn, move_type::en_passant_capture>(
						out_index, parent_idx, bitboards, king_index, started_in_check, incremental_key,
						start, (1ull << ((color_to_move == white) ? 16 : 40)) << ep_capture_file);
				}
			}
		}

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::noncaptures)
		{
			const bitboard empty_squares = bitboards.get_empty();
			bitboard move_one_square = pawns & ((color_to_move == white) ? empty_squares << 8 : empty_squares >> 8);

			bitboard noncapture_promotions = move_one_square & promotion_start_file;
			move_one_square ^= noncapture_promotions;

			while (noncapture_promotions)
			{
				const size_t start_idx = get_next_bit_index(noncapture_promotions);
				const bitboard start = get_next_bit(noncapture_promotions);
				noncapture_promotions = clear_next_bit(noncapture_promotions);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, pawn, move_type::other, queen>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 8 : start << 8);
				append_if_legal<color_to_move, check_type, pawn, move_type::other, knight>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 8 : start << 8);
				append_if_legal<color_to_move, check_type, pawn, move_type::other, rook>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 8 : start << 8);
				append_if_legal<color_to_move, check_type, pawn, move_type::other, bishop>(
					out_index, parent_idx, bitboards, king_index, started_in_check,
					incremental_key, start, (color_to_move == white) ? start >> 8 : start << 8);
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

				append_if_legal<color_to_move, check_type, pawn, move_type::pawn_two_squares>(
					out_index, parent_idx, bitboards, king_index, started_in_check, incremental_key,
					start, (color_to_move == white) ? start >> 16 : start << 16);
			}

			while (move_one_square)
			{
				const size_t start_idx = get_next_bit_index(move_one_square);
				const bitboard start = get_next_bit(move_one_square);
				move_one_square = clear_next_bit(move_one_square);

				const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | pawn][start_idx];

				append_if_legal<color_to_move, check_type, pawn>(
					out_index, parent_idx, bitboards, king_index, started_in_check, incremental_key,
					start, (color_to_move == white) ? start >> 8 : start << 8);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void find_knight_moves(size_t& out_index, const size_t parent_idx, const bitboards& bitboards,
											   const rank rank, const file file,
											   const size_t king_index, const bool started_in_check, const tt::key key)
	{
		const size_t knight_index = to_index(rank, file);
		const bitboard moves = knight_attack_masks[knight_index];

		bitboard captures = moves & bitboards.get<other_color(color_to_move)>();
		bitboard noncaptures = moves & bitboards.get_empty();

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::captures)
		{
			while (captures)
			{
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<color_to_move, check_type, knight, move_type::capture>(
					out_index, parent_idx, bitboards, king_index, started_in_check, key,
					1ull << to_index(rank, file), end);
			}
		}

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::noncaptures)
		{
			while (noncaptures)
			{
				const bitboard end = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<color_to_move, check_type, knight>(
					out_index, parent_idx, bitboards, king_index, started_in_check, key,
					1ull << to_index(rank, file), end);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type, piece_t piece_type>
	force_inline_toggle void find_slider_moves(size_t& out_index, const size_t parent_idx, const bitboards& bitboards,
											   const rank rank, const file file,
											   const size_t king_index, const bool started_in_check, const tt::key key)
	{
		static_assert(piece_type == bishop || piece_type == rook || piece_type == queen);

		const size_t idx = to_index(rank, file);

		bitboard bishop_pext_mask;
		bitboard rook_pext_mask;
		if constexpr (piece_type == bishop || piece_type == queen)
			bishop_pext_mask = bishop_pext_masks[idx];
		if constexpr (piece_type == rook || piece_type == queen)
			rook_pext_mask = rook_pext_masks[idx];

		const bitboard occupied = bitboards.white | bitboards.black;

		size_t bishop_movemask_idx;
		size_t rook_movemask_idx;
		if constexpr (piece_type == bishop || piece_type == queen)
			bishop_movemask_idx = pext(occupied, bishop_pext_mask);
		if constexpr (piece_type == rook || piece_type == queen)
			rook_movemask_idx = pext(occupied, rook_pext_mask);

		bitboard moves;
		if constexpr (piece_type == bishop || piece_type == queen)
			moves |= (*bishop_move_masks)[idx][bishop_movemask_idx];
		if constexpr (piece_type == rook || piece_type == queen)
			moves |= (*rook_move_masks)[idx][rook_movemask_idx];

		if constexpr (gen_moves == gen_moves::captures ||
					  gen_moves == gen_moves::all)
		{
			bitboard captures = moves & bitboards.get<other_color(color_to_move)>();
			while (captures)
			{
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<color_to_move, check_type, piece_type, move_type::capture>(
					out_index, parent_idx, bitboards, king_index, started_in_check, key,
					1ull << to_index(rank, file), end);
			}
		}

		if constexpr (gen_moves == gen_moves::noncaptures ||
					  gen_moves == gen_moves::all)
		{
			bitboard noncaptures = moves & bitboards.get_empty();
			while (noncaptures)
			{
				const bitboard end = get_next_bit(noncaptures);
				noncaptures = clear_next_bit(noncaptures);

				append_if_legal<color_to_move, check_type, piece_type>(
					out_index, parent_idx, bitboards, king_index, started_in_check, key,
					1ull << to_index(rank, file), end);
			}
		}
	}
	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void find_king_moves(size_t& out_index, const size_t parent_idx, const bitboards& bitboards,
											 const size_t king_index, const bool started_in_check, const tt::key key)
	{
		const rank rank = king_index / 8;
		const file file = king_index % 8;

		const tt::key incremental_key = key ^ tt::z_keys.piece_square_keys[color_to_move | king][to_index(rank, file)];

		const position& position = positions[parent_idx];

		const bitboard moves = king_attack_masks[king_index];

		bitboard captures = moves & bitboards.get<other_color(color_to_move)>();
		bitboard non_captures = moves & bitboards.get_empty();

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::captures)
		{
			while (captures)
			{
				size_t end_idx = get_next_bit_index(captures);
				const bitboard end = get_next_bit(captures);
				captures = clear_next_bit(captures);

				append_if_legal<color_to_move, check_type::do_all, king, move_type::capture>(
					out_index, parent_idx, bitboards, end_idx, started_in_check, incremental_key,
					1ull << king_index, end);
			}
		}

		if constexpr (gen_moves == gen_moves::all ||
					  gen_moves == gen_moves::noncaptures)
		{
			while (non_captures)
			{
				size_t end_idx = get_next_bit_index(non_captures);
				const bitboard end = get_next_bit(non_captures);
				non_captures = clear_next_bit(non_captures);

				append_if_legal<color_to_move, check_type::do_all, king>(
					out_index, parent_idx, bitboards, end_idx, started_in_check, incremental_key,
					1ull << king_index, end);
			}

			if (started_in_check) return; // A king cannot castle out of check.

			const board& board = boards[parent_idx];

			const bool can_castle_ks = (color_to_move == white) ? board.white_can_castle_ks() : board.black_can_castle_ks();

			constexpr uint32_t ks_pieces_mask = ((color_to_move | rook) << 24) + (empty << 16) + (empty << 8) + (color_to_move | king);
			constexpr size_t king_start_idx = (color_to_move == white) ? 60 : 4;
			const uint32_t ks_pieces = *(uint32_t*)(&position[king_start_idx]);

			if (can_castle_ks && ks_pieces == ks_pieces_mask)
			{
				// Check that the king would not be moving through check.
				if (!is_king_in_check<color_to_move, check_type::do_all>(bitboards, rank, file + 1))
				{
					append_if_legal<color_to_move, check_type::do_all, king, move_type::castle_kingside>(
						out_index, parent_idx, bitboards, to_index(rank, file + 2), started_in_check, incremental_key,
						1ull << king_index, 1ull << to_index(rank, file + 2));
				}
			}

			const bool can_castle_qs = (color_to_move == white) ? board.white_can_castle_qs() : board.black_can_castle_qs();

			constexpr uint32_t qs_pieces_mask = (empty << 24) + (empty << 16) + (empty << 8) + (color_to_move | rook);
			constexpr size_t qs_rook_start_index = (color_to_move == white) ? 56 : 0;
			const uint32_t qs_pieces = *(uint32_t*)(&position[qs_rook_start_index]);

			if (can_castle_qs && qs_pieces == qs_pieces_mask)
			{
				if (!is_king_in_check<color_to_move, check_type::do_all>(bitboards, rank, file - 1))
				{
					append_if_legal<color_to_move, check_type::do_all, king, move_type::castle_queenside>(
						out_index, parent_idx, bitboards, to_index(rank, file - 2), started_in_check, incremental_key,
						1ull << king_index, 1ull << to_index(rank, file - 2));
				}
			}
		}
	}

	force_inline_toggle bool moving_piece_might_have_been_pinned(const size_t king_index,
																 const bitboard start)
	{
		// If a piece left a square that shared a rank, file, or diagonal with the king, it might have been pinned.
		const bitboard king_attack_mask = bishop_attack_masks[king_index] | rook_attack_masks[king_index];
		return (king_attack_mask & start) != 0;
	}

	template <color_t moving_color, check_type check_type, piece_t moving_piece_type,
		move_type move_type = move_type::other, piece_t promotion_type = empty, typename... board_args>
	force_inline_toggle void append_if_legal(size_t& out_index, const size_t parent_idx, const bitboards& bitboards,
											 const size_t king_index, const bool started_in_check, const tt::key key,
											 const bitboard start, const bitboard end)
	{
		constexpr color_t child_color = other_color(moving_color);

		board& child_board = boards[out_index];
		child_board.update_bitboards<child_color, moving_piece_type, move_type, promotion_type>(parent_idx, start, end);

		// If the king is in check, return early. We could be in check if:
		// - We are trying to move our king, or
		// - we started in check, or
		// - the moving piece shared a rank, file, or diagonal with the king.
		// If none of these are the case (most of the time), we cannot possibly be in check, and
		// we can skip the expensive call to is_king_in_check().
		if (moving_piece_type == king ||
			started_in_check ||
			moving_piece_might_have_been_pinned(king_index, start))
		{
			if (is_king_in_check<moving_color, check_type>(child_board.get_bitboards(), king_index / 8, king_index % 8)) return;
		}

		board::make_board<child_color, moving_piece_type, move_type, promotion_type>(out_index, parent_idx, start, end);

		position& child_position = positions[out_index];
		const position& parent_position = positions[parent_idx];
		make_move<moving_color, moving_piece_type, move_type, promotion_type>(child_position, parent_position, child_board);

		++out_index;

		child_board.update_key_and_eval<moving_color, moving_piece_type, move_type, promotion_type>(
			parent_position, boards[parent_idx], key);

		if constexpr (config::verify_incremental_key)
			if (child_board.get_key() != generate_key(child_board, child_position, child_color))
				std::cout << "Incremental and generated keys mismatch in append_if_legal\n";

		if constexpr (config::verify_incremental_eval)
			if (child_board.get_eval() != child_position.evaluate_position())
				std::cout << "Incremental and generated evals mismatch in append_if_legal\n";

		if constexpr (config::verify_incremental_bitboards)
			if (child_board.get_bitboards() != get_bitboards(child_position))
				std::cout << "Incremental and generated bitboards mismatch in append_if_legal\n";
	}

	template<color_t color_to_move, gen_moves gen_moves, check_type check_type>
	force_inline_toggle void generate_child_boards(size_t& end_idx, const size_t parent_idx, const bitboards& bitboards,
												   const size_t king_index, const bool started_in_check,
												   const tt::key key)
	{
		find_pawn_moves<color_to_move, gen_moves, check_type>(
			end_idx, parent_idx, bitboards, king_index, started_in_check, key);
		find_moves_for<color_to_move, knight>(
			end_idx, parent_idx, bitboards, king_index, started_in_check, key,
			&find_knight_moves<color_to_move, gen_moves, check_type>);
		find_moves_for<color_to_move, bishop>(
			end_idx, parent_idx, bitboards, king_index, started_in_check, key,
			&find_slider_moves<color_to_move, gen_moves, check_type, bishop>);
		find_moves_for<color_to_move, rook>(
			end_idx, parent_idx, bitboards, king_index, started_in_check, key,
			&find_slider_moves<color_to_move, gen_moves, check_type, rook>);
		find_moves_for<color_to_move, queen>(
			end_idx, parent_idx, bitboards, king_index, started_in_check, key,
			&find_slider_moves<color_to_move, gen_moves, check_type, queen>);
		find_king_moves<color_to_move, gen_moves, check_type>(
			end_idx, parent_idx, bitboards, king_index, started_in_check, key);
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
		const size_t king_index = get_next_bit_index(bitboards.get<color_to_move>() & bitboards.kings);
		size_t end_idx = first_child_index(parent_idx);

		if (last_moved_piece.is_pawn() &&
			square_is_attacked_by_pawn<opp_color>(bitboards, king_index))
		{
			generate_child_boards<color_to_move, gen_moves, check_type::do_pawn_checks>(
				end_idx, parent_idx, bitboards, king_index, true, key);
		}
		else if (last_moved_piece.is_knight() &&
				 square_is_attacked_by_knight<opp_color>(bitboards, king_index))
		{
			generate_child_boards<color_to_move, gen_moves, check_type::do_knight_checks>(
				end_idx, parent_idx, bitboards, king_index, true, key);
		}
		else // the nominal path
		{
			const bool started_in_check = is_king_in_check<color_to_move, check_type::skip_pawn_and_knight_checks>(
				bitboards, king_index / 8, king_index % 8);
			generate_child_boards<color_to_move, gen_moves, check_type::skip_pawn_and_knight_checks>(
				end_idx, parent_idx, bitboards, king_index, started_in_check, key);
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
