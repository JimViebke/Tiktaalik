#pragma once

#include <array>
#include <stdint.h>

#include "piece.hpp"

namespace chess
{
	enum class result : int8_t
	{
		unknown,
		white_wins_by_checkmate,
		black_wins_by_checkmate,
		draw_by_stalemate
	};

	using rank = int8_t;
	using file = int8_t;

	static constexpr color_t white = piece::white;
	static constexpr color_t black = piece::black;
}
