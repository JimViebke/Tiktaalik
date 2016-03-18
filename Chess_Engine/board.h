#pragma once

#include <list>

#include "board_layouts.h"
#include "constants.h"

class Board
{
private:
	using Rank = int;
	using File = int;

	// position
	std::vector<Piece> board;

	// state
	color to_move = color::white;
	int fifty_move_rule = 0;
	bool white_can_castle_k_s = true;
	bool white_can_castle_q_s = true;
	bool black_can_castle_k_s = true;
	bool black_can_castle_q_s = true;

public:
	Board(const std::vector<Piece> & set_board) : board(set_board) {}
	Board(const Board & parent_board, const int start_rank, const int start_file, const int end_rank, const int end_file)
	{
		// update position
		board = parent_board.get_board();

		// update state
		if (!piece_at(end_rank, end_file).is_empty()) fifty_move_rule = 0;
		to_move = ((parent_board.get_color_turn() == color::white) ? color::black : color::white);

		// make the move
		move_piece(start_rank, start_file, end_rank, end_file);
	}

	inline const color get_color_turn() const { return to_move; }

	inline const Piece& piece_at(const Rank rank, const File file) const
	{
		return board[rank*8 + file];
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
		set_piece(rank, file, Piece(color::none, piece::empty));
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

	static void remove_invalid_boards(std::list<Board> & boards);

	bool is_valid_position() const;

	std::vector<Piece> get_board() const { return board; }

private:
	template<typename T> inline bool bounds_check(const T rank_or_file) const { return rank_or_file < 8 && rank_or_file >= 0; }
	template<typename T> inline bool bounds_check(const T rank, const T file) const { return bounds_check(rank) && bounds_check(file); }

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

	void find_rook_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_bishop_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_knight_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_queen_moves(std::list<Board> & child_boards, const int rank, const int file) const;
	void find_king_moves(std::list<Board> & child_boards, const int rank, const int file) const;
};
