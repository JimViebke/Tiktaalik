#pragma once

#include <array>
#include <bit>
#include <variant>
#include <vector>

#include "bitboard.hpp"
#include "config.hpp"
#include "defines.hpp"
#include "evaluation.hpp"
#include "move.hpp"
#include "transposition_table.hpp"
#include "util/intrinsics.hpp"
#include "util/util.hpp"

namespace chess
{
	enum class move_type
	{
		capture,
		en_passant_capture,
		pawn_two_squares,
		castle_kingside,
		castle_queenside,
		other
	};

	class board;

	constexpr size_t max_n_of_moves = 256;
	constexpr size_t boards_size = max_ply * max_n_of_moves;
	extern std::array<board, boards_size> boards;

	struct move_info
	{
		bitboard pawn_check_squares{};
		bitboard knight_check_squares{};
		bitboard bishop_check_squares{};
		bitboard rook_check_squares{};

		bitboard discovery_blockers{};

		bitboard bishops_and_queens{};
		bitboard rooks_and_queens{};

		size_t opp_king_idx{};
	};

	inline constexpr size_t first_child_index(const size_t parent_index)
	{
		static_assert(std::popcount(max_n_of_moves) == 1);
		constexpr size_t ply_mask = ~(max_n_of_moves - 1);

		return (parent_index + max_n_of_moves) & ply_mask;
	}

	template <color attacker_color>
	[[clang::always_inline]] bool square_is_attacked_by_pawn(const bitboards& bitboards, const size_t target_idx)
	{
		const bitboard opp_pawns = bitboards.get<attacker_color, pawn>();
		const bitboard index_bb = 1ull << target_idx;

		const bitboard checkers_to_lower_file =
		    opp_pawns & pawn_capture_lower_file & ((attacker_color == white) ? index_bb << 9 : index_bb >> 7);
		const bitboard checkers_to_higher_file =
		    opp_pawns & pawn_capture_higher_file & ((attacker_color == white) ? index_bb << 7 : index_bb >> 9);

		return checkers_to_lower_file | checkers_to_higher_file;
	}

	template <color attacker_color>
	[[clang::always_inline]] bool square_is_attacked_by_knight(const bitboards& bitboards, const size_t target_idx)
	{
		const bitboard opp_knights = bitboards.get<attacker_color, knight>();
		return opp_knights & knight_attack_masks[target_idx];
	}

	template <color attacker_color>
	[[clang::always_inline]] bool square_is_attacked_by_king(const bitboards& bitboards, const size_t target_idx)
	{
		const bitboard opp_king = bitboards.get<attacker_color, king>();
		return opp_king & king_attack_masks[target_idx];
	}

	// Constrain which types of checks in_check() performs.
	// - If we are moving our king or are outside of move generation, do all checks.
	// - Otherwise, if we started in check from a pawn, do pawn and slider checks.
	// - Otherwise, if we started in check from a knight, do knight and slider checks.
	// - Otherwise (the nominal case), only do slider checks.
	enum class check_type
	{
		all,
		pawn,
		knight,
		sliders
	};

	template <color king_color, check_type check_type = check_type::all>
	force_inline_toggle bool in_check(const board& board, const size_t king_idx)
	{
		constexpr color opp_color = other_color(king_color);

		if constexpr (check_type == check_type::all)
		{
			if (square_is_attacked_by_king<opp_color>(board.get_bitboards(), king_idx)) return true;
		}

		if constexpr (check_type == check_type::knight || check_type == check_type::all)
		{
			if (square_is_attacked_by_knight<opp_color>(board.get_bitboards(), king_idx)) return true;
		}

		if constexpr (check_type == check_type::pawn || check_type == check_type::all)
		{
			if (square_is_attacked_by_pawn<opp_color>(board.get_bitboards(), king_idx)) return true;
		}

		return is_attacked_by_sliding_piece<king_color>(board.get_bitboards(), king_idx);
	}

	template <color king_color, check_type check_type = check_type::all>
	force_inline_toggle bool in_check(const board& board)
	{
		const size_t king_idx = get_next_bit_index(board.get_bitboards().get<king_color, king>());
		return in_check<king_color, check_type>(board, king_idx);
	}

	inline eval_t taper(const phase_t phase, const int64_t mg_eval, const int64_t eg_eval)
	{
		return (mg_eval * phase + eg_eval * (eval::total_phase - phase)) / eval::total_phase;
	}

	class board
	{
	public:
		board() {}

		// Simple, nonvalidating FEN parser
		color load_fen(const std::string& fen);
		void generate_eval();
		template <bool verify_key = true, bool verify_phase = true, bool verify_eval = true>
		void verify_key_phase_eval(const color color_to_move);

		template <color moving_color, bool perft, piece piece, move_type move_type, chess::piece promoted_piece>
		force_inline_toggle void copy_make_bitboards(
		    const board& parent_board, const bitboard from, const bitboard to, chess::piece& captured_piece)
		{
			const class bitboards& parent_bbs = parent_board.get_bitboards();

			uint256_t bb_lo = _mm256_loadu_si256((uint256_t*)&parent_bbs);
			uint256_t bb_hi = _mm256_loadu_si256(((uint256_t*)&parent_bbs) + 1);

			// 8 bitboards require two ymm vector registers:
			// [    0,     1,     2,       3] [      4,     5,      6,     7]
			// [white, black, pawns, knights] [bishops, rooks, queens, kings]

			// Remove the captured bit. We don't know what type of piece was captured,
			// so clear the bit across all eight bitboards.
			if constexpr (move_type == move_type::capture)
			{
				const uint256_t clear_bits = _mm256_set1_epi64x(to);

				const uint256_t old_lo = bb_lo;
				const uint256_t old_hi = bb_hi;
				bb_lo = _mm256_andnot_si256(clear_bits, bb_lo);
				bb_hi = _mm256_andnot_si256(clear_bits, bb_hi);

				if constexpr (!perft)
				{
					uint32_t lo = _mm256_movemask_pd(_mm256_cmpeq_epi64(bb_lo, old_lo));
					uint32_t hi = _mm256_movemask_pd(_mm256_cmpeq_epi64(bb_hi, old_hi));
					uint32_t masks = (hi << 4) | lo;            // Combine.
					masks ^= 0b1111'1111;                       // Invert the byte we care about.
					masks >>= 2;                                // Remove the two color bits.
					captured_piece = get_next_bit_index(masks); // Get 0..4 for pawn..queen.
				}
			}

			if constexpr (promoted_piece == empty)
			{
				uint256_t toggle_bits = _mm256_setzero_si256();
				toggle_bits = _mm256_insert_epi64(toggle_bits, from | to, 0);

				if constexpr (piece == pawn || piece == knight)
				{
					// If a white pawn is moving, XOR at 0 and 2.
					// If a white knight is moving, XOR at 0 and 3.
					// If a black pawn is moving, XOR at 1 and 2.
					// If a black knight is moving, XOR at 1 and 3.
					if constexpr (moving_color == white && piece == pawn)
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b01'00'01'00); // 0b01 is empty.
					else if constexpr (moving_color == white && piece == knight)
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b00'01'01'00);
					else if constexpr (moving_color == black && piece == pawn)
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b01'00'00'01);
					else // A black knight is moving.
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b00'01'00'01);

					bb_lo = _mm256_xor_si256(bb_lo, toggle_bits);
				}
				else // A rook, bishop, queen, or king is moving.
				{
					// XOR at [0] or [1] for the color bitboard.
					if constexpr (moving_color == white)
						bb_lo = _mm256_xor_si256(bb_lo, toggle_bits);
					else
						bb_lo = _mm256_xor_si256(bb_lo, _mm256_permute4x64_epi64(toggle_bits, 0b01'01'00'01));

					// XOR at one of [4-7] for the moving bishop/rook/queen/king.
					if constexpr (piece == bishop)
						bb_hi = _mm256_xor_si256(bb_hi, toggle_bits);
					else if constexpr (piece == rook)
						bb_hi = _mm256_xor_si256(bb_hi, _mm256_permute4x64_epi64(toggle_bits, 0b01'01'00'01));
					else if constexpr (piece == queen)
						bb_hi = _mm256_xor_si256(bb_hi, _mm256_permute4x64_epi64(toggle_bits, 0b01'00'01'01));
					else // A king is moving.
						bb_hi = _mm256_xor_si256(bb_hi, _mm256_permute4x64_epi64(toggle_bits, 0b00'01'01'01));
				}
			}

			_mm256_storeu_si256(((uint256_t*)&bitboards), bb_lo);
			_mm256_storeu_si256(((uint256_t*)&bitboards) + 1, bb_hi);

			bitboard our_pieces = bitboards.get<moving_color>();
			bitboard opp_pieces = bitboards.get<other_color(moving_color)>();

			if constexpr (promoted_piece != empty)
			{
				our_pieces ^= from | to;

				bitboards.pawns ^= from;

				if constexpr (promoted_piece == knight)
					bitboards.knights |= to;
				else if constexpr (promoted_piece == bishop)
					bitboards.bishops |= to;
				else if constexpr (promoted_piece == rook)
					bitboards.rooks |= to;
				else if constexpr (promoted_piece == queen)
					bitboards.queens |= to;
			}

			// For en passant captures, remove two additional bits.
			if constexpr (move_type == move_type::en_passant_capture)
			{
				const bitboard captured_pawn_bit = ((moving_color == white) ? to << 8 : to >> 8);
				// Remove the arriving bit (+/- 8) from pawns.
				bitboards.pawns ^= captured_pawn_bit;
				// Remove the arriving bit (+/- 8) from opp_pieces.
				opp_pieces ^= captured_pawn_bit;
			}

			// For castling:
			// - Invert two additional bits in rooks.
			// - Invert two additional bits in our_pieces.
			// - Todo: invert all four bits in our_pieces at once.
			if constexpr (move_type == move_type::castle_kingside)
			{
				constexpr bitboard invert_bits = 0b1010'0000uz << ((moving_color == white) ? 56 : 0);
				bitboards.rooks ^= invert_bits;
				our_pieces ^= invert_bits;
			}
			else if constexpr (move_type == move_type::castle_queenside)
			{
				constexpr bitboard invert_bits = 0b1001uz << ((moving_color == white) ? 56 : 0);
				bitboards.rooks ^= invert_bits;
				our_pieces ^= invert_bits;
			}

			bitboards.white = (moving_color == white) ? our_pieces : opp_pieces;
			bitboards.black = (moving_color == white) ? opp_pieces : our_pieces;
		}

		template <color moving_color, bool quiescing, bool perft, piece piece, move_type move_type,
		    chess::piece promoted_piece>
		force_inline_toggle void copy_make_board(const board& parent_board, tt_key incremental_key, const bitboard from,
		    const bitboard to, const chess::piece captured_piece, const move_info& move_info)
		{
			// Generate a mask at compile time to selectively copy parent state.
			constexpr uint64_t copy_mask = get_copy_mask<moving_color, piece, move_type, promoted_piece>();
			uint64_t bitfield = *(uint64_t*)&parent_board.board_state & copy_mask;

			// Record the move that resulted in this position.
			const size_t start_idx = get_next_bit_index(from);
			const size_t end_idx = get_next_bit_index(to);
			move = move::make_move<piece, promoted_piece>(start_idx, end_idx);

			// If the move is not a capture or pawn move, increment the fifty-move counter.
			if constexpr (move_type != move_type::capture && piece != pawn)
			{
				bitfield += 1ull << fifty_move_counter_offset;
			}

			// Record any en passant rights for the opponent.
			if constexpr (move_type == move_type::pawn_two_squares)
			{
				bitfield |= 1ull << en_passant_offset;
			}

			constexpr chess::piece piece_after = (promoted_piece == empty) ? piece : promoted_piece;

			const bool gives_check =
			    detect_check<moving_color, piece_after, move_type, promoted_piece>(from, to, move_info);
			bitfield |= uint64_t(gives_check) << check_offset;

			// If a rook moves, it cannot be used to castle.
			// Update castling rights for the moving player.
			// During quiescence, we can't castle, so skip this.
			if constexpr (piece == rook && !quiescing)
			{
				const rank start_rank = start_idx / 8;
				const file start_file = start_idx % 8;
				if constexpr (moving_color == white)
				{
					if (start_rank == 7)
					{
						if (start_file == 0)
							bitfield &= ~(1ull << white_can_castle_qs_offset);
						else if (start_file == 7)
							bitfield &= ~(1ull << white_can_castle_ks_offset);
					}
				}
				else
				{
					if (start_rank == 0)
					{
						if (start_file == 0)
							bitfield &= ~(1ull << black_can_castle_qs_offset);
						else if (start_file == 7)
							bitfield &= ~(1ull << black_can_castle_ks_offset);
					}
				}
			}

			// If a rook is captured, it cannot be used to castle.
			// Update castling rights for the opponent.
			// During quiescence, we can't castle, so skip this.
			if constexpr (move_type == move_type::capture && !quiescing)
			{
				const rank end_rank = end_idx / 8;
				const file end_file = end_idx % 8;
				if constexpr (moving_color == white)
				{
					if (end_rank == 0)
					{
						if (end_file == 0)
							bitfield &= ~(1ull << black_can_castle_qs_offset);
						else if (end_file == 7)
							bitfield &= ~(1ull << black_can_castle_ks_offset);
					}
				}
				else
				{
					if (end_rank == 7)
					{
						if (end_file == 0)
							bitfield &= ~(1ull << white_can_castle_qs_offset);
						else if (end_file == 7)
							bitfield &= ~(1ull << white_can_castle_ks_offset);
					}
				}
			}

			*(uint64_t*)&board_state = bitfield;

			// Don't update keys, phases, or evals during perft.
			if constexpr (perft) return;

			// The incremental_key has already had the leaving piece removed, and the color to move toggled.
			// We receive it by copy so we can modify it before writing it.

			// Don't update keys during quiescence.
			if constexpr (!quiescing)
			{
				// Add the key for the arriving piece.
				incremental_key ^= piece_square_key<moving_color, piece_after>(end_idx);
			}

			eval_t incremental_mg_eval = parent_board.mg_eval;
			eval_t incremental_eg_eval = parent_board.eg_eval;

			// Update piece-square evals for the moving piece.
			incremental_mg_eval -= eval::piece_square_eval_mg<moving_color, piece>(start_idx);
			incremental_eg_eval -= eval::piece_square_eval_eg<moving_color, piece>(start_idx);
			incremental_mg_eval += eval::piece_square_eval_mg<moving_color, piece_after>(end_idx);
			incremental_eg_eval += eval::piece_square_eval_eg<moving_color, piece_after>(end_idx);

			constexpr color opp_color = other_color(moving_color);

			if constexpr (move_type == move_type::pawn_two_squares)
			{
				// Add en passant rights for the opponent.
				incremental_key ^= en_passant_key(end_idx % 8);
			}
			else if constexpr (move_type == move_type::en_passant_capture)
			{
				const size_t captured_pawn_idx = end_idx + ((moving_color == white) ? 8 : -8);

				if constexpr (!quiescing)
				{
					// Remove key for the captured pawn.
					incremental_key ^= piece_square_key<opp_color, pawn>(captured_pawn_idx);
				}

				// Update piece-square evals for the captured pawn.
				incremental_mg_eval -= eval::piece_square_eval_mg<opp_color, pawn>(captured_pawn_idx);
				incremental_eg_eval -= eval::piece_square_eval_eg<opp_color, pawn>(captured_pawn_idx);
			}
			else if constexpr (move_type == move_type::castle_kingside || move_type == move_type::castle_queenside)
			{
				constexpr rank rook_start_rank = (moving_color == white ? 7 : 0);
				constexpr file rook_start_file = (move_type == move_type::castle_kingside ? 7 : 0);
				constexpr size_t rook_start_index = rook_start_rank * 8 + rook_start_file;
				constexpr size_t rook_end_index = rook_start_index + (move_type == move_type::castle_kingside ? -2 : 3);

				if constexpr (!quiescing)
				{
					// Update keys for the moving rook.
					incremental_key ^= piece_square_key<moving_color, rook>(rook_start_index);
					incremental_key ^= piece_square_key<moving_color, rook>(rook_end_index);
				}

				// Update piece-square evals for the moving rook.
				incremental_mg_eval -= eval::piece_square_eval_mg<moving_color, rook>(rook_start_index);
				incremental_eg_eval -= eval::piece_square_eval_eg<moving_color, rook>(rook_start_index);
				incremental_mg_eval += eval::piece_square_eval_mg<moving_color, rook>(rook_end_index);
				incremental_eg_eval += eval::piece_square_eval_eg<moving_color, rook>(rook_end_index);
			}
			else if constexpr (move_type == move_type::capture)
			{
				if constexpr (!quiescing)
				{
					// Remove the key for the captured piece.
					incremental_key ^= piece_square_key<opp_color>(captured_piece, end_idx);

					// Update the opponent's castling rights.
					update_key_castling_rights_for<opp_color>(incremental_key, parent_board);
				}

				// Update piece-square evals for the captured piece.
				incremental_mg_eval -= eval::piece_square_eval_mg<opp_color>(captured_piece, end_idx);
				incremental_eg_eval -= eval::piece_square_eval_eg<opp_color>(captured_piece, end_idx);
			}

			if constexpr ((piece == king || piece == rook) && !quiescing)
			{
				// A king or rook is moving; update our castling rights.
				// During quiescence, we can't castle, so skip this.
				update_key_castling_rights_for<moving_color>(incremental_key, parent_board);
			}

			if constexpr (!quiescing)
			{
				key = incremental_key;
			}

			phase_t incremental_phase = parent_board.phase;
			eval_t incremental_persistent_eval = parent_board.persistent_eval;

			if constexpr (move_type == move_type::capture)
			{
				incremental_phase -= eval::phase_weights[captured_piece];
				incremental_persistent_eval -= eval::piece_count_eval<opp_color>(captured_piece, bitboards);
				incremental_persistent_eval -=
				    eval::file_piece_count_eval<opp_color>(captured_piece, end_idx % 8, bitboards);
			}
			else if constexpr (move_type == move_type::en_passant_capture)
			{
				incremental_phase -= eval::phase_weights[pawn];
				incremental_persistent_eval -= eval::piece_count_eval<opp_color, pawn>(bitboards);
				incremental_persistent_eval -= eval::file_piece_count_eval<opp_color, pawn>(end_idx % 8, bitboards);
			}
			else if constexpr (move_type == move_type::castle_kingside || move_type == move_type::castle_queenside)
			{
				constexpr file rook_start_file = (move_type == move_type::castle_kingside ? 7 : 0);
				constexpr file rook_end_file = (move_type == move_type::castle_kingside ? 5 : 3);
				incremental_persistent_eval -=
				    eval::file_piece_count_eval<moving_color, rook>(rook_start_file, bitboards);
				incremental_persistent_eval += eval::file_piece_count_eval<moving_color, rook>(
				    bitboards.file_count<moving_color, rook>(rook_end_file) - 1);
			}

			if constexpr (promoted_piece != empty)
			{
				incremental_phase -= eval::phase_weights[pawn];
				incremental_phase += eval::phase_weights[promoted_piece];
				incremental_persistent_eval -= eval::piece_count_eval<moving_color, pawn>(bitboards);
				incremental_persistent_eval += eval::piece_count_eval<moving_color, promoted_piece>(
				    bitboards.count<moving_color, promoted_piece>() - 1);
			}

			// If the moving piece is a rook, or a pawn that is promoting or changing files,
			// update the file-count eval for the moving piece.
			if constexpr (piece == rook || promoted_piece != empty ||
			              (piece == pawn &&
			                  (move_type == move_type::capture || move_type == move_type::en_passant_capture)))
			{
				bitboard start_file_pieces = bitboards.get<moving_color, piece>() & (file_mask << (start_idx % 8));
				// If the moving piece is a rook, exclude the arriving bit, in case the rook is moving within a file.
				if constexpr (piece == rook) start_file_pieces &= ~to;

				incremental_persistent_eval -=
				    eval::file_piece_count_eval<moving_color, piece>(::util::popcount(start_file_pieces));
				incremental_persistent_eval += eval::file_piece_count_eval<moving_color, piece_after>(
				    bitboards.file_count<moving_color, piece_after>(end_idx % 8) - 1);
			}

			// If the move is a capture, en passant capture, or promotion, the phase was modified and must be stored.
			// Otherwise, the unmodified value was already copied over with the parent board state.
			if constexpr (move_type == move_type::capture || move_type == move_type::en_passant_capture ||
			              promoted_piece != empty)
			{
				phase = incremental_phase;
			}

			// If the move is a capture, en passant capture, promotion, rook move, or castle,
			// the persistent evaluation was modified and must be stored.
			// Otherwise, the unmodified value was already copied over with the parent board state.
			if constexpr (move_type == move_type::capture || move_type == move_type::en_passant_capture ||
			              promoted_piece != empty || piece == rook || move_type == move_type::castle_kingside ||
			              move_type == move_type::castle_queenside)
			{
				persistent_eval = incremental_persistent_eval;
			}

			mg_eval = incremental_mg_eval;
			eg_eval = incremental_eg_eval;
			eval = incremental_persistent_eval + taper(incremental_phase, incremental_mg_eval, incremental_eg_eval);
		}

		tt_key get_key() const { return key; }
		eval_t get_eval() const { return eval; }
		template <color color_to_move>
		eval_t get_eval() const
		{
			return color_to_move == white ? eval : -eval;
		}
		move get_move() const { return move; }
		bool move_is(const move passed_move) const { return move == passed_move; }
		piece get_moved_piece() const { return move.get_moved_piece(); }
		bool can_capture_ep() const { return (board_state >> en_passant_offset) & en_passant_mask; }
		bool white_can_castle_ks() const { return (board_state >> white_can_castle_ks_offset) & castling_right_mask; }
		bool white_can_castle_qs() const { return (board_state >> white_can_castle_qs_offset) & castling_right_mask; }
		bool black_can_castle_ks() const { return (board_state >> black_can_castle_ks_offset) & castling_right_mask; }
		bool black_can_castle_qs() const { return (board_state >> black_can_castle_qs_offset) & castling_right_mask; }
		size_t get_fifty_move_counter() const
		{
			return (board_state >> fifty_move_counter_offset) & fifty_move_counter_mask;
		}
		bool in_check() const { return (board_state >> check_offset) & check_mask; }
		const class bitboards& get_bitboards() const { return bitboards; }

	private:
		void set_previous_move_info(const color color_to_move);

		template <color moving_color, piece piece, move_type move_type, chess::piece promoted_piece>
		consteval static uint64_t get_copy_mask()
		{
			uint64_t copy_mask = 0;

			// Always copy the opponent's castling rights.
			if constexpr (moving_color == white)
			{
				copy_mask |= 1ull << black_can_castle_ks_offset;
				copy_mask |= 1ull << black_can_castle_qs_offset;
			}
			else
			{
				copy_mask |= 1ull << white_can_castle_ks_offset;
				copy_mask |= 1ull << white_can_castle_qs_offset;
			}

			// If the move is not a king move, copy the moving player's castling rights.
			if constexpr (piece != king)
			{
				if constexpr (moving_color == white)
				{
					copy_mask |= 1ull << white_can_castle_ks_offset;
					copy_mask |= 1ull << white_can_castle_qs_offset;
				}
				else
				{
					copy_mask |= 1ull << black_can_castle_ks_offset;
					copy_mask |= 1ull << black_can_castle_qs_offset;
				}
			}

			// If the move is not a capture or pawn move, copy the fifty-move counter.
			if constexpr (move_type != move_type::capture && piece != pawn)
			{
				copy_mask |= fifty_move_counter_mask << fifty_move_counter_offset;
			}

			// If the move is not a capture, en passant capture, or promotion, the phase will not change.
			// Copy it unmodified from the parent.
			if constexpr (move_type != move_type::capture && move_type != move_type::en_passant_capture &&
			              promoted_piece == empty)
			{
				copy_mask |= 0x00'00'00'00'FF'FF'00'00;

				// If the move is not a capture, en passant capture, promotion rook move, or castle,
				// the persistent eval will not change. Copy it unmodified from the parent.
				if constexpr (piece != rook && move_type != move_type::castle_kingside &&
				              move_type != move_type::castle_queenside)
				{
					copy_mask |= 0x00'00'FF'FF'00'00'00'00;
				}
			}

			return copy_mask;
		}

		void set_ep_capture() { board_state |= 1ull << en_passant_offset; }
		void set_white_can_castle_ks(const uint16_t arg) { board_state |= arg << white_can_castle_ks_offset; }
		void set_white_can_castle_qs(const uint16_t arg) { board_state |= arg << white_can_castle_qs_offset; }
		void set_black_can_castle_ks(const uint16_t arg) { board_state |= arg << black_can_castle_ks_offset; }
		void set_black_can_castle_qs(const uint16_t arg) { board_state |= arg << black_can_castle_qs_offset; }
		void set_fifty_move_counter(const uint16_t arg) { board_state |= arg << fifty_move_counter_offset; }
		void set_in_check() { board_state |= 1ull << check_offset; }

		/*
		Return true if the moving player (moving_color) has just put the opponent into check.
		Return false otherwise.
		If the move was a promotion, `piece` is the promoted-to type.
		*/
		template <color moving_color, piece piece, move_type move_type, chess::piece promoted_piece>
		force_inline_toggle bool detect_check(const bitboard from, const bitboard to, const move_info& move_info) const
		{
			bitboard checkers{};

			if constexpr (piece == pawn)
			{
				checkers |= to & move_info.pawn_check_squares;
			}

			if constexpr (piece == knight)
			{
				checkers |= to & move_info.knight_check_squares;
			}

			if constexpr (piece == bishop || piece == queen)
			{
				checkers |= to & move_info.bishop_check_squares;
			}

			if constexpr (piece == rook || piece == queen)
			{
				checkers |= to & move_info.rook_check_squares;
			}

			if constexpr (move_type == move_type::castle_kingside || move_type == move_type::castle_queenside)
			{
				constexpr size_t white_castling = (moving_color == white ? 56 : 0);
				constexpr size_t castle_kingside = (move_type == move_type::castle_kingside ? 2 : 0);
				constexpr bitboard rook_end_bb = 1ull << (3 + white_castling + castle_kingside);

				checkers |= rook_end_bb & get_slider_moves<rook>(bitboards, move_info.opp_king_idx);
			}
			else if (from & move_info.discovery_blockers || move_type == move_type::en_passant_capture)
			{
				bitboard bishop_checkers{};
				bitboard rook_checkers{};

				if constexpr (promoted_piece == bishop || promoted_piece == queen)
				{
					bishop_checkers = to;
				}
				if constexpr (promoted_piece == rook || promoted_piece == queen)
				{
					rook_checkers = to;
				}

				const bitboard bishop_check_squares = get_slider_moves<bishop>(bitboards, move_info.opp_king_idx);
				checkers |= (move_info.bishops_and_queens | bishop_checkers) & bishop_check_squares;

				const bitboard rook_check_squares = get_slider_moves<rook>(bitboards, move_info.opp_king_idx);
				checkers |= (move_info.rooks_and_queens | rook_checkers) & rook_check_squares;
			}

			return checkers;
		}

		template <color update_color>
		inline_toggle_member void update_key_castling_rights_for(tt_key& incremental_key, const board& parent_board)
		{
			if constexpr (update_color == white)
			{
				if (white_can_castle_ks() != parent_board.white_can_castle_ks()) incremental_key ^= w_castle_ks_key();
				if (white_can_castle_qs() != parent_board.white_can_castle_qs()) incremental_key ^= w_castle_qs_key();
			}
			else
			{
				if (black_can_castle_ks() != parent_board.black_can_castle_ks()) incremental_key ^= b_castle_ks_key();
				if (black_can_castle_qs() != parent_board.black_can_castle_qs()) incremental_key ^= b_castle_qs_key();
			}
		}

		template <bool gen_key = true, bool gen_phase = true, bool gen_eval = true>
		void generate_key_phase_eval(const color color_to_move)
		{
			tt_key new_key = 0;

			phase_t new_phase = 0;
			eval_t new_persistent_eval = 0;
			eval_t new_mg_eval = 0;
			eval_t new_eg_eval = 0;

			auto iterate_squares = [&]<color color, piece piece>()
			{
				bitboard bb = bitboards.get<color, piece>();

				if constexpr (piece != king)
				{
					const auto count = ::util::popcount(bb);
					if constexpr (gen_phase) new_phase += count * eval::phase_weights[piece];
					if constexpr (gen_eval)
						for (size_t i = 0; i < count; ++i)
							new_persistent_eval += eval::piece_count_eval<color>(piece, i);
				}

				while (bb)
				{
					const size_t piece_idx = get_next_bit_index(bb);
					bb = clear_next_bit(bb);

					if constexpr (gen_key) new_key ^= piece_square_key<color, piece>(piece_idx);
					if constexpr (gen_eval) new_mg_eval += eval::piece_square_eval_mg<color, piece>(piece_idx);
					if constexpr (gen_eval) new_eg_eval += eval::piece_square_eval_eg<color, piece>(piece_idx);
				}

				if constexpr (gen_eval && (piece == pawn || piece == rook))
				{
					for (file file = 0; file < 8; ++file)
					{
						const auto count = bitboards.file_count<color, piece>(file);
						for (size_t i = 0; i < count; ++i)
							new_persistent_eval += eval::file_piece_count_eval<color, piece>(i);
					}
				}
			};

			auto iterate_pieces = [=]<color color>()
			{
				iterate_squares.template operator()<color, pawn>();
				iterate_squares.template operator()<color, knight>();
				iterate_squares.template operator()<color, bishop>();
				iterate_squares.template operator()<color, rook>();
				iterate_squares.template operator()<color, queen>();
				iterate_squares.template operator()<color, king>();
			};

			iterate_pieces.template operator()<white>();
			iterate_pieces.template operator()<black>();

			if constexpr (gen_key)
			{
				if (can_capture_ep())
				{
					new_key ^= en_passant_key(move.get_end_file());
				}

				if (color_to_move == black)
				{
					new_key ^= black_to_move_key();
				}

				new_key ^= (w_castle_ks_key() * white_can_castle_ks());
				new_key ^= (w_castle_qs_key() * white_can_castle_qs());
				new_key ^= (b_castle_ks_key() * black_can_castle_ks());
				new_key ^= (b_castle_qs_key() * black_can_castle_qs());

				key = new_key;
			}

			if constexpr (gen_phase)
			{
				phase = new_phase;
			}

			if constexpr (gen_eval)
			{
				mg_eval = new_mg_eval;
				eg_eval = new_eg_eval;
				persistent_eval = new_persistent_eval;
				eval = new_persistent_eval + taper(phase, new_mg_eval, new_eg_eval);
			}
		}

		// bitfield sizes
		static constexpr size_t en_passant_bits = 1;
		static constexpr size_t castling_right_bits = 1;
		static constexpr size_t fifty_move_counter_bits = std::bit_width(50u * 2);
		static constexpr size_t check_bits = 1;

		// bitfield positions
		static constexpr size_t en_passant_offset = 0;
		static constexpr size_t white_can_castle_ks_offset = en_passant_offset + en_passant_bits;
		static constexpr size_t white_can_castle_qs_offset = white_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t black_can_castle_ks_offset = white_can_castle_qs_offset + castling_right_bits;
		static constexpr size_t black_can_castle_qs_offset = black_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t fifty_move_counter_offset =
		    black_can_castle_qs_offset + castling_right_bits; // counts ply
		static constexpr size_t check_offset = fifty_move_counter_offset + fifty_move_counter_bits;

		// bitfield masks, not including offsets
		static constexpr uint64_t en_passant_mask = (1ull << en_passant_bits) - 1;
		static constexpr uint64_t castling_right_mask = (1ull << castling_right_bits) - 1;
		static constexpr uint64_t fifty_move_counter_mask = (1ull << fifty_move_counter_bits) - 1;
		static constexpr uint64_t check_mask = (1ull << check_bits) - 1;

		// Check that we're using the expected number of bits.
		static_assert(check_bits + check_offset + 3 == 16);

		tt_key key{};
		bitboards bitboards{};

		move move{};
		eval_t mg_eval{};
		eval_t eg_eval{};
		eval_t eval{};

		uint16_t board_state{};
		uint16_t phase{};
		eval_t persistent_eval{};
	};
}
