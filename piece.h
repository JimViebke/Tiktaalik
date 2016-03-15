#pragma once

#include "constants.h"

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

class Piece
{
private:
	color _color;
	piece _piece;

public:
	Piece() : _color(color::none), _piece(piece::empty) {}
	Piece(color color, piece piece) : _color(color), _piece(piece) {}
	
	// check empty
	inline bool is_empty() const { return _piece == piece::empty; }

	// check color
	inline bool is_white() const { return _color == color::white; }
	inline bool is_black() const { return _color == color::black; }
	inline bool is_color(color compare_color) const { return _color == compare_color; }

	// check piece type
	inline bool is_king() const { return _piece == piece::king; }
	inline bool is_queen() const { return _piece == piece::queen; }
	inline bool is_rook() const { return _piece == piece::rook; }
	inline bool is_bishop() const { return _piece == piece::bishop; }
	inline bool is_knight() const { return _piece == piece::knight; }
	inline bool is_pawn() const { return _piece == piece::pawn; }
	inline bool is_piece(piece compare_piece) const { return _piece == compare_piece; }

	// check entire thing
	inline bool is(Piece compare_piece) const { return _color == compare_piece._color && _piece == compare_piece._piece; }
};
