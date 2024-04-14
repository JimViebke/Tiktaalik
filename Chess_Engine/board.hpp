#pragma once

#include <array>
#include <iterator> // for parsing FENs
#include <sstream> // for parsing FENs
#include <vector>

#include "board_layouts.hpp"
#include "types.hpp"

namespace chess
{
	class Board
	{
	private:
		position _position;
		char move[4] = "   ";
		color color_to_move = color::white;

		file en_passant_flag = -1;
		bool white_can_castle_k_s = true;
		bool white_can_castle_q_s = true;
		bool black_can_castle_k_s = true;
		bool black_can_castle_q_s = true;

		int8_t fifty_move_rule = 0;
		result _result = result::unknown;

	public:
		using board_list = std::vector<Board>;

		explicit Board(const position& set_position) : _position(set_position) {}
		explicit Board(const Board& parent_board, const int start_rank, const int start_file, const int end_rank, const int end_file)
		{
			// copy the position
			_position = parent_board._position;
			// copy castle flags
			white_can_castle_k_s = parent_board.white_can_castle_k_s;
			white_can_castle_q_s = parent_board.white_can_castle_q_s;
			black_can_castle_k_s = parent_board.black_can_castle_k_s;
			black_can_castle_q_s = parent_board.black_can_castle_q_s;
			// update 50 move rule - increment for a non-pawn-move or non-capture move
			if (!piece_at(end_rank, end_file).is_pawn() || piece_at(end_rank, end_file).is_empty()) ++fifty_move_rule; else fifty_move_rule = 0;
			// toggle color to move
			color_to_move = other_color(parent_board.get_color_to_move());
			// check if the en passant indicator needs to be set
			en_passant_flag = (parent_board.piece_at(start_rank, start_file).is_pawn() && abs(start_rank - end_rank) == 2)
				? start_file : -1;

			// check for en passant captures
			if (piece_at(start_rank, start_file).is_pawn() &&
				abs(start_rank - end_rank) == 1 && start_file != end_file &&
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
				else // the moving king is black
					black_can_castle_k_s = black_can_castle_q_s = false;

				if (abs(start_file - end_file) > 1) // if the king is castling, move the rook
				{
					if (start_file < end_file) // kingside castle
						move_piece(start_rank, 7, start_rank, 5); // move the rook
					else if (start_file > end_file) // queenside castle
						move_piece(start_rank, 0, start_rank, 3); // move the rook
				}
			}

			// if a rook moves out of its starting corner, it cannot be used to castle
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
			move_piece(start_rank, start_file, end_rank, end_file);

			// record the move
			save_move(start_rank, start_file, end_rank, end_file);
		}
		explicit Board(const Board& parent_board, const int start_rank, const int start_file, const int end_rank, const int end_file, const piece& promote_to)
			: Board(parent_board, start_rank, start_file, end_rank, end_file)
		{
			// call this constructor (four times) for pawn promotion
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

		result get_result() const { return _result; }

		static void print_board(const Board& board, const unsigned& offset = 0);
		static void print_board(const board_list& boards);

		const std::string move_to_string() const
		{
			std::string result = "";
			result += move[0];
			result += move[1];
			result += move[2];
			result += move[3];
			return result;
		}

		const color get_color_to_move() const { return color_to_move; }

		bool white_to_move() const { return color_to_move == color::white; }
		bool black_to_move() const { return color_to_move == color::black; }

		const piece& piece_at(const rank rank, const file file) const
		{
			return _position[(size_t)rank * 8 + file];
		}
		piece& piece_at(const rank rank, const file file)
		{
			return _position[(size_t)rank * 8 + file];
		}

		bool is_empty(const rank rank, const file file) const
		{
			return piece_at(rank, file).is_empty();
		}
		bool is_occupied(const rank rank, const file file) const
		{
			return !is_empty(rank, file);
		}

		int evaluate_position() const
		{
			int material_value = 0;

			// evaluate material
			for (unsigned i = 0; i < 64; ++i)
				material_value += evaluate_piece(_position[i]);

			return material_value;
		}

		position get_position() const { return _position; }

		bool is_king_in_check(const color check_color) const;

		board_list generate_child_boards();

	private:
		// Make this private at least for now.
		// External users should be using the constructor, not modifying an existing board.
		void move_piece(const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			piece_at(end_rank, end_file) = piece_at(start_rank, start_file);
			piece_at(start_rank, start_file) = piece(piece::empty);
		}

		void save_move(const rank start_rank, const file start_file, const rank end_rank, const file end_file)
		{
			move[0] = (start_file + 'a');
			move[1] = ((start_rank * -1) + 8 + '0');
			move[2] = (end_file + 'a');
			move[3] = ((end_rank * -1) + 8 + '0');
		}

		void set_color_to_move(const color set_color_to_move) { color_to_move = set_color_to_move; }

		template<typename T> bool bounds_check(const T rank_or_file) const { return rank_or_file < 8 && rank_or_file >= 0; }
		template<typename T> bool bounds_check(const T rank, const T file) const { return bounds_check(rank) && bounds_check(file); }

		bool is_valid_position() const;

		static void remove_invalid_boards(board_list& boards);

		int evaluate_piece(const piece& piece) const
		{
			if (piece.is_empty()) return 0;

			if (piece.is_white())
			{
				if (piece.is_pawn()) return constants::PAWN_VALUE;
				else if (piece.is_knight()) return constants::KNIGHT_VALUE;
				else if (piece.is_bishop()) return constants::BISHOP_VALUE;
				else if (piece.is_rook()) return constants::ROOK_VALUE;
				else if (piece.is_queen()) return constants::QUEEN_VALUE;
				else if (piece.is_king()) return constants::KING_VALUE;
			}
			else
			{
				if (piece.is_pawn()) return -constants::PAWN_VALUE;
				else if (piece.is_knight()) return -constants::KNIGHT_VALUE;
				else if (piece.is_bishop()) return -constants::BISHOP_VALUE;
				else if (piece.is_rook()) return -constants::ROOK_VALUE;
				else if (piece.is_queen()) return -constants::QUEEN_VALUE;
				else if (piece.is_king()) return -constants::KING_VALUE;
			}

			return 0; // should never happen
		}

		void find_pawn_moves(board_list& child_boards, const int rank, const int file) const;
		void find_rook_moves(board_list& child_boards, const int rank, const int file) const;
		void find_bishop_moves(board_list& child_boards, const int rank, const int file) const;
		void find_knight_moves(board_list& child_boards, const int rank, const int file) const;
		void find_queen_moves(board_list& child_boards, const int rank, const int file) const;
		void find_king_moves(board_list& child_boards, const int rank, const int file) const;

		bool is_king_in_check(const piece piece, const rank rank, const file file) const;

		static color other_color(const color other_color) { return (other_color == color::white) ? color::black : color::white; }
	};

}
