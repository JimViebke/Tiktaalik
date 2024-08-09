#pragma once

#include "piece_defines.hpp"

namespace chess
{
	class piece
	{
	private:
		piece_t _piece;

	public:
		constexpr piece(const piece_t piece = empty) : _piece(piece) {}

		constexpr piece_t value() const { return _piece; }
		color_t other_color() const { return chess::other_color(get_color()); }

		bool is_empty() const { return value() == empty; }
		bool is_occupied() const { return !is_empty(); }

		bool is_white() const { return is_color(white); }
		bool is_black() const { return !is_white(); }
		bool is_color(color_t compare_color) const { return get_color() == compare_color; }
		bool is_color(piece piece) const { return is_color(piece.get_color()); }
		bool is_opposing_color(color_t compare_color) const { return !is_color(compare_color); }
		bool is_opposing_color(piece piece) const { return !is_color(piece); }

		bool is_king() const { return get_type() == king; }
		bool is_queen() const { return get_type() == queen; }
		bool is_rook() const { return get_type() == rook; }
		bool is_bishop() const { return get_type() == bishop; }
		bool is_knight() const { return get_type() == knight; }
		bool is_pawn() const { return get_type() == pawn; }

		bool is(const piece_t compare_piece_t) const { return value() == compare_piece_t; }
		bool is(const piece compare_piece) const { return is(compare_piece.value()); }

		char to_algebraic_char() const
		{
			if (is_knight())
				return 'N';
			else if (is_bishop())
				return 'B';
			else if (is_rook())
				return 'R';
			else if (is_queen())
				return 'Q';
			else if (is_king())
				return 'K';
			else
				return '?';
		}
		char to_promoted_char() const
		{
			if (is_knight())
				return 'n';
			else if (is_bishop())
				return 'b';
			else if (is_rook())
				return 'r';
			else if (is_queen())
				return 'q';
			else
				return '?';
		}

	private:
		static constexpr piece_t type_mask = 0b1110;
		static constexpr piece_t color_mask = 0b0001;
		color_t get_color() const { return value() & color_mask; }
		piece_t get_type() const { return value() & type_mask; }
	};
}
