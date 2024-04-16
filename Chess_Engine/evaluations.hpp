#pragma once

#include <array>

namespace chess
{
	using evaluation_t = float;

	namespace evaluations
	{
		// const evaluation_t king = 1000000.f;
		constexpr evaluation_t queen = 9.75f;
		constexpr evaluation_t rook = 5.00f;
		constexpr evaluation_t bishop = 3.25f;
		constexpr evaluation_t knight = 3.20f;
		constexpr evaluation_t pawn = 1.00f;

		static constexpr std::array eval_lookup = {
			evaluation_t{ 0 }, // 0 (empty piece)
			pawn, knight, bishop, rook, queen, evaluation_t{ 0 }, // 1-6
			evaluation_t{ 0 }, evaluation_t{ 0 }, // 7, 8 (invalid)
			-pawn, -knight, -bishop, -rook, -queen, evaluation_t{ 0 } // 9-14
		};
	}
}
