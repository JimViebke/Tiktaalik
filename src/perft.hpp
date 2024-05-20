#pragma once

#include "types.hpp"

namespace chess
{
	template<color_t color_to_move>
	void perft(const depth_t max_depth);

	template<color_t color_to_move>
	void divide(const depth_t max_depth);
}
