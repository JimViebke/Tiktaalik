#pragma once

#include "types.hpp"

namespace chess::evaluations
{
	// const evaluation_t king = 1000000.f;
	const evaluation_t queen = 9.75f;
	const evaluation_t rook = 5.00f;
	const evaluation_t bishop = 3.25f;
	const evaluation_t knight = 3.20f;
	const evaluation_t pawn = 1.00f;
}
