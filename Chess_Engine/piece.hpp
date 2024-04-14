#pragma once

#include "types.hpp"

namespace chess
{
	class piece
	{
	public:
		static constexpr piece_t black_king = -6;
		static constexpr piece_t black_queen = -5;
		static constexpr piece_t black_rook = -4;
		static constexpr piece_t black_bishop = -3;
		static constexpr piece_t black_knight = -2;
		static constexpr piece_t black_pawn = -1;

		static constexpr piece_t empty = 0;

		static constexpr piece_t white_pawn = 1;
		static constexpr piece_t white_knight = 2;
		static constexpr piece_t white_bishop = 3;
		static constexpr piece_t white_rook = 4;
		static constexpr piece_t white_queen = 5;
		static constexpr piece_t white_king = 6;

	private:
		piece_t _piece;

	public:
		explicit piece() : _piece(piece::empty) {}
		explicit piece(const piece_t piece) : _piece(piece) {}

		// check empty
		bool is_empty() const { return _piece == piece::empty; }
		bool is_occupied() const { return _piece != piece::empty; }

		// check color
		bool is_white() const { return _piece > 0; }
		bool is_black() const { return _piece < 0; }
		bool is_color(color compare_color) const { return is_occupied() && (_piece ^ compare_color) >= 0; }
		bool is_color(piece piece) const { return (_piece ^ piece._piece) >= 0; }
		bool is_opposing_color(color compare_color) const { return (_piece ^ compare_color) < 0; }
		bool is_opposing_color(piece piece) const { return (_piece ^ piece._piece) < 0; }

		// check piece type
		bool is_king() const { return my_abs(_piece) == piece::white_king; }
		bool is_queen() const { return my_abs(_piece) == piece::white_queen; }
		bool is_rook() const { return my_abs(_piece) == piece::white_rook; }
		bool is_bishop() const { return my_abs(_piece) == piece::white_bishop; }
		bool is_knight() const { return my_abs(_piece) == piece::white_knight; }
		bool is_pawn() const { return my_abs(_piece) == piece::white_pawn; }

		// check piece type and color
		bool is(const piece compare_piece) const { return compare_piece._piece == _piece; }
		bool is(const piece_t compare_piece) const { return compare_piece == _piece; }

		static char my_abs(char x)
		{
			int s = x >> 8;
			return (x ^ s) - s;
		}
	};

}
