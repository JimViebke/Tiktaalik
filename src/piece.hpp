#pragma once

#include "piece_defines.hpp"

namespace chess
{
	class piece
	{
	private:
		piece_t _piece;

	public:
		constexpr piece() : _piece(empty) {}
		constexpr piece(const piece_t piece) : _piece(piece) {}

		constexpr piece_t value() const { return _piece; }
		color_t get_color() const { return _piece & detail::color_mask; }

		// check empty
		bool is_empty() const { return _piece == empty; }
		bool is_occupied() const { return _piece != empty; }

		// check color
		bool is_white() const { return (_piece & detail::color_mask) == white; }
		bool is_black() const { return !is_white(); }
		bool is_color(color_t compare_color) const { return (_piece & detail::color_mask) == compare_color; }
		bool is_color(piece piece) const { return (_piece & detail::color_mask) == (piece._piece & detail::color_mask); }
		bool is_opposing_color(color_t compare_color) const { return !is_color(compare_color); }
		bool is_opposing_color(piece piece) const { return !is_color(piece); }

		// check piece type
		bool is_king() const { return (_piece & detail::type_mask) == detail::king; }
		bool is_queen() const { return (_piece & detail::type_mask) == detail::queen; }
		bool is_rook() const { return (_piece & detail::type_mask) == detail::rook; }
		bool is_bishop() const { return (_piece & detail::type_mask) == detail::bishop; }
		bool is_knight() const { return (_piece & detail::type_mask) == detail::knight; }
		bool is_pawn() const { return (_piece & detail::type_mask) == detail::pawn; }

		// check piece type and color
		bool is(const piece compare_piece) const { return compare_piece._piece == _piece; }
		bool is(const piece_t compare_piece) const { return compare_piece == _piece; }

		char to_algebraic_char() const
		{
			if (is_knight()) return 'N';
			else if (is_bishop()) return 'B';
			else if (is_rook()) return 'R';
			else if (is_queen()) return 'Q';
			else if (is_king()) return 'K';
			else return '?';
		}
	};

}
