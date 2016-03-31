#pragma once

#include <list>

#include "board_layouts.h"

class Board
{
private:
	using Rank = int;
	using File = int;

	// position
	std::vector<Piece> board;

	// state
	color color_to_move = color::white;
	int fifty_move_rule = 0;
	File en_passant_flag = -1;
	bool white_can_castle_k_s = true;
	bool white_can_castle_q_s = true;
	bool black_can_castle_k_s = true;
	bool black_can_castle_q_s = true;

public:
	// explicit Board(const Board& b);
	explicit Board(const std::vector<Piece> & set_board) : board(set_board) {}
	explicit Board(const Board & parent_board, const int start_rank, const int start_file, const int end_rank, const int end_file)
	{
		// copy the position
		board = parent_board.get_board();

		// update 50 move rule
		if (piece_at(end_rank, end_file).is_empty()) ++fifty_move_rule; else fifty_move_rule = 0;
		// switch color to move		
		color_to_move = other_color(parent_board.get_color_to_move());

		// if the moving piece is a pawn that is moving two spaces
		en_passant_flag = (parent_board.piece_at(start_rank, start_file).is_pawn() && abs(start_rank - end_rank) == 2)
			? start_file : -1;

		// normally we can detect captures based on the presence of a piece at the destination coordinates,
		// but en passant requires a special check
		if (piece_at(start_rank, start_file).is_pawn() &&
			abs(start_rank - end_rank) == 1 && start_file != end_file &&
			piece_at(end_rank, end_file).is_empty())
		{
			fifty_move_rule = 0;

			// the captured pawn will always be on the same rank that the pawn started, and at the same file that the pawn ended
			remove_piece(start_rank, end_file);
		}

		// make the move
		move_piece(start_rank, start_file, end_rank, end_file);
	}

	static void print_board(const Board & board, const unsigned & offset = 0);
	static void print_board(const std::list<Board> & boards);

	inline const color get_color_to_move() const { return color_to_move; }
	inline void set_color_to_move(const color set_color_to_move) { color_to_move = set_color_to_move; }

	inline const Piece& piece_at(const Rank rank, const File file) const
	{
		return board[rank * 8 + file];
	}
	inline Piece& piece_at(const Rank rank, const File file)
	{
		return board[rank * 8 + file];
	}

	inline void set_piece(const Rank rank, const File file, const Piece piece)
	{
		piece_at(rank, file) = piece;
	}
	inline void remove_piece(const Rank rank, const File file)
	{
		set_piece(rank, file, Piece(empty));
	}
	inline void move_piece(const Rank start_rank, const File start_file, const Rank end_rank, const File end_file)
	{
		set_piece(end_rank, end_file, piece_at(start_rank, start_file));
		remove_piece(start_rank, start_file);
	}

	int evaluate_position() const
	{
		int position_value = 0;

		// evaluate material
		for (unsigned rank = 0; rank < 8; ++rank)
			for (unsigned file = 0; file < 8; ++file)
				position_value += evaluate_piece(piece_at(rank, file));

		return position_value;
	}

	std::list<Board> get_child_boards() const;

	std::vector<Piece> get_board() const { return board; }

private:
	template<typename T> inline bool bounds_check(const T rank_or_file) const { return rank_or_file < 8 && rank_or_file >= 0; }
	template<typename T> inline bool bounds_check(const T rank, const T file) const { return bounds_check(rank) && bounds_check(file); }

	bool is_valid_position() const;

	static void remove_invalid_boards(std::list<Board> & boards);

	int evaluate_piece(const Piece & piece) const
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

	void find_pawn_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_rook_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_bishop_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_knight_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_queen_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_king_moves(std::list<Board> & child_boards, const int rank, const int file) const;

	bool is_king_in_check(const color check_color) const;
	bool is_king_in_check(const Piece piece, const Rank rank, const File file) const;

	static color other_color(const color other_color) { return (other_color == color::white) ? color::black : color::white; }
};
