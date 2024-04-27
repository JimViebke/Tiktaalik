#pragma once

#include <stdint.h>

namespace chess
{
	using piece_t = int8_t;
	using color_t = piece_t;

	static constexpr piece_t white = 0;
	static constexpr piece_t black = 1;

	namespace detail
	{
		static constexpr piece_t pawn = 0 << 1;
		static constexpr piece_t knight = 1 << 1;
		static constexpr piece_t bishop = 2 << 1;
		static constexpr piece_t rook = 3 << 1;
		static constexpr piece_t queen = 4 << 1;
		static constexpr piece_t king = 5 << 1;

		static constexpr piece_t type_mask = 0b1110;
		static constexpr piece_t color_mask = 0b0001;
	}

	static constexpr piece_t empty = black + detail::king + 1;

	static constexpr size_t n_of_piece_types = 6 + 6 + 1;

	static constexpr piece_t white_pawn{ white | detail::pawn };
	static constexpr piece_t white_knight{ white | detail::knight };
	static constexpr piece_t white_bishop{ white | detail::bishop };
	static constexpr piece_t white_rook{ white | detail::rook };
	static constexpr piece_t white_queen{ white | detail::queen };
	static constexpr piece_t white_king{ white | detail::king };

	static constexpr piece_t black_pawn{ black | detail::pawn };
	static constexpr piece_t black_knight{ black | detail::knight };
	static constexpr piece_t black_bishop{ black | detail::bishop };
	static constexpr piece_t black_rook{ black | detail::rook };
	static constexpr piece_t black_queen{ black | detail::queen };
	static constexpr piece_t black_king{ black | detail::king };
}
