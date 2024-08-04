#pragma once

#include <array>
#include <bit>
#include <variant>
#include <vector>

#include "bitboard.hpp"
#include "config.hpp"
#include "evaluation.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "types.hpp"

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

	extern std::array<board, positions_size> boards;

	class board
	{
	public:
		board() {}
		explicit board(const position& loaded_position, const color_t color_to_move,
					   const bool w_castle_ks, const bool w_castle_qs, const bool b_castle_ks, const bool b_castle_qs,
					   const file en_passant_file, const int8_t fifty_move_counter);

		template <color_t color_to_move, piece_t moving_piece_type, move_type move_type, piece_t promotion_type>
		force_inline_directive void update_bitboards(const size_t parent_idx,
													 const bitboard leaving_bit, const bitboard arriving_bit)
		{
			constexpr color_t moved_color = other_color(color_to_move);

			const class bitboards& parent_bbs = boards[parent_idx].get_bitboards();

			uint256_t bb_lo = _mm256_loadu_si256((uint256_t*)&parent_bbs);
			uint256_t bb_hi = _mm256_loadu_si256(((uint256_t*)&parent_bbs) + 1);

			// 8 bitboards require two ymm vector registers:
			// [    0,     1,     2,       3] [      4,     5,      6,     7]
			// [white, black, pawns, knights] [bishops, rooks, queens, kings]

			// Remove the captured bit. We don't know what type of piece was captured,
			// so clear the bit across all eight bitboards.
			if constexpr (move_type == move_type::capture)
			{
				const uint256_t clear_bits = _mm256_set1_epi64x(arriving_bit);
				bb_lo = _mm256_andnot_si256(clear_bits, bb_lo);
				bb_hi = _mm256_andnot_si256(clear_bits, bb_hi);
			}

			if constexpr (promotion_type == empty)
			{
				uint256_t toggle_bits = _mm256_setzero_si256();
				toggle_bits = _mm256_insert_epi64(toggle_bits, leaving_bit | arriving_bit, 0);

				if constexpr (moving_piece_type == pawn || moving_piece_type == knight)
				{
					// If a white pawn is moving, XOR at 0 and 2.
					// If a white knight is moving, XOR at 0 and 3.
					// If a black pawn is moving, XOR at 1 and 2.
					// If a black knight is moving, XOR at 1 and 3.
					if constexpr (moved_color == white && moving_piece_type == pawn)
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b01'00'01'00); // 0b01 is empty
					else if constexpr (moved_color == white && moving_piece_type == knight)
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b00'01'01'00);
					else if constexpr (moved_color == black && moving_piece_type == pawn)
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b01'00'00'01);
					else // A black knight is moving.
						toggle_bits = _mm256_permute4x64_epi64(toggle_bits, 0b00'01'00'01);

					bb_lo = _mm256_xor_si256(bb_lo, toggle_bits);
				}
				else // A rook, bishop, queen, or king is moving.
				{
					// XOR at [0] or [1] for the color bitboard.
					if constexpr (moved_color == white)
						bb_lo = _mm256_xor_si256(bb_lo, toggle_bits);
					else
						bb_lo = _mm256_xor_si256(bb_lo, _mm256_permute4x64_epi64(toggle_bits, 0b01'01'00'01));

					// XOR at one of [4-7] for the moving bishop/rook/queen/king.
					if constexpr (moving_piece_type == bishop)
						bb_hi = _mm256_xor_si256(bb_hi, toggle_bits);
					else if constexpr (moving_piece_type == rook)
						bb_hi = _mm256_xor_si256(bb_hi, _mm256_permute4x64_epi64(toggle_bits, 0b01'01'00'01));
					else if constexpr (moving_piece_type == queen)
						bb_hi = _mm256_xor_si256(bb_hi, _mm256_permute4x64_epi64(toggle_bits, 0b01'00'01'01));
					else // A king is moving.
						bb_hi = _mm256_xor_si256(bb_hi, _mm256_permute4x64_epi64(toggle_bits, 0b00'01'01'01));
				}
			}

			_mm256_storeu_si256(((uint256_t*)&bitboards), bb_lo);
			_mm256_storeu_si256(((uint256_t*)&bitboards) + 1, bb_hi);

			bitboard our_pieces = bitboards.get<moved_color>();
			bitboard opp_pieces = bitboards.get<color_to_move>();

			if constexpr (promotion_type != empty)
			{
				our_pieces ^= leaving_bit | arriving_bit;

				bitboards.pawns ^= leaving_bit;

				if constexpr (promotion_type == knight)
					bitboards.knights |= arriving_bit;
				else if constexpr (promotion_type == bishop)
					bitboards.bishops |= arriving_bit;
				else if constexpr (promotion_type == rook)
					bitboards.rooks |= arriving_bit;
				else if constexpr (promotion_type == queen)
					bitboards.queens |= arriving_bit;
			}

			// For en passant captures, remove two additional bits.
			if constexpr (move_type == move_type::en_passant_capture)
			{
				const bitboard captured_pawn_bit = ((moved_color == white) ? arriving_bit << 8 : arriving_bit >> 8);
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
				constexpr bitboard invert_bits = 0b1010'0000uz << ((moved_color == white) ? 56 : 0);
				bitboards.rooks ^= invert_bits;
				our_pieces ^= invert_bits;
			}
			else if constexpr (move_type == move_type::castle_queenside)
			{
				constexpr bitboard invert_bits = 0b1001uz << ((moved_color == white) ? 56 : 0);
				bitboards.rooks ^= invert_bits;
				our_pieces ^= invert_bits;
			}

			bitboards.white = (moved_color == white) ? our_pieces : opp_pieces;
			bitboards.black = (moved_color == white) ? opp_pieces : our_pieces;
		}

		template<color_t color_to_move, piece_t moving_piece_type, move_type move_type, piece_t promotion_type>
		force_inline_directive static void make_board(const size_t child_idx, const size_t parent_idx,
													  const bitboard start, const bitboard end)
		{
			// Generate a mask at compile time to selectively copy parent state.
			constexpr uint32_t copy_mask = get_copy_mask<color_to_move, moving_piece_type, move_type, promotion_type>();
			uint32_t bitfield = boards[parent_idx].board_state & copy_mask;

			// Record the move that resulted in this position.
			const size_t start_idx = get_next_bit_index(start);
			const size_t end_idx = get_next_bit_index(end);
			bitfield |= start_idx;
			bitfield |= end_idx << end_file_offset;

			// Record the type of piece that was moved, or in the case of promotion,
			// record the promoted-to type.
			if constexpr (promotion_type == empty)
			{
				bitfield |= moving_piece_type << (moved_piece_offset - 1);
			}
			else
			{
				bitfield |= promotion_type << (moved_piece_offset - 1);
				bitfield |= 1ull << promotion_offset;
			}

			// If the move is not a capture or pawn move, increment the fifty-move counter.
			if constexpr (move_type != move_type::capture && moving_piece_type != pawn)
			{
				bitfield += 1ull << fifty_move_counter_offset;
			}

			// Record any en passant right for the opponent.
			if constexpr (move_type == move_type::pawn_two_squares)
			{
				bitfield |= (start_idx % 8) << en_passant_file_offset;
			}
			else
			{
				bitfield |= empty << en_passant_file_offset;
			}

			// If a rook moves, it cannot be used to castle.
			// Update castling rights for the moving player.
			constexpr color_t moving_color = other_color(color_to_move);
			if constexpr (moving_piece_type == rook)
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
			if constexpr (move_type == move_type::capture)
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

			boards[child_idx].board_state = bitfield;
		}

		template <color_t moving_color, piece_t moving_piece_type, move_type move_type, piece_t promotion_type>
		inline_toggle_member void update_key_and_eval(const position& parent_position, const board& parent_board, tt::key incremental_key)
		{
			// The incremental_key has already had the leaving piece removed, and the color to move toggled.
			// We receive it by copy so we can modify it before writing it.

			const rank start_rank = get_start_rank();
			const file start_file = get_start_file();
			const rank end_rank = get_end_rank();
			const file end_file = get_end_file();

			const size_t start_idx = to_index(start_rank, start_file);
			const size_t end_idx = to_index(end_rank, end_file);

			eval_t incremental_eval = parent_board.get_eval();

			constexpr piece_t piece_before = moving_piece_type | moving_color;
			constexpr piece_t piece_after = (promotion_type == empty) ? piece_before : (moving_color | promotion_type);

			if constexpr (promotion_type != empty)
			{
				incremental_eval -= eval::piece_eval(piece_before);
				incremental_eval += eval::piece_eval(piece_after);
			}

			// add the key for the arriving piece
			incremental_key ^= tt::z_keys.piece_square_keys[piece_after][end_idx];
			// update square eval for the piece
			incremental_eval -= eval::piece_square_eval(piece_before, start_idx);
			incremental_eval += eval::piece_square_eval(piece_after, end_idx);

			if constexpr (move_type == move_type::pawn_two_squares)
			{
				// add en passant rights for the opponent
				incremental_key ^= tt::z_keys.en_passant_keys[end_file];
			}
			else if (move_type == move_type::en_passant_capture)
			{
				const size_t captured_pawn_idx = end_idx + ((moving_color == white) ? 8 : -8);
				constexpr piece_t captured_pawn = other_color(moving_color) | pawn;
				// remove key for the captured pawn
				incremental_key ^= tt::z_keys.piece_square_keys[captured_pawn][captured_pawn_idx];
				// udpate eval for the captured pawn
				incremental_eval -= eval::piece_eval(captured_pawn);
				incremental_eval -= eval::piece_square_eval(captured_pawn, captured_pawn_idx);
			}
			else if (move_type == move_type::castle_kingside ||
					 move_type == move_type::castle_queenside)
			{
				constexpr size_t rook_start_file = (move_type == move_type::castle_kingside ? 7 : 0);
				constexpr size_t rook_start_rank = (moving_color == white ? 7 : 0);
				constexpr size_t rook_start_index = rook_start_rank * 8 + rook_start_file;
				constexpr size_t rook_end_index = rook_start_index + (move_type == move_type::castle_kingside ? -2 : 3);

				constexpr piece_t moving_rook = moving_color | rook;
				// update keys for the moving rook
				incremental_key ^= tt::z_keys.piece_square_keys[moving_rook][rook_start_index];
				incremental_key ^= tt::z_keys.piece_square_keys[moving_rook][rook_end_index];
				// update eval for the moving rook
				incremental_eval -= eval::piece_square_eval(moving_rook, rook_start_index);
				incremental_eval += eval::piece_square_eval(moving_rook, rook_end_index);
			}
			else if (move_type == move_type::capture) // non-en passant capture
			{
				const piece_t captured_piece = parent_position.piece_at(end_idx).value();
				// remove the key for the captured piece
				incremental_key ^= tt::z_keys.piece_square_keys[captured_piece][end_idx];
				// update our opponent's castling rights
				update_key_castling_rights_for<other_color(moving_color)>(incremental_key, parent_board);
				// update eval for the captured piece
				incremental_eval -= eval::piece_eval(captured_piece);
				incremental_eval -= eval::piece_square_eval(captured_piece, end_idx);
			}

			if constexpr (moving_piece_type == king ||
						  moving_piece_type == rook)
			{
				// a king or rook is moving; update our castling rights
				update_key_castling_rights_for<moving_color>(incremental_key, parent_board);
			}

			eval = incremental_eval;
			key = incremental_key;
		}

		tt::key get_key() const { return key; }
		eval_t get_eval() const { return eval; }
		packed_move get_packed_move() const { return board_state & packed_move(-1); }
		rank get_start_rank() const { return (board_state >> start_rank_offset) & square_mask; }
		file get_start_file() const { return (board_state >> start_file_offset) & square_mask; }
		rank get_end_rank() const { return (board_state >> end_rank_offset) & square_mask; }
		file get_end_file() const { return (board_state >> end_file_offset) & square_mask; }
		piece moved_piece_without_color() const { return ((board_state >> moved_piece_offset) & moved_piece_mask) << 1; }
		file get_en_passant_file() const { return (board_state >> en_passant_file_offset) & en_passant_mask; }
		bool white_can_castle_ks() const { return (board_state >> white_can_castle_ks_offset) & castling_right_mask; }
		bool white_can_castle_qs() const { return (board_state >> white_can_castle_qs_offset) & castling_right_mask; }
		bool black_can_castle_ks() const { return (board_state >> black_can_castle_ks_offset) & castling_right_mask; }
		bool black_can_castle_qs() const { return (board_state >> black_can_castle_qs_offset) & castling_right_mask; }
		size_t get_fifty_move_counter() const { return (board_state >> fifty_move_counter_offset) & fifty_move_counter_mask; }
		bool move_is(const packed_move best_move) const { return best_move == get_packed_move(); }
		const std::string move_to_string() const
		{
			std::string result;
			result += get_start_file().value() + 'a';
			result += (get_start_rank().value() * -1) + 8 + '0';
			result += get_end_file().value() + 'a';
			result += (get_end_rank().value() * -1) + 8 + '0';

			if (is_promotion())
			{
				// Append one of "qrbk".
				result += moved_piece_without_color().to_promoted_char();
			}

			return result;
		}
		const class bitboards& get_bitboards() const { return bitboards; }

		void set_end_index(size_t idx) { board_state |= uint32_t(idx) << end_file_offset; }
		void set_moved_piece(piece piece) { board_state |= uint32_t(piece.value() >> 1) << moved_piece_offset; }

	private:
		bool is_promotion() const { return (board_state >> promotion_offset) & promotion_bits; }

		void set_en_passant_file(file file) { board_state |= uint32_t(file.value()) << en_passant_file_offset; }
		void set_white_can_castle_ks(uint32_t arg) { board_state |= arg << white_can_castle_ks_offset; }
		void set_white_can_castle_qs(uint32_t arg) { board_state |= arg << white_can_castle_qs_offset; }
		void set_black_can_castle_ks(uint32_t arg) { board_state |= arg << black_can_castle_ks_offset; }
		void set_black_can_castle_qs(uint32_t arg) { board_state |= arg << black_can_castle_qs_offset; }
		void white_cant_castle_ks() { board_state &= ~(1 << white_can_castle_ks_offset); }
		void white_cant_castle_qs() { board_state &= ~(1 << white_can_castle_qs_offset); }
		void black_cant_castle_ks() { board_state &= ~(1 << black_can_castle_ks_offset); }
		void black_cant_castle_qs() { board_state &= ~(1 << black_can_castle_qs_offset); }
		void set_fifty_move_counter(uint32_t arg) { board_state |= arg << fifty_move_counter_offset; }

		template<color_t update_color>
		inline_toggle_member void update_key_castling_rights_for(tt::key& incremental_key, const board& parent_board)
		{
			if constexpr (update_color == white)
			{
				if (white_can_castle_ks() != parent_board.white_can_castle_ks())
					incremental_key ^= tt::z_keys.w_castle_ks;
				if (white_can_castle_qs() != parent_board.white_can_castle_qs())
					incremental_key ^= tt::z_keys.w_castle_qs;
			}
			else
			{
				if (black_can_castle_ks() != parent_board.black_can_castle_ks())
					incremental_key ^= tt::z_keys.b_castle_ks;
				if (black_can_castle_qs() != parent_board.black_can_castle_qs())
					incremental_key ^= tt::z_keys.b_castle_qs;
			}
		}

		template<color_t color_to_move, piece_t moving_piece_type, move_type move_type, piece_t promotion_type>
		consteval static uint32_t get_copy_mask()
		{
			constexpr color_t moving_color = other_color(color_to_move);

			uint32_t copy_mask = 0;

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
			if constexpr (moving_piece_type != king)
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
			if constexpr (moving_piece_type != pawn &&
						  move_type != move_type::capture)
			{
				copy_mask |= fifty_move_counter_mask << fifty_move_counter_offset;
			}

			return copy_mask;
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
		static constexpr size_t start_rank_offset = start_file_offset + square_bits; // the move that resulted in this position
		static constexpr size_t end_file_offset = start_rank_offset + square_bits;
		static constexpr size_t end_rank_offset = end_file_offset + square_bits;
		static constexpr size_t moved_piece_offset = end_rank_offset + square_bits;
		static constexpr size_t promotion_offset = moved_piece_offset + moved_piece_bits;
		static constexpr size_t en_passant_file_offset = promotion_offset + promotion_bits;
		static constexpr size_t white_can_castle_ks_offset = en_passant_file_offset + en_passant_bits;
		static constexpr size_t white_can_castle_qs_offset = white_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t black_can_castle_ks_offset = white_can_castle_qs_offset + castling_right_bits;
		static constexpr size_t black_can_castle_qs_offset = black_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t fifty_move_counter_offset = black_can_castle_qs_offset + castling_right_bits; // counts ply

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

		uint32_t board_state;
		eval_t eval;
		tt::key key;
		bitboards bitboards;
	};

	tt::key generate_key(const board& board, const position& position, const color_t color_to_move);
}
