#pragma once

#include <array>
#include <iterator> // for parsing FENs
#include <sstream> // for parsing FENs
#include <vector>

#include "evaluation.hpp"
#include "types.hpp"

namespace chess
{
	template<typename T>
	constexpr bool bounds_check(const T rank_or_file)
	{
		return rank_or_file.value() < 8 && rank_or_file.value() >= 0;
	}
	constexpr bool bounds_check(const rank rank, const file file)
	{
		return bounds_check(rank) && bounds_check(file);
	}

	constexpr size_t to_index(const rank rank, const file file)
	{
		return rank.value() * 8ull + file.value();
	}

	class Board
	{
	private:
		position _position;
		color_t color_to_move = white;

		rank _start_rank = -1; // the move that resulted in this position
		file _start_file = -1;
		rank _end_rank = -1;
		file _end_file = -1;

		file en_passant_flag = -1;
		bool white_can_castle_k_s = true;
		bool white_can_castle_q_s = true;
		bool black_can_castle_k_s = true;
		bool black_can_castle_q_s = true;

		int8_t fifty_move_rule = 0;
		result _result = result::unknown;

	public:
		using board_list = std::vector<Board>;

		explicit Board(const Board& parent_board, const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			// copy the position
			_position = parent_board._position;
			// toggle color to move
			color_to_move = other_color(parent_board.get_color_to_move());

			// record the move that resulted in this position
			_start_rank = start_rank;
			_start_file = start_file;
			_end_rank = end_rank;
			_end_file = end_file;

			// check if the en passant indicator needs to be set
			en_passant_flag = (parent_board.piece_at(start_rank, start_file).is_pawn() && std::abs(rank{ start_rank - end_rank }.value()) == 2)
				? start_file : -1;
			// copy castle flags
			white_can_castle_k_s = parent_board.white_can_castle_k_s;
			white_can_castle_q_s = parent_board.white_can_castle_q_s;
			black_can_castle_k_s = parent_board.black_can_castle_k_s;
			black_can_castle_q_s = parent_board.black_can_castle_q_s;

			// update 50 move rule - increment for a non-pawn-move or non-capture move
			if (!piece_at(end_rank, end_file).is_pawn() || piece_at(end_rank, end_file).is_empty()) ++fifty_move_rule; else fifty_move_rule = 0;

			// check for en passant captures
			if (piece_at(start_rank, start_file).is_pawn() &&
				abs(rank{ start_rank - end_rank }.value()) == 1 && start_file != end_file &&
				piece_at(end_rank, end_file).is_empty())
			{
				fifty_move_rule = 0;
				piece_at(start_rank, end_file) = piece(piece::empty); // the captured pawn will always be on the same rank that the pawn started, and at the same file that the pawn ended
			}
			// if a king is moving
			else if (piece_at(start_rank, start_file).is_king())
			{
				// it can no longer castle either way
				if (parent_board.piece_at(start_rank, start_file).is_white())
					white_can_castle_k_s = white_can_castle_q_s = false;
				else
					black_can_castle_k_s = black_can_castle_q_s = false;

				if (abs(file{ start_file - end_file }.value()) > 1) // if the king is castling, move the rook
				{
					if (start_file < end_file) // kingside castle
						make_move(start_rank, 7, start_rank, 5); // move the rook
					else if (start_file > end_file) // queenside castle
						make_move(start_rank, 0, start_rank, 3); // move the rook
				}
			}

			// if a rook moves, it cannot be used to castle
			if (start_rank == 0)
			{
				if (start_file == 0 && piece_at(start_rank, start_file).is_rook())
					black_can_castle_q_s = false;
				else if (start_file == 7 && piece_at(start_rank, start_file).is_rook())
					black_can_castle_k_s = false;
			}
			else if (start_rank == 7)
			{
				if (start_file == 0 && piece_at(start_rank, start_file).is_rook())
					white_can_castle_q_s = false;
				else if (start_file == 7 && piece_at(start_rank, start_file).is_rook())
					white_can_castle_k_s = false;
			}

			// if a rook is captured, it cannot be used to castle
			if (end_rank == 0) // detect captures of black's rooks
			{
				if (end_file == 0) // queen's rook
					black_can_castle_q_s = false;
				else if (end_rank == 7) // king's rook
					black_can_castle_k_s = false;
			}
			else if (end_rank == 7) // detect captures of white's rooks
			{
				if (end_file == 0) // queen's rook
					black_can_castle_q_s = false;
				else if (end_rank == 7) // king's rook
					white_can_castle_k_s = false;
			}

			// move the piece
			make_move(start_rank, start_file, end_rank, end_file);
		}
		explicit Board(const Board& parent_board, const rank start_rank, const file start_file, const rank end_rank, const file end_file,
					   const piece& promote_to) // call this constructor for each pawn promotion
			: Board(parent_board, start_rank, start_file, end_rank, end_file)
		{			
			piece_at(end_rank, end_file) = piece(promote_to);
		}
		explicit Board(const std::string& fen); // simple, nonvalidating FEN parser

		void set_white_can_castle_k_s(bool flag) { white_can_castle_k_s = flag; }
		void set_white_can_castle_q_s(bool flag) { white_can_castle_q_s = flag; }
		void set_black_can_castle_k_s(bool flag) { black_can_castle_k_s = flag; }
		void set_black_can_castle_q_s(bool flag) { black_can_castle_q_s = flag; }

		result get_result() const { return _result; }

		static void print_board(const Board& board, const unsigned& offset = 0);
		static void print_board(const board_list& boards);

		const std::string move_to_string() const
		{
			std::string result;
			result += _start_file.value() + 'a';
			result += (_start_rank.value() * -1) + 8 + '0';
			result += _end_file.value() + 'a';
			result += (_end_rank.value() * -1) + 8 + '0';
			return result;
		}

		const color_t get_color_to_move() const { return color_to_move; }

		bool white_to_move() const { return color_to_move == white; }
		bool black_to_move() const { return color_to_move == black; }

		const piece& piece_at(const rank rank, const file file) const
		{
			return _position[to_index(rank, file)];
		}
		piece& piece_at(const rank rank, const file file)
		{
			return _position[to_index(rank, file)];
		}

		bool is_empty(const auto rank, const auto file) const
		{
			return piece_at(rank, file).is_empty();
		}
		bool is_occupied(const rank rank, const file file) const
		{
			return !is_empty(rank, file);
		}

		eval_t evaluate_position() const
		{
			eval_t material_value = 0;

			// evaluate material
			for (const auto& piece : _position)
			{
				material_value += piece.eval();
			}

			return material_value;
		}

		position get_position() const { return _position; }

		bool is_king_in_check(const color_t check_color) const;

		board_list& generate_child_boards();

	private:
		void make_move(const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			auto& start = piece_at(start_rank, start_file);
			piece_at(end_rank, end_file) = start;
			start = piece(piece::empty);
		}

		bool board_has_move() const
		{
			// if any of start/end file/rank are valid (not -1), all are valid
			return _start_rank != rank{ -1 };
		}
		const piece& last_moved_piece() const
		{
			return piece_at(_end_rank, _end_file);
		}

		void set_color_to_move(const color_t set_color_to_move) { color_to_move = set_color_to_move; }

		bool is_valid_position() const;

		static void remove_invalid_boards(board_list& boards);

		void find_pawn_moves(board_list& child_boards, const rank rank, const file file) const;
		void find_rook_moves(board_list& child_boards, const rank rank, const file file) const;
		void find_bishop_moves(board_list& child_boards, const rank rank, const file file) const;
		void find_knight_moves(board_list& child_boards, const rank rank, const file file) const;
		void find_queen_moves(board_list& child_boards, const rank rank, const file file) const;
		void find_king_moves(board_list& child_boards, const rank rank, const file file) const;

		bool is_king_in_check(const piece piece, const rank rank, const file file) const;

		static color_t other_color(const color_t other_color) { return (other_color == white) ? black : white; }
	};

}
