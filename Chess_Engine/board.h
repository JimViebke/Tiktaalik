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
	explicit Board(const std::vector<Piece> & set_board) : board(set_board) {}
	explicit Board(const Board & parent_board, const int start_rank, const int start_file, const int end_rank, const int end_file)
	{
		// copy the position
		board = parent_board.board;
		// copy castle flags
		white_can_castle_k_s = parent_board.white_can_castle_k_s;
		white_can_castle_q_s = parent_board.white_can_castle_q_s;
		black_can_castle_k_s = parent_board.black_can_castle_k_s;
		black_can_castle_q_s = parent_board.black_can_castle_q_s;
		// update 50 move rule
		if (piece_at(end_rank, end_file).is_empty()) ++fifty_move_rule; else fifty_move_rule = 0;
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
			piece_at(start_rank, end_file) = Piece(empty); // the captured pawn will always be on the same rank that the pawn started, and at the same file that the pawn ended
		}
		// if a king is moving  moves, 
		else if (parent_board.piece_at(start_file, start_file).is_king())
		{
			// it can no longer castle either way
			if (parent_board.piece_at(start_file, start_file).is_white())
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
	}
	explicit Board(const Board & parent_board, const int start_rank, const int start_file, const int end_rank, const int end_file, const piece & promote_to)
		: Board(parent_board, start_rank, start_file, end_rank, end_file)
	{
		// call this constructor (four times) for pawn promotion
		piece_at(end_rank, end_file) = Piece(promote_to);
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

	inline void move_piece(const Rank start_rank, const File start_file, const Rank end_rank, const File end_file)
	{
		piece_at(end_rank, end_file) = piece_at(start_rank, start_file);
		piece_at(start_rank, start_file) = Piece(empty);
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
