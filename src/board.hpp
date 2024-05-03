#pragma once

#include <array>
#include <variant>
#include <vector>

#include "capture.hpp"
#include "evaluation.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "types.hpp"

namespace chess
{
	template<color_t color_to_move>
	class board : public colorable<color_to_move>
	{
	public:
		piece moved_piece{};

		rank _start_rank = -1; // the move that resulted in this position
		file _start_file = -1;
		rank _end_rank = -1;
		file _end_file = -1;

		file en_passant_file = -1;
		bool white_can_castle_ks = true;
		bool white_can_castle_qs = true;
		bool black_can_castle_ks = true;
		bool black_can_castle_qs = true;

	private:
		int8_t fifty_move_counter = 0; // counts to 100 ply
		result _result = result::unknown;

	public:
		using other_board_t = board<other(color_to_move)>;
		using child_boards = std::vector<other_board_t>;

		explicit board(const bool w_castle_ks, const bool w_castle_qs, const bool b_castle_ks, const bool b_castle_qs,
					   const file set_en_passant_file, const int8_t set_fifty_move_counter) :
			en_passant_file(set_en_passant_file),
			white_can_castle_ks(w_castle_ks),
			white_can_castle_qs(w_castle_qs),
			black_can_castle_ks(b_castle_ks),
			black_can_castle_qs(b_castle_qs),
			fifty_move_counter(set_fifty_move_counter)
		{
		}

		explicit board(const other_board_t& parent_board, const position& parent_position,
					   const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			moved_piece = parent_position.piece_at(start_rank, start_file);

			// record the move that resulted in this position
			_start_rank = start_rank;
			_start_file = start_file;
			_end_rank = end_rank;
			_end_file = end_file;

			// check en passant rights
			en_passant_file = (moved_piece.is_pawn() && std::abs(rank{ start_rank - end_rank }.value()) == 2)
				? start_file : -1;
			// copy castling rights
			white_can_castle_ks = parent_board.white_can_castle_ks;
			white_can_castle_qs = parent_board.white_can_castle_qs;
			black_can_castle_ks = parent_board.black_can_castle_ks;
			black_can_castle_qs = parent_board.black_can_castle_qs;

			// update counter for 50 move rule - increment for a non-pawn move or non-capture move, reset otherwise
			if (!moved_piece.is_pawn() ||
				parent_position.piece_at(end_rank, end_file).is_empty()) ++fifty_move_counter; else fifty_move_counter = 0;

			// check for en passant captures
			if (moved_piece.is_pawn() &&
				abs(rank{ start_rank - end_rank }.value()) == 1 && // todo: this condition should be unnecessary, remove it
				start_file != end_file &&
				parent_position.piece_at(end_rank, end_file).is_empty())
			{
				fifty_move_counter = 0;
			}
			// if a king is moving, it can no longer castle either way
			else if (moved_piece.is_king())
			{
				if (moved_piece.is_white())
					white_can_castle_ks = white_can_castle_qs = false;
				else
					black_can_castle_ks = black_can_castle_qs = false;
			}

			// if a rook moves, it cannot be used to castle
			if (start_rank == 0) // black rooks
			{
				if (start_file == 0)
					black_can_castle_qs = false;
				else if (start_file == 7)
					black_can_castle_ks = false;
			}
			else if (start_rank == 7) // white rooks
			{
				if (start_file == 0)
					white_can_castle_qs = false;
				else if (start_file == 7)
					white_can_castle_ks = false;
			}

			// if a rook is captured, it cannot be used to castle
			if (end_rank == 0) // black rooks
			{
				if (end_file == 0)
					black_can_castle_qs = false;
				else if (end_file == 7)
					black_can_castle_ks = false;
			}
			else if (end_rank == 7) // white rooks
			{
				if (end_file == 0)
					white_can_castle_qs = false;
				else if (end_file == 7)
					white_can_castle_ks = false;
			}
		}
		explicit board(const other_board_t& parent_board, const position& parent_position,
					   const rank start_rank, const file start_file, const rank end_rank, const file end_file,
					   const piece& promote_to) // call this constructor for each pawn promotion
			: board(parent_board, parent_position, start_rank, start_file, end_rank, end_file)
		{
			moved_piece = promote_to;
		}

		void set_white_can_castle_ks(bool flag) { white_can_castle_ks = flag; }
		void set_white_can_castle_qs(bool flag) { white_can_castle_qs = flag; }
		void set_black_can_castle_ks(bool flag) { black_can_castle_ks = flag; }
		void set_black_can_castle_qs(bool flag) { black_can_castle_qs = flag; }

		result get_result() const { return _result; }
		void set_result(const result set_result) { _result = set_result; }

		const std::string move_to_string() const
		{
			std::string result;
			result += _start_file.value() + 'a';
			result += (_start_rank.value() * -1) + 8 + '0';
			result += _end_file.value() + 'a';
			result += (_end_rank.value() * -1) + 8 + '0';
			return result;
		}

		bool has_move() const { return !moved_piece.is(empty); }

		const piece& last_moved_piece(const position& position) const
		{
			// Deliberately require a position to be passed here, instead of just reading the
			// this->last_move field. This field should be compressible down to 3 bits (for the
			// piece type), at which point it would probably be easier to read from the position
			// anyway.
			return position.piece_at(_end_rank, _end_file);
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
