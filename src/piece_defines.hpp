#pragma once

#include <stdint.h>

#include "types.hpp"

namespace chess
{
	using piece_t = int8_t;
	using color_t = piece_t;

	static constexpr piece_t white = 0;
	static constexpr piece_t black = 1;

	constexpr inline color_t other_color(const color_t color) { return (color == white) ? black : white; }

	template<color_t color>
	class colorable
	{
	public:
		static consteval bool white_to_move() { return color == white; }
		static consteval bool black_to_move() { return !white_to_move(); }

		static consteval color_t color_to_move() { return color; }
		static consteval color_t other_color() { return chess::other_color(color); }
	};

	namespace detail
	{
		static constexpr piece_t type_mask = 0b1110;
		static constexpr piece_t color_mask = 0b0001;
	}

	static constexpr piece_t pawn = 0 << 1;
	static constexpr piece_t knight = 1 << 1;
	static constexpr piece_t bishop = 2 << 1;
	static constexpr piece_t rook = 3 << 1;
	static constexpr piece_t queen = 4 << 1;
	static constexpr piece_t king = 5 << 1;

	static constexpr piece_t empty = black + king + 1;
	static constexpr piece_t other_piece = empty;

	static constexpr size_t n_of_piece_types = 6 + 6 + 1;

	static constexpr piece_t white_pawn{ white | pawn };
	static constexpr piece_t white_knight{ white | knight };
	static constexpr piece_t white_bishop{ white | bishop };
	static constexpr piece_t white_rook{ white | rook };
	static constexpr piece_t white_queen{ white | queen };
	static constexpr piece_t white_king{ white | king };

	static constexpr piece_t black_pawn{ black | pawn };
	static constexpr piece_t black_knight{ black | knight };
	static constexpr piece_t black_bishop{ black | bishop };
	static constexpr piece_t black_rook{ black | rook };
	static constexpr piece_t black_queen{ black | queen };
	static constexpr piece_t black_king{ black | king };
}
