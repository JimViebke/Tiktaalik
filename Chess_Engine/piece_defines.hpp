#pragma once

#include <stdint.h>

namespace chess
{
	namespace piece_defines
	{
		using piece_t = int8_t;
		using color_t = piece_t;

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
	}
}
