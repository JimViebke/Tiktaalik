#pragma once

#include "constants.h"

namespace
{
	enum color : char {
		black = -1,
		white = 1,
	};
}

enum piece : char {
	black_king = -6,
	black_queen = -5,
	black_rook = -4,
	black_bishop = -3,
	black_knight = -2,
	black_pawn = -1,

	empty = 0,

	white_pawn = 1,
	white_knight = 2,
	white_bishop = 3,
	white_rook = 4,
	white_queen = 5,
	white_king = 6
};

class Piece
{
private:
	piece _piece;

public:
	inline Piece() : _piece(piece::empty) {}
	inline Piece(piece piece) : _piece(piece) {}

	// check empty
	inline bool is_empty() const { return _piece == piece::empty; }
	inline bool is_occupied() const { return _piece != piece::empty; }

	// check color
	inline bool is_white() const { return _piece > 0; }
	inline bool is_black() const { return _piece < 0; }
	inline bool is_color(color compare_color) const { return is_occupied() && (_piece ^ compare_color) >= 0; }
	inline bool is_color(Piece piece) const { return (_piece ^ piece._piece) >= 0; }
	inline bool is_opposing_color(color compare_color) const { return (_piece ^ compare_color) < 0; }
	inline bool is_opposing_color(Piece piece) const { return (_piece ^ piece._piece) < 0; }

	// check piece type
	inline bool is_king() const { return my_abs(_piece) == piece::white_king; }
	inline bool is_queen() const { return my_abs(_piece) == piece::white_queen; }
	inline bool is_rook() const { return my_abs(_piece) == piece::white_rook; }
	inline bool is_bishop() const { return my_abs(_piece) == piece::white_bishop; }
	inline bool is_knight() const { return my_abs(_piece) == piece::white_knight; }
	inline bool is_pawn() const { return my_abs(_piece) == piece::white_pawn; }
	inline bool is_piece(piece compare_piece) const { return _piece == compare_piece; }

	// check entire piece
	inline bool is(Piece compare_piece) const { return compare_piece._piece == _piece; }
	inline bool is(piece compare_piece) const { return compare_piece == _piece; }

	static inline char my_abs(char x)
	{
		int s = x >> 8;
		return (x ^ s) - s;
	}
};
