#pragma once

#include "constants.h"

namespace
{
	enum class color : char {
		none,

		white,
		black,
	};

	enum class piece : char {
		empty,

		pawn,
		knight,
		bishop,
		rook,
		queen,
		king
	};
}

class Piece
{
private:
	color _color;
	piece _piece;

public:
	inline Piece() : _color(color::none), _piece(piece::empty) {}
	inline Piece(color color, piece piece) : _color(color), _piece(piece) {}

	// check empty
	inline bool is_empty() const { return _piece == piece::empty; }
	inline bool is_occupied() const { return _piece != piece::empty; }

	// check color
	inline bool is_white() const { return _color == color::white; }
	inline bool is_black() const { return _color == color::black; }
	inline bool is_color(color compare_color) const { return _color == compare_color; }
	inline bool is_color(Piece piece) const { return piece._color == _color; }
	inline bool is_opposing_color(color compare_color) const { return _color != compare_color; }
	inline bool is_opposing_color(Piece piece) const { return piece._color != _color; }

	// check piece type
	inline bool is_king() const { return _piece == piece::king; }
	inline bool is_queen() const { return _piece == piece::queen; }
	inline bool is_rook() const { return _piece == piece::rook; }
	inline bool is_bishop() const { return _piece == piece::bishop; }
	inline bool is_knight() const { return _piece == piece::knight; }
	inline bool is_pawn() const { return _piece == piece::pawn; }
	inline bool is_piece(piece compare_piece) const { return _piece == compare_piece; }

	// check entire piece
	inline bool is(Piece compare_piece) const { return _color == compare_piece._color && _piece == compare_piece._piece; }
	inline bool is(const color check_color, const piece check_piece) const { return _piece == check_piece && _color == check_color; }
};
