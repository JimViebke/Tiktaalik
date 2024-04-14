#pragma once

#include <array>
#include <stdint.h>

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
	
	enum color : int8_t
	{
		black = -1,
		white = 1,
	};

	using piece_t = int8_t;

	using evaluation_t = float;
}
