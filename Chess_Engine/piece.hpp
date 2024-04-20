#pragma once

#include "evaluations.hpp"

namespace chess
{
	using piece_t = int8_t;
	using color_t = piece_t;

	class piece
	{
	public:
		static constexpr piece_t empty = 0;

		static constexpr piece_t pawn = 1;
		static constexpr piece_t knight = 2;
		static constexpr piece_t bishop = 3;
		static constexpr piece_t rook = 4;
		static constexpr piece_t queen = 5;
		static constexpr piece_t king = 6;

		static constexpr piece_t white = 0 << 3;
		static constexpr piece_t black = 1 << 3;

		static constexpr piece_t type_mask = 0b0111;
		static constexpr piece_t color_mask = 0b1000;

	private:
		piece_t _piece;

	public:
		explicit piece() : _piece(piece::empty) {}
		explicit piece(const piece_t piece) : _piece(piece) {}

		piece_t get_piece() const { return _piece; }
		color_t get_color() const { return _piece & color_mask; }

		evaluation_t eval() const { return evaluations::eval_lookup[_piece]; }

		// check empty
		bool is_empty() const { return _piece == piece::empty; }
		bool is_occupied() const { return _piece != piece::empty; }

		// check color
		bool is_white() const { return (_piece & color_mask) == white; }
		bool is_black() const { return !is_white(); }
		bool is_color(color_t compare_color) const { return (_piece & color_mask) == compare_color; }
		bool is_color(piece piece) const { return (_piece & color_mask) == (piece._piece & color_mask); }
		bool is_opposing_color(color_t compare_color) const { return !is_color(compare_color); }
		bool is_opposing_color(piece piece) const { return !is_color(piece); }

		// check piece type
		bool is_king() const { return (_piece & type_mask) == piece::king; }
		bool is_queen() const { return (_piece & type_mask) == piece::queen; }
		bool is_rook() const { return (_piece & type_mask) == piece::rook; }
		bool is_bishop() const { return (_piece & type_mask) == piece::bishop; }
		bool is_knight() const { return (_piece & type_mask) == piece::knight; }
		bool is_pawn() const { return (_piece & type_mask) == piece::pawn; }

		char to_algebraic_char() const
		{
			if (is_knight()) return 'N';
			else if (is_bishop()) return 'B';
			else if (is_rook()) return 'R';
			else if (is_queen()) return 'Q';
			else if (is_king()) return 'K';
			else return '?';
		}

		// check piece type and color
		bool is(const piece compare_piece) const { return compare_piece._piece == _piece; }
		bool is(const piece_t compare_piece) const { return compare_piece == _piece; }
	};

}
