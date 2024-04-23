#pragma once

#include "evaluation.hpp"
#include "piece_defines.hpp"

namespace chess
{
	using piece_t = piece_defines::piece_t;
	using color_t = piece_defines::color_t;

	class piece
	{
	public:
		static constexpr piece_t empty = piece_defines::empty;

		static constexpr piece_t pawn = piece_defines::pawn;
		static constexpr piece_t knight = piece_defines::knight;
		static constexpr piece_t bishop = piece_defines::bishop;
		static constexpr piece_t rook = piece_defines::rook;
		static constexpr piece_t queen = piece_defines::queen;
		static constexpr piece_t king = piece_defines::king;

		static constexpr piece_t white = piece_defines::white;
		static constexpr piece_t black = piece_defines::black;

	private:
		static constexpr auto type_mask = piece_defines::type_mask;
		static constexpr auto color_mask = piece_defines::color_mask;

		piece_t _piece;

	public:
		explicit piece() : _piece(piece::empty) {}
		explicit piece(const piece_t piece) : _piece(piece) {}

		piece_t get_piece() const { return _piece; }
		color_t get_color() const { return _piece & color_mask; }

		eval_t eval() const { return eval::eval_lookup[_piece]; }

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
