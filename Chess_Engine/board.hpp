#pragma once

#include <array>
#include <iterator> // for parsing FENs
#include <sstream> // for parsing FENs
#include <vector>

#include "board_layouts.hpp"
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

		explicit Board(const position& set_position, const color_t& set_to_move) : _position(set_position), color_to_move(set_to_move) {}
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
		/*
		explicit Board(const std::string & FEN)
		{
			std::stringstream ss(FEN);
			const std::istream_iterator<std::string> begin(ss);
			const std::vector<std::string> strings(begin, std::istream_iterator<std::string>());

			const std::map<const char, const piece> pieces = {
				{ 'p', black_pawn },
				{ 'n', black_knight },
				{ 'b', black_bishop },
				{ 'r', black_rook },
				{ 'q', black_queen },
				{ 'k', black_king },

				{ 'P', white_pawn },
				{ 'N', white_knight },
				{ 'B', white_bishop },
				{ 'R', white_rook },
				{ 'Q', white_queen },
				{ 'K', white_king } };

			position.reserve(64);

			// Edited from Wikipedia:

			// A FEN record contains six fields. The separator between fields is a space. The fields are:

			// 1. Piece placement (from white's perspective). Each rank is described, starting with rank 8 and ending with rank 1; within each rank, the contents of each
			// square are described from file "a" through file "h". Following the Standard Algebraic Notation (SAN), each piece is identified by a single letter taken
			// from the standard English names (pawn = "P", knight = "N", bishop = "B", rook = "R", queen = "Q" and king = "K"). White pieces are designated using
			// upper-case letters ("PNBRQK") while black pieces use lowercase ("pnbrqk"). Empty squares are noted using digits 1 through 8 (the number of empty squares),
			// and "/" separates ranks.

			// for each charater
			for (auto it = strings[0].cbegin(); it != strings[0].cend(); ++it)
			{
				// find the piece
				const auto piece_type = pieces.find(*it);
				if (piece_type != pieces.cend())
					position.push_back(Piece(piece_type->second));
				else if (*it >= '0' && *it <= '9')
					for (int empty = 0; empty < (*it) - '0'; ++empty)
						position.emplace_back(Piece(piece::empty));
			}

			// 2. Active color. "w" means White moves next, "b" means Black.

			color_to_move = ((strings[1].find('w') != std::string::npos) ? white : black);

			// 3. Castling availability. If neither side can castle, this is "-". Otherwise, this has one or more letters: "K" (White can castle kingside), "Q" (White
			// can castle queenside), "k" (Black can castle kingside), and/or "q" (Black can castle queenside).

			white_can_castle_k_s = (strings[2].find("K") != std::string::npos);
			white_can_castle_q_s = (strings[2].find("Q") != std::string::npos);
			black_can_castle_k_s = (strings[2].find("k") != std::string::npos);
			black_can_castle_q_s = (strings[2].find("q") != std::string::npos);

			// 4. En passant target square in algebraic notation. If there's no en passant target square, this is "-". If a pawn has just made a two-square move, this
			// is the position "behind" the pawn. This is recorded regardless of whether there is a pawn in position to make an en passant capture.

			// 5. Halfmove clock. This is the number of halfmoves since the last capture or pawn advance. This is used to determine if a draw can be claimed under the
			// fifty - move rule.

			// 6. Fullmove number. The number of the full move. It starts at 1, and is incremented after Black's move.
		}*/

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
