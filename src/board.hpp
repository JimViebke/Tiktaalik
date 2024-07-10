#pragma once

#include <array>
#include <bit>
#include <variant>
#include <vector>

#include "capture.hpp"
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
	private:
		// bitfield sizes
		static constexpr size_t square_bits = 3;
		static constexpr size_t moved_piece_bits = 3;
		static constexpr size_t promotion_bits = 1;
		static constexpr size_t en_passant_bits = 4;
		static constexpr size_t castling_right_bits = 1;
		static constexpr size_t fifty_move_counter_bits = std::bit_width(50u * 2);
		static constexpr size_t terminal_bits = 1;

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
		static constexpr size_t fifty_move_counter_offset = black_can_castle_qs_offset + castling_right_bits; // counts to 100 ply
		static constexpr size_t terminal_offset = fifty_move_counter_offset + fifty_move_counter_bits;

		// bitfield masks, not including offsets
		static constexpr uint64_t square_mask = (1ull << square_bits) - 1;
		static constexpr uint64_t index_mask = (square_mask << square_bits) | square_mask;
		static constexpr uint64_t moved_piece_mask = (1ull << moved_piece_bits) - 1;
		static constexpr uint64_t promotion_mask = (1ull << promotion_bits) - 1;
		static constexpr uint64_t en_passant_mask = (1ull << en_passant_bits) - 1;
		static constexpr uint64_t castling_right_mask = (1ull << castling_right_bits) - 1;
		static constexpr uint64_t fifty_move_counter_mask = (1ull << fifty_move_counter_bits) - 1;
		static constexpr uint64_t terminal_mask = (1ull << terminal_bits) - 1;

		// make sure the required bitfield size is what we expect
		static_assert(terminal_bits + terminal_offset == 32);

		uint8_t board_state[4]{};

		uint32_t& bitfield() { return *(uint32_t*)&board_state[0]; }
		const uint32_t& bitfield() const { return *(uint32_t*)&board_state[0]; }

		eval_t eval = 0;
		eval_t static_eval = 0;

		tt::key key;

	public:
		board() { bitfield() = 0; }
		explicit board(const position& loaded_position, const color_t color_to_move,
					   const bool w_castle_ks, const bool w_castle_qs, const bool b_castle_ks, const bool b_castle_qs,
					   const file en_passant_file, const int8_t fifty_move_counter);

	private:
		explicit board(const size_t parent_idx,
					   const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			const piece moved_piece = positions[parent_idx].piece_at(start_rank, start_file);
			set_moved_piece(moved_piece);

			// record the move that resulted in this position
			set_start_rank(start_rank);
			set_start_file(start_file);
			set_end_rank(end_rank);
			set_end_file(end_file);

			// copy castling rights
			const board& parent_board = boards[parent_idx];
			set_white_can_castle_ks(parent_board.white_can_castle_ks());
			set_white_can_castle_qs(parent_board.white_can_castle_qs());
			set_black_can_castle_ks(parent_board.black_can_castle_ks());
			set_black_can_castle_qs(parent_board.black_can_castle_qs());
		}
		explicit board(const size_t parent_idx,
					   const rank start_rank, const file start_file, const rank end_rank, const file end_file,
					   const piece promote_to) // call this constructor for each pawn promotion
			: board(parent_idx, start_rank, start_file, end_rank, end_file)
		{
			set_moved_piece(promote_to);
			set_promotion();
		}

		void set_en_passant_file(const size_t, const rank, const file start_file, const rank, const file)
		{
			set_en_passant_file(start_file);
		}

		template<color_t color>
		void update_castling_rights_for(const rank rank, const file file)
		{
			// rank and file might be start or end idx.
			if constexpr (color == white)
			{
				if (rank == 7)
				{
					if (file == 0)
						white_cant_castle_qs();
					else if (file == 7)
						white_cant_castle_ks();
				}
			}
			else
			{
				if (rank == 0)
				{
					if (file == 0)
						black_cant_castle_qs();
					else if (file == 7)
						black_cant_castle_ks();
				}
			}
		}

		template<color_t moving_color>
		inline_toggle_member void update_castling_rights_for_moving_player(const size_t, const rank start_rank, const file start_file, const rank, const file)
		{
			// if a rook moves, it cannot be used to castle
			update_castling_rights_for<moving_color>(start_rank, start_file);
		}
		template<color_t moving_color>
		void update_castling_rights_for_moving_player(const size_t parent_idx,
													  const rank start_rank, const file start_file, const rank end_rank, const file end_file,
													  const piece)
		{
			update_castling_rights_for_moving_player<moving_color>(parent_idx, start_rank, start_file, end_rank, end_file);
		}

		template<color_t nonmoving_color>
		inline_toggle_member void update_castling_rights_for_nonmoving_player(const size_t,
																			  const rank, const file, const rank end_rank, const file end_file)
		{
			// if a rook is captured, it cannot be used to castle
			update_castling_rights_for<nonmoving_color>(end_rank, end_file);
		}
		template<color_t nonmoving_color>
		void update_castling_rights_for_nonmoving_player(const size_t parent_idx,
														 const rank start_rank, const file start_file, const rank end_rank, const file end_file,
														 const piece)
		{
			update_castling_rights_for_nonmoving_player<nonmoving_color>(parent_idx, start_rank, start_file, end_rank, end_file);
		}

	public:
		template<color_t color_to_move, piece_t moving_piece_type, move_type move_type, typename... board_args>
		inline_toggle_member static board make_board(board_args&&... args)
		{
			board board(std::forward<board_args>(args)...);

			if constexpr (move_type == move_type::capture || moving_piece_type == pawn)
			{
				board.clear_50_move_counter();
			}
			else
			{
				board.increment_50_move_counter();
			}

			if constexpr (move_type == move_type::pawn_two_squares)
			{
				board.set_en_passant_file(std::forward<board_args>(args)...);
			}
			else // any other move
			{
				board.set_en_passant_file(empty);
			}

			// check castling rights for the moving player
			if constexpr (moving_piece_type == king)
			{
				if constexpr (other_color(color_to_move) == white)
				{
					board.white_cant_castle_ks();
					board.white_cant_castle_qs();
				}
				else
				{
					board.black_cant_castle_ks();
					board.black_cant_castle_qs();
				}
			}
			else if constexpr (moving_piece_type == rook)
			{
				board.update_castling_rights_for_moving_player<other_color(color_to_move)>(std::forward<board_args>(args)...);
			}

			if constexpr (move_type == move_type::capture)
			{
				board.update_castling_rights_for_nonmoving_player<color_to_move>(std::forward<board_args>(args)...);
			}

			return board;
		}

		void set_key(const tt::key& set_key) { key = set_key; }
		tt::key get_key() const { return key; }

		template<color_t moved_piece_color>
		piece moved_piece() const
		{
			// restore the color bit
			return moved_piece_without_color().value() | moved_piece_color;
		}
		rank get_start_rank() const { return (bitfield() >> start_rank_offset) & square_mask; }
		file get_start_file() const { return (bitfield() >> start_file_offset) & square_mask; }
		rank get_end_rank() const { return (bitfield() >> end_rank_offset) & square_mask; }
		file get_end_file() const { return (bitfield() >> end_file_offset) & square_mask; }
		size_t get_start_index() const { return (bitfield() >> start_file_offset) & index_mask; }
		size_t get_end_index() const { return (bitfield() >> end_file_offset) & index_mask; }
		bool is_promotion() const { return (bitfield() >> promotion_offset) & promotion_bits; }
		file get_en_passant_file() const { return (bitfield() >> en_passant_file_offset) & en_passant_mask; }
		bool white_can_castle_ks() const { return (bitfield() >> white_can_castle_ks_offset) & castling_right_mask; }
		bool white_can_castle_qs() const { return (bitfield() >> white_can_castle_qs_offset) & castling_right_mask; }
		bool black_can_castle_ks() const { return (bitfield() >> black_can_castle_ks_offset) & castling_right_mask; }
		bool black_can_castle_qs() const { return (bitfield() >> black_can_castle_qs_offset) & castling_right_mask; }
		size_t get_fifty_move_counter() const { return (bitfield() >> fifty_move_counter_offset) & fifty_move_counter_mask; }
		bool is_terminal() const { return (bitfield() >> terminal_offset) & terminal_mask; }

		piece moved_piece_without_color() const
		{
			piece_t type = (bitfield() >> moved_piece_offset) & moved_piece_mask;
			// make room for the color bit
			return type << 1;
		}

		// these can only set a field high
	private:
		void set_moved_piece(piece piece) { bitfield() |= uint32_t(piece.value() >> 1) << moved_piece_offset; }
		void set_start_rank(rank rank) { bitfield() |= uint32_t(rank.value()) << start_rank_offset; }
		void set_start_file(file file) { bitfield() |= uint32_t(file.value()) << start_file_offset; }
		void set_end_rank(rank rank) { bitfield() |= uint32_t(rank.value()) << end_rank_offset; }
		void set_end_file(file file) { bitfield() |= uint32_t(file.value()) << end_file_offset; }
		void set_promotion() { bitfield() |= uint32_t(1) << promotion_offset; }
		void set_en_passant_file(file file) { bitfield() |= uint32_t(file.value()) << en_passant_file_offset; }
		void set_white_can_castle_ks(uint32_t arg) { bitfield() |= arg << white_can_castle_ks_offset; }
		void set_white_can_castle_qs(uint32_t arg) { bitfield() |= arg << white_can_castle_qs_offset; }
		void set_black_can_castle_ks(uint32_t arg) { bitfield() |= arg << black_can_castle_ks_offset; }
		void set_black_can_castle_qs(uint32_t arg) { bitfield() |= arg << black_can_castle_qs_offset; }
		void set_fifty_move_counter(uint32_t arg) { bitfield() |= arg << fifty_move_counter_offset; }
	public:
		void set_terminal() { bitfield() |= uint32_t(1) << terminal_offset; }

		// todo: these could be narrowed to modify a single byte (determined at compile time)
		void white_cant_castle_ks() { bitfield() &= ~(1 << white_can_castle_ks_offset); }
		void white_cant_castle_qs() { bitfield() &= ~(1 << white_can_castle_qs_offset); }
		void black_cant_castle_ks() { bitfield() &= ~(1 << black_can_castle_ks_offset); }
		void black_cant_castle_qs() { bitfield() &= ~(1 << black_can_castle_qs_offset); }

		void increment_50_move_counter() { bitfield() += 1 << fifty_move_counter_offset; }
		void clear_50_move_counter() { bitfield() &= ~(fifty_move_counter_mask << fifty_move_counter_offset); }

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

		bool has_move() const { return moved_piece_without_color().is_occupied(); }

		const piece& last_moved_piece(const position& position) const
		{
			return position.piece_at(get_end_rank(), get_end_file());
		}

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

		template <color_t moving_color, piece_t moving_piece_type, move_type move_type>
		inline_toggle_member void update_key_and_static_eval(const position& parent_position, const board& parent_board, tt::key incremental_key)
		{
			// The incremental_key has already had the leaving piece removed, and the color to move toggled.
			// We receive it by copy so we can modify it before writing it.

			const rank start_rank = get_start_rank();
			const file start_file = get_start_file();
			const rank end_rank = get_end_rank();
			const file end_file = get_end_file();

			const size_t start_idx = to_index(start_rank, start_file);
			const size_t end_idx = to_index(end_rank, end_file);

			eval_t incremental_eval = parent_board.get_static_eval();

			piece piece_before{};
			if constexpr (moving_piece_type == pawn || moving_piece_type == king)
			{
				piece_before = moving_piece_type + moving_color; // known at compile time
			}
			else
			{
				piece_before = parent_position.piece_at(start_idx);
			}

			piece piece_after{};
			if constexpr (moving_piece_type == king ||
						  move_type == move_type::pawn_two_squares ||
						  move_type == move_type::en_passant_capture)
			{
				piece_after = moving_color | moving_piece_type; // determine at compile time when possible
			}
			else if (moving_piece_type == pawn)
			{
				piece_after = moved_piece<moving_color>(); // will be a different type if promoting
				incremental_eval -= eval::piece_eval(piece_before);
				incremental_eval += eval::piece_eval(piece_after);
			}
			else
			{
				piece_after = piece_before;
			}
			// add the key for the arriving piece
			incremental_key ^= tt::z_keys.piece_square_keys[end_idx][piece_after.value()];
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
				const size_t captured_pawn_idx = to_index(start_rank, end_file); // the captured pawn is at the start rank and end file
				constexpr piece_t captured_pawn = other_color(moving_color) | pawn;
				// remove key for the captured pawn
				incremental_key ^= tt::z_keys.piece_square_keys[captured_pawn_idx][captured_pawn];
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
				incremental_key ^= tt::z_keys.piece_square_keys[rook_start_index][moving_rook];
				incremental_key ^= tt::z_keys.piece_square_keys[rook_end_index][moving_rook];
				// update eval for the moving rook
				incremental_eval -= eval::piece_square_eval(moving_rook, rook_start_index); // todo: determine at compile time
				incremental_eval += eval::piece_square_eval(moving_rook, rook_end_index); // todo: determine at compile time
			}
			else if (move_type == move_type::capture) // non-en passant capture
			{
				const piece_t captured_piece = parent_position.piece_at(end_idx).value();
				// remove the key for the captured piece
				incremental_key ^= tt::z_keys.piece_square_keys[end_idx][captured_piece];
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

			key = incremental_key;
			static_eval = incremental_eval;
		}

		eval_t get_static_eval() const { return static_eval; }

		void set_eval(const eval_t set_eval) { eval = set_eval; }
		eval_t get_eval() const { return eval; }
	};

	tt::key generate_key(const board& board, const position& position, const color_t color_to_move);

	bool same_move(const board& move_a, const board& move_b);
}
