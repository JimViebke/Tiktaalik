#pragma once

#include <array>
#include <bit>
#include <variant>
#include <vector>

#include "capture.hpp"
#include "evaluation.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "types.hpp"

namespace chess
{
	enum class move_type
	{
		rook, // indicates rook and queen moves
		king,
		pawn_two_squares,
		pawn_other,
		other
	};

	template<color_t color_to_move>
	class board : public colorable<color_to_move>
	{
	private:
		// bitfield sizes
		static constexpr size_t moved_piece_bits = 3;
		static constexpr size_t square_bits = 3;
		static constexpr size_t en_passant_bits = 4;
		static constexpr size_t castling_right_bits = 1;
		static constexpr size_t fifty_move_counter_bits = std::bit_width(50u * 2);
		static constexpr size_t result_bits = 2;

		// bitfield masks
		static constexpr uint64_t moved_piece_mask = (1ull << moved_piece_bits) - 1;
		static constexpr uint64_t square_mask = (1ull << square_bits) - 1;
		static constexpr uint64_t en_passant_mask = (1ull << en_passant_bits) - 1;
		static constexpr uint64_t castling_right_mask = (1ull << castling_right_bits) - 1;
		static constexpr uint64_t fifty_move_counter_mask = (1ull << fifty_move_counter_bits) - 1;
		static constexpr uint64_t result_mask = (1ull << result_bits) - 1;

		// bitfield positions
		static constexpr size_t moved_piece_offset = 0;
		static constexpr size_t start_rank_offset = moved_piece_offset + moved_piece_bits; // the move that resulted in this position
		static constexpr size_t start_file_offset = start_rank_offset + square_bits;
		static constexpr size_t end_rank_offset = start_file_offset + square_bits;
		static constexpr size_t end_file_offset = end_rank_offset + square_bits;
		static constexpr size_t en_passant_file_offset = end_file_offset + square_bits;
		static constexpr size_t white_can_castle_ks_offset = en_passant_file_offset + en_passant_bits;
		static constexpr size_t white_can_castle_qs_offset = white_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t black_can_castle_ks_offset = white_can_castle_qs_offset + castling_right_bits;
		static constexpr size_t black_can_castle_qs_offset = black_can_castle_ks_offset + castling_right_bits;
		static constexpr size_t fifty_move_counter_offset = black_can_castle_qs_offset + castling_right_bits; // counts to 100 ply
		static constexpr size_t result_offset = fifty_move_counter_offset + fifty_move_counter_bits;

		// make sure the required bitfield size is what we expect
		static_assert(result_bits + result_offset == 32);

		uint8_t board_state[4]{};

		uint32_t& bitfield() { return *(uint32_t*)&board_state[0]; }
		const uint32_t& bitfield() const { return *(uint32_t*)&board_state[0]; }

	public:
		using other_board_t = board<other(color_to_move)>;
		using child_boards = std::vector<other_board_t>;

		explicit board(const bool w_castle_ks, const bool w_castle_qs, const bool b_castle_ks, const bool b_castle_qs,
					   const file en_passant_file, const int8_t fifty_move_counter)
		{
			set_white_can_castle_ks(w_castle_ks);
			set_white_can_castle_qs(w_castle_qs);
			set_black_can_castle_ks(b_castle_ks);
			set_black_can_castle_qs(b_castle_qs);

			set_en_passant_file(en_passant_file.value());
			set_fifty_move_counter(fifty_move_counter);
		}

	private:
		explicit board(const other_board_t& parent_board, const position& parent_position,
					   const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			const piece moved_piece = parent_position.piece_at(start_rank, start_file);
			set_moved_piece(moved_piece);

			// record the move that resulted in this position
			set_start_rank(start_rank);
			set_start_file(start_file);
			set_end_rank(end_rank);
			set_end_file(end_file);

			// copy castling rights
			set_white_can_castle_ks(parent_board.white_can_castle_ks());
			set_white_can_castle_qs(parent_board.white_can_castle_qs());
			set_black_can_castle_ks(parent_board.black_can_castle_ks());
			set_black_can_castle_qs(parent_board.black_can_castle_qs());

			// if a rook is captured, it cannot be used to castle
			if (end_rank == 0) // black rooks
			{
				if (end_file == 0)
					black_cant_castle_qs();
				else if (end_file == 7)
					black_cant_castle_ks();
			}
			else if (end_rank == 7) // white rooks
			{
				if (end_file == 0)
					white_cant_castle_qs();
				else if (end_file == 7)
					white_cant_castle_ks();
			}
		}
		explicit board(const other_board_t& parent_board, const position& parent_position,
					   const rank start_rank, const file start_file, const rank end_rank, const file end_file,
					   const piece& promote_to) // call this constructor for each pawn promotion
			: board(parent_board, parent_position, start_rank, start_file, end_rank, end_file)
		{
			set_moved_piece(promote_to);
		}

		void set_en_passant_file(const other_board_t&, const position&,
								 const rank, const file start_file, const rank, const file)
		{
			set_en_passant_file(start_file);
		}

		void update_50_move_counter(const other_board_t&, const position& position,
									const rank, const file, const rank end_rank, const file end_file)
		{
			if (position.piece_at(end_rank, end_file).is_empty())
			{
				increment_50_move_counter();
			}
			else
			{
				clear_50_move_counter();
			}
		}

		void update_50_move_counter(const other_board_t& board, const position& position,
									const rank start_rank, const file start_file, const rank end_rank, const file end_file,
									const piece&)
		{
			update_50_move_counter(board, position, start_rank, start_file, end_rank, end_file);
		}

		void update_castling_rights(const other_board_t&, const position&,
									const rank start_rank, const file start_file, const rank, const file)
		{
			// if a rook moves, it cannot be used to castle
			if (start_rank == 0) // black rooks
			{
				if (start_file == 0)
					black_cant_castle_qs();
				else if (start_file == 7)
					black_cant_castle_ks();
			}
			else if (start_rank == 7) // white rooks
			{
				if (start_file == 0)
					white_cant_castle_qs();
				else if (start_file == 7)
					white_cant_castle_ks();
			}
		}

	public:
		template<move_type move_type, typename... board_args>
		static board<color_to_move> make_board(board_args... args)
		{
			board<color_to_move> board(std::forward<board_args>(args)...);

			// reset for any kind of pawn move, otherwise, check the destination square
			if constexpr (move_type == move_type::pawn_two_squares ||
						  move_type == move_type::pawn_other)
			{
				board.clear_50_move_counter();
			}
			else
			{
				board.update_50_move_counter(std::forward<board_args>(args)...);
			}

			if constexpr (move_type == move_type::pawn_two_squares)
			{
				board.set_en_passant_file(std::forward<board_args>(args)...);
			}
			else // any other move
			{
				board.set_en_passant_file(empty);
			}

			if constexpr (move_type == move_type::rook)
			{
				board.update_castling_rights(std::forward<board_args>(args)...);
			}

			// if a king is moving, it can no longer castle either way
			if constexpr (move_type == move_type::king)
			{
				if constexpr (other(color_to_move) == white)
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

			return board;
		}

		piece moved_piece() const
		{
			piece_t type = (bitfield() >> moved_piece_offset) & moved_piece_mask;
			// make room for the color bit
			type <<= 1;
			// restore the color bit
			return type + other(color_to_move);
		}
		rank start_rank() const { return (bitfield() >> start_rank_offset) & square_mask; }
		file start_file() const { return (bitfield() >> start_file_offset) & square_mask; }
		rank end_rank() const { return (bitfield() >> end_rank_offset) & square_mask; }
		file end_file() const { return (bitfield() >> end_file_offset) & square_mask; }
		file en_passant_file() const { return (bitfield() >> en_passant_file_offset) & en_passant_mask; }
		bool white_can_castle_ks() const { return (bitfield() >> white_can_castle_ks_offset) & castling_right_mask; }
		bool white_can_castle_qs() const { return (bitfield() >> white_can_castle_qs_offset) & castling_right_mask; }
		bool black_can_castle_ks() const { return (bitfield() >> black_can_castle_ks_offset) & castling_right_mask; }
		bool black_can_castle_qs() const { return (bitfield() >> black_can_castle_qs_offset) & castling_right_mask; }
		size_t fifty_move_counter() const { return (bitfield() >> fifty_move_counter_offset) & fifty_move_counter_mask; }
		result get_result() const { return result((bitfield() >> result_offset) & result_mask); }

		// these can only set a field high
	private:
		void set_moved_piece(piece piece) { bitfield() |= uint32_t(piece.value() >> 1) << moved_piece_offset; }
		void set_start_rank(rank rank) { bitfield() |= uint32_t(rank.value()) << start_rank_offset; }
		void set_start_file(file file) { bitfield() |= uint32_t(file.value()) << start_file_offset; }
		void set_end_rank(rank rank) { bitfield() |= uint32_t(rank.value()) << end_rank_offset; }
		void set_end_file(file file) { bitfield() |= uint32_t(file.value()) << end_file_offset; }
		void set_en_passant_file(file file) { bitfield() |= uint32_t(file.value()) << en_passant_file_offset; }
		void set_white_can_castle_ks(uint32_t arg) { bitfield() |= arg << white_can_castle_ks_offset; }
		void set_white_can_castle_qs(uint32_t arg) { bitfield() |= arg << white_can_castle_qs_offset; }
		void set_black_can_castle_ks(uint32_t arg) { bitfield() |= arg << black_can_castle_ks_offset; }
		void set_black_can_castle_qs(uint32_t arg) { bitfield() |= arg << black_can_castle_qs_offset; }
		void set_fifty_move_counter(uint32_t arg) { bitfield() |= arg << fifty_move_counter_offset; }
	public:
		void set_result(result result) { bitfield() |= uint32_t(result) << result_offset; }

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
			result += start_file().value() + 'a';
			result += (start_rank().value() * -1) + 8 + '0';
			result += end_file().value() + 'a';
			result += (end_rank().value() * -1) + 8 + '0';
			return result;
		}

		bool has_move() const { return !moved_piece().is(empty); }

		const piece& last_moved_piece(const position& position) const
		{
			return position.piece_at(end_rank(), end_file());
		}

	private:
		friend board<white>;
		friend board<black>;
	};

	void print_board(const auto& board, const unsigned& indent)
	{
		for (rank rank = 0; rank < 8; ++rank)
		{
			// add indent
			for (unsigned i = 0; i < indent * 9; ++i) std::cout << ' ';

			for (file file = 0; file < 8; ++file)
			{
				const piece& piece = board.piece_at(rank, file);

				if (piece.is_empty()) std::cout << ".";
				else if (piece.is_white())
				{
					if (piece.is_rook()) std::cout << "R";
					else if (piece.is_bishop()) std::cout << "B";
					else if (piece.is_knight()) std::cout << "N";
					else if (piece.is_queen()) std::cout << "Q";
					else if (piece.is_king()) std::cout << "K";
				}
				else if (piece.is_black())
				{
					if (piece.is_rook()) std::cout << "r";
					else if (piece.is_bishop()) std::cout << "b";
					else if (piece.is_knight()) std::cout << "n";
					else if (piece.is_queen()) std::cout << "q";
					else if (piece.is_king()) std::cout << "k";
				}
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	void print_board(const auto& boards)
	{
		for (rank rank = 0; rank < 8; ++rank)
		{
			// render this rank for each board
			for (auto it = boards.cbegin(); it != boards.cend(); ++it)
			{
				for (file file = 0; file < 8; ++file)
				{
					const piece& piece = it->piece_at(rank, file);

					if (piece.is_empty()) std::cout << ".";
					else if (piece.is_white())
					{
						if (piece.is_pawn()) std::cout << "P";
						else if (piece.is_rook()) std::cout << "R";
						else if (piece.is_bishop()) std::cout << "B";
						else if (piece.is_knight()) std::cout << "N";
						else if (piece.is_queen()) std::cout << "Q";
						else if (piece.is_king()) std::cout << "K";
					}
					else if (piece.is_black())
					{
						if (piece.is_pawn()) std::cout << "p";
						else if (piece.is_rook()) std::cout << "r";
						else if (piece.is_bishop()) std::cout << "b";
						else if (piece.is_knight()) std::cout << "n";
						else if (piece.is_queen()) std::cout << "q";
						else if (piece.is_king()) std::cout << "k";
					}
				}

				// add indent if there is another board to print
				if (it != --boards.cend())
					for (unsigned i = 0; i < 5; ++i) std::cout << ' ';
			}

			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}
