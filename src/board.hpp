#pragma once

#include <array>
#include <bit>
#include <variant>
#include <vector>

#include "bitboard.hpp"
#include "config.hpp"
#include "defines.hpp"
#include "evaluation.hpp"
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
	constexpr size_t boards_size = eval::max_ply * max_n_of_moves;
	extern std::array<board, boards_size> boards;

	inline constexpr size_t first_child_index(const size_t parent_index)
	{
		static_assert(std::popcount(max_n_of_moves) == 1);
		constexpr size_t ply_mask = ~(max_n_of_moves - 1);

		return (parent_index + max_n_of_moves) & ply_mask;
	}

	inline eval_t taper(const phase_t phase, const int64_t mg_eval, const int64_t eg_eval)
	{
		return (mg_eval * phase + eg_eval * (eval::total_phase - phase)) / eval::total_phase;
	}

	enum class gen
	{
		key_phase_eval,
		eval
	};

	class board
	{
	public:
		board() {}

		// Simple, nonvalidating FEN parser
		color load_fen(const std::string& fen);
		void set_last_moved_info(const color color_to_move);
		void generate_eval();
		void verify_key_and_eval(const color color_to_move);

		template <color moving_color, piece piece, move_type move_type, chess::piece promoted_piece>
		inline_toggle_member void copy_make_bitboards(
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

				uint32_t lo = _mm256_movemask_pd(_mm256_cmpeq_epi64(bb_lo, old_lo));
				uint32_t hi = _mm256_movemask_pd(_mm256_cmpeq_epi64(bb_hi, old_hi));
				uint32_t masks = (hi << 4) | lo;            // Combine.
				masks ^= 0b1111'1111;                       // Invert the byte we care about.
				masks >>= 2;                                // Remove the two color bits.
				captured_piece = get_next_bit_index(masks); // Get 0..4 for pawn..queen.
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
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b01'00'01'00); // 0b01 is empty
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

		template <color moving_color, piece piece, move_type move_type, chess::piece promoted_piece>
		inline_toggle_member void copy_make_board(const board& parent_board, tt_key incremental_key,
		    const bitboard from, const bitboard to, const chess::piece captured_piece)
		{
			// Generate a mask at compile time to selectively copy parent state.
			constexpr uint32_t copy_mask = get_copy_mask<moving_color, piece, move_type, promoted_piece>();
			uint32_t bitfield = parent_board.board_state & copy_mask;

			// Record the move that resulted in this position.
			const size_t start_idx = get_next_bit_index(from);
			const size_t end_idx = get_next_bit_index(to);
			bitfield |= start_idx;
			bitfield |= end_idx << end_file_offset;

			// Record the type of piece that was moved, or in the case of promotion,
			// record the promoted-to type.
			if constexpr (promoted_piece == empty)
			{
				bitfield |= uint32_t(piece) << moved_piece_offset;
			}
			else
			{
				bitfield |= uint32_t(promoted_piece) << moved_piece_offset;
				bitfield |= 1 << promotion_offset;
			}

			// If the move is not a capture or pawn move, increment the fifty-move counter.
			if constexpr (move_type != move_type::capture && piece != pawn)
			{
				bitfield += 1 << fifty_move_counter_offset;
			}

			// Record any en passant right for the opponent.
			if constexpr (move_type == move_type::pawn_two_squares)
			{
				bitfield |= (start_idx % 8) << en_passant_file_offset;
			}
			else
			{
				bitfield |= uint32_t(no_ep_file) << en_passant_file_offset;
			}

			// If a rook moves, it cannot be used to castle.
			// Update castling rights for the moving player.
			if constexpr (piece == rook)
			{
				const rank start_rank = start_idx / 8;
				const file start_file = start_idx % 8;
				if constexpr (moving_color == white)
				{
					if (start_rank == 7)
					{
						if (start_file == 0)
							bitfield &= ~(1 << white_can_castle_qs_offset);
						else if (start_file == 7)
							bitfield &= ~(1 << white_can_castle_ks_offset);
					}
				}
				else
				{
					if (start_rank == 0)
					{
						if (start_file == 0)
							bitfield &= ~(1 << black_can_castle_qs_offset);
						else if (start_file == 7)
							bitfield &= ~(1 << black_can_castle_ks_offset);
					}
				}
			}

			// If a rook is captured, it cannot be used to castle.
			// Update castling rights for the opponent.
			if constexpr (move_type == move_type::capture)
			{
				const rank end_rank = end_idx / 8;
				const file end_file = end_idx % 8;
				if constexpr (moving_color == white)
				{
					if (end_rank == 0)
					{
						if (end_file == 0)
							bitfield &= ~(1 << black_can_castle_qs_offset);
						else if (end_file == 7)
							bitfield &= ~(1 << black_can_castle_ks_offset);
					}
				}
				else
				{
					if (end_rank == 7)
					{
						if (end_file == 0)
							bitfield &= ~(1 << white_can_castle_qs_offset);
						else if (end_file == 7)
							bitfield &= ~(1 << white_can_castle_ks_offset);
					}
				}
			}

			board_state = bitfield;

			// The incremental_key has already had the leaving piece removed, and the color to move toggled.
			// We receive it by copy so we can modify it before writing it.

			constexpr chess::piece piece_after = (promoted_piece == empty) ? piece : promoted_piece;

			eval_t incremental_mg_eval = parent_board.get_mg_eval();
			eval_t incremental_eg_eval = parent_board.get_eg_eval();

			if constexpr (promoted_piece != empty)
			{
				incremental_mg_eval -= eval::piece_eval<moving_color, piece>();
				incremental_eg_eval -= eval::piece_eval<moving_color, piece>();
				incremental_mg_eval += eval::piece_eval<moving_color, piece_after>();
				incremental_eg_eval += eval::piece_eval<moving_color, piece_after>();
			}

			// add the key for the arriving piece
			incremental_key ^= piece_square_key<moving_color, piece_after>(end_idx);

			// update square eval for the piece
			incremental_mg_eval -= eval::piece_square_eval_mg<moving_color, piece>(start_idx);
			incremental_eg_eval -= eval::piece_square_eval_eg<moving_color, piece>(start_idx);
			incremental_mg_eval += eval::piece_square_eval_mg<moving_color, piece_after>(end_idx);
			incremental_eg_eval += eval::piece_square_eval_eg<moving_color, piece_after>(end_idx);

			constexpr color opp_color = other_color(moving_color);

			if constexpr (move_type == move_type::pawn_two_squares)
			{
				// add en passant rights for the opponent
				incremental_key ^= en_passant_key(end_idx % 8);
			}
			else if constexpr (move_type == move_type::en_passant_capture)
			{
				const size_t captured_pawn_idx = end_idx + ((moving_color == white) ? 8 : -8);
				// remove key for the captured pawn
				incremental_key ^= piece_square_key<opp_color, pawn>(captured_pawn_idx);
				// udpate eval for the captured pawn
				incremental_mg_eval -= eval::piece_eval<opp_color, pawn>();
				incremental_eg_eval -= eval::piece_eval<opp_color, pawn>();
				incremental_mg_eval -= eval::piece_square_eval_mg<opp_color, pawn>(captured_pawn_idx);
				incremental_eg_eval -= eval::piece_square_eval_eg<opp_color, pawn>(captured_pawn_idx);
			}
			else if constexpr (move_type == move_type::castle_kingside || move_type == move_type::castle_queenside)
			{
				constexpr size_t rook_start_file = (move_type == move_type::castle_kingside ? 7 : 0);
				constexpr size_t rook_start_rank = (moving_color == white ? 7 : 0);
				constexpr size_t rook_start_index = rook_start_rank * 8 + rook_start_file;
				constexpr size_t rook_end_index = rook_start_index + (move_type == move_type::castle_kingside ? -2 : 3);

				// update keys for the moving rook
				incremental_key ^= piece_square_key<moving_color, rook>(rook_start_index);
				incremental_key ^= piece_square_key<moving_color, rook>(rook_end_index);
				// update eval for the moving rook
				incremental_mg_eval -= eval::piece_square_eval_mg<moving_color, rook>(rook_start_index);
				incremental_eg_eval -= eval::piece_square_eval_eg<moving_color, rook>(rook_start_index);
				incremental_mg_eval += eval::piece_square_eval_mg<moving_color, rook>(rook_end_index);
				incremental_eg_eval += eval::piece_square_eval_eg<moving_color, rook>(rook_end_index);
			}
			else if constexpr (move_type == move_type::capture) // non-en passant capture
			{
				// remove the key for the captured piece
				incremental_key ^= piece_square_key<opp_color>(captured_piece, end_idx);
				// update our opponent's castling rights
				update_key_castling_rights_for<opp_color>(incremental_key, parent_board);
				// update eval for the captured piece
				incremental_mg_eval -= eval::piece_eval<opp_color>(captured_piece);
				incremental_eg_eval -= eval::piece_eval<opp_color>(captured_piece);
				incremental_mg_eval -= eval::piece_square_eval_mg<opp_color>(captured_piece, end_idx);
				incremental_eg_eval -= eval::piece_square_eval_eg<opp_color>(captured_piece, end_idx);
			}

			if constexpr (piece == king || piece == rook)
			{
				// a king or rook is moving; update our castling rights
				update_key_castling_rights_for<moving_color>(incremental_key, parent_board);
			}

			key = incremental_key;

			phase_t incremental_phase = parent_board.get_phase();

			if constexpr (move_type == move_type::capture)
			{
				incremental_phase -= eval::phase_weights[captured_piece];
			}
			else if constexpr (move_type == move_type::en_passant_capture)
			{
				incremental_phase -= eval::phase_weights[pawn];
			}

			if constexpr (promoted_piece != empty)
			{
				incremental_phase -= eval::phase_weights[pawn];
				incremental_phase += eval::phase_weights[promoted_piece];
			}

			eval = taper(incremental_phase, incremental_mg_eval, incremental_eg_eval);

			phase = incremental_phase;
			mg_eval = incremental_mg_eval;
			eg_eval = incremental_eg_eval;
		}

		tt_key get_key() const { return key; }
		eval_t get_mg_eval() const { return mg_eval; }
		eval_t get_eg_eval() const { return eg_eval; }
		uint16_t get_phase() const { return phase; }
		eval_t get_eval() const { return eval; }

		packed_move get_packed_move() const { return board_state & packed_move(-1); }
		rank get_start_rank() const { return (board_state >> start_rank_offset) & square_mask; }
		file get_start_file() const { return (board_state >> start_file_offset) & square_mask; }
		rank get_end_rank() const { return (board_state >> end_rank_offset) & square_mask; }
		file get_end_file() const { return (board_state >> end_file_offset) & square_mask; }
		piece get_moved_piece() const { return (board_state >> moved_piece_offset) & moved_piece_mask; }
		file get_en_passant_file() const { return (board_state >> en_passant_file_offset) & en_passant_mask; }
		bool white_can_castle_ks() const { return (board_state >> white_can_castle_ks_offset) & castling_right_mask; }
		bool white_can_castle_qs() const { return (board_state >> white_can_castle_qs_offset) & castling_right_mask; }
		bool black_can_castle_ks() const { return (board_state >> black_can_castle_ks_offset) & castling_right_mask; }
		bool black_can_castle_qs() const { return (board_state >> black_can_castle_qs_offset) & castling_right_mask; }
		size_t get_fifty_move_counter() const
		{
			return (board_state >> fifty_move_counter_offset) & fifty_move_counter_mask;
		}
		const class bitboards& get_bitboards() const { return bitboards; }

		bool move_is(const packed_move best_move) const { return best_move == get_packed_move(); }

		const std::string move_to_string() const
		{
			std::string result;
			result += get_start_file() + 'a';
			result += (get_start_rank() * -1) + 8 + '0';
			result += get_end_file() + 'a';
			result += (get_end_rank() * -1) + 8 + '0';

			if (is_promotion())
			{
				switch (get_moved_piece()) // Append one of "qrbk".
				{
				case knight: result += 'n'; break;
				case bishop: result += 'b'; break;
				case rook: result += 'r'; break;
				case queen: result += 'q'; break;
				default: result += '?';
				}
			}

			return result;
		}

		void set_end_index(const size_t idx) { board_state |= uint32_t(idx) << end_file_offset; }

		template <piece piece>
		void set_moved_piece()
		{
			board_state |= uint32_t(piece) << moved_piece_offset;
		}

	private:
		bool is_promotion() const { return (board_state >> promotion_offset) & promotion_bits; }

		template <color moving_color, piece piece, move_type move_type, chess::piece promoted_piece>
		consteval static uint32_t get_copy_mask()
		{
			uint32_t copy_mask = 0;

			// Always copy the opponent's castling rights.
			if constexpr (moving_color == white)
			{
				copy_mask |= 1 << black_can_castle_ks_offset;
				copy_mask |= 1 << black_can_castle_qs_offset;
			}
			else
			{
				copy_mask |= 1 << white_can_castle_ks_offset;
				copy_mask |= 1 << white_can_castle_qs_offset;
			}

			// If the move is not a king move, copy the moving player's castling rights.
			if constexpr (piece != king)
			{
				if constexpr (moving_color == white)
				{
					copy_mask |= 1 << white_can_castle_ks_offset;
					copy_mask |= 1 << white_can_castle_qs_offset;
				}
				else
				{
					copy_mask |= 1 << black_can_castle_ks_offset;
					copy_mask |= 1 << black_can_castle_qs_offset;
				}
			}

			// If the move is not a capture or pawn move, copy the fifty-move counter.
			if constexpr (move_type != move_type::capture && piece != pawn)
			{
				copy_mask |= fifty_move_counter_mask << fifty_move_counter_offset;
			}

			return copy_mask;
		}

		void set_en_passant_file(const file file) { board_state |= uint32_t(file) << en_passant_file_offset; }
		void set_white_can_castle_ks(const uint32_t arg) { board_state |= arg << white_can_castle_ks_offset; }
		void set_white_can_castle_qs(const uint32_t arg) { board_state |= arg << white_can_castle_qs_offset; }
		void set_black_can_castle_ks(const uint32_t arg) { board_state |= arg << black_can_castle_ks_offset; }
		void set_black_can_castle_qs(const uint32_t arg) { board_state |= arg << black_can_castle_qs_offset; }
		void white_cant_castle_ks() { board_state &= ~(1 << white_can_castle_ks_offset); }
		void white_cant_castle_qs() { board_state &= ~(1 << white_can_castle_qs_offset); }
		void black_cant_castle_ks() { board_state &= ~(1 << black_can_castle_ks_offset); }
		void black_cant_castle_qs() { board_state &= ~(1 << black_can_castle_qs_offset); }
		void set_fifty_move_counter(const uint32_t arg) { board_state |= arg << fifty_move_counter_offset; }

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

		template <bool gen_key = true, bool gen_eval = true, bool gen_phase = true>
		void generate_key_eval_phase(const color color_to_move)
		{
			tt_key new_key = 0;

			phase_t new_phase = 0;
			eval_t new_mg_eval = 0;
			eval_t new_eg_eval = 0;

			auto iterate_squares = [&]<color color, piece piece>()
			{
				bitboard bb = bitboards.get<color, piece>();

				if constexpr (piece != king)
				{
					const auto count = ::util::popcount(bb);
					if constexpr (gen_phase) new_phase += count * eval::phase_weights[piece];
					if constexpr (gen_eval) new_mg_eval += count * eval::piece_eval<color>(piece);
					if constexpr (gen_eval) new_eg_eval += count * eval::piece_eval<color>(piece);
				}

				while (bb)
				{
					const size_t piece_idx = get_next_bit_index(bb);
					bb = clear_next_bit(bb);

					if constexpr (gen_key) new_key ^= piece_square_key<color, piece>(piece_idx);
					if constexpr (gen_eval) new_mg_eval += eval::piece_square_eval_mg<color, piece>(piece_idx);
					if constexpr (gen_eval) new_eg_eval += eval::piece_square_eval_eg<color, piece>(piece_idx);
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
				const file ep_file = get_en_passant_file();
				if (ep_file != no_ep_file)
				{
					new_key ^= en_passant_key(ep_file);
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
				eval = taper(phase, new_mg_eval, new_eg_eval);
			}
		}

		// bitfield sizes
		static constexpr size_t square_bits = 3;
		static constexpr size_t moved_piece_bits = 3;
		static constexpr size_t promotion_bits = 1;
		static constexpr size_t en_passant_bits = 4;
		static constexpr size_t castling_right_bits = 1;
		static constexpr size_t fifty_move_counter_bits = std::bit_width(50u * 2);

		// bitfield positions
		static constexpr size_t start_file_offset = 0;
		static constexpr size_t start_rank_offset =
		    start_file_offset + square_bits; // the move that resulted in this position
		static constexpr size_t end_file_offset = start_rank_offset + square_bits;
		static constexpr size_t end_rank_offset = end_file_offset + square_bits;
		static constexpr size_t moved_piece_offset = end_rank_offset + square_bits;
		static constexpr size_t promotion_offset = moved_piece_offset + moved_piece_bits;
		static constexpr size_t en_passant_file_offset = promotion_offset + promotion_bits;
		static constexpr size_t white_can_castle_ks_offset = en_passant_file_offset + en_passant_bits;
		static constexpr size_t white_can_castle_qs_offset = white_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t black_can_castle_ks_offset = white_can_castle_qs_offset + castling_right_bits;
		static constexpr size_t black_can_castle_qs_offset = black_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t fifty_move_counter_offset =
		    black_can_castle_qs_offset + castling_right_bits; // counts ply

		// bitfield masks, not including offsets
		static constexpr uint64_t square_mask = (1ull << square_bits) - 1;
		static constexpr uint64_t index_mask = (square_mask << square_bits) | square_mask;
		static constexpr uint64_t moved_piece_mask = (1ull << moved_piece_bits) - 1;
		static constexpr uint64_t promotion_mask = (1ull << promotion_bits) - 1;
		static constexpr uint64_t en_passant_mask = (1ull << en_passant_bits) - 1;
		static constexpr uint64_t castling_right_mask = (1ull << castling_right_bits) - 1;
		static constexpr uint64_t fifty_move_counter_mask = (1ull << fifty_move_counter_bits) - 1;

		// Check that we're using the expected number of bits.
		static_assert(fifty_move_counter_bits + fifty_move_counter_offset + 1 == 32);

		tt_key key{};
		bitboards bitboards{};
		uint32_t board_state{};

		eval_t mg_eval{};
		eval_t eg_eval{};
		uint16_t phase{};
		eval_t eval{};
	};
}
