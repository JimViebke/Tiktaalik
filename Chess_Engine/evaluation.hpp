#pragma once

#include <array>
#include <limits>

#include "piece_defines.hpp"

namespace chess
{
	using eval_t = int16_t;

	namespace eval
	{
		static constexpr eval_t eval_min = std::numeric_limits<eval_t>::min();
		static constexpr eval_t eval_max = std::numeric_limits<eval_t>::max();

		// const eval_t king = 10'000;
		constexpr eval_t queen = 975;
		constexpr eval_t rook = 500;
		constexpr eval_t bishop = 325;
		constexpr eval_t knight = 320;
		constexpr eval_t pawn = 100;

		static constexpr std::array eval_lookup = []() consteval
		{
			namespace piece = piece_defines;

			std::array<eval_t, 15> vals{};

			const auto white = piece::white;
			const auto black = piece::black;

			vals[white | piece::queen] = queen;
			vals[white | piece::rook] = rook;
			vals[white | piece::bishop] = bishop;
			vals[white | piece::knight] = knight;
			vals[white | piece::pawn] = pawn;

			vals[black | piece::queen] = -queen;
			vals[black | piece::rook] = -rook;
			vals[black | piece::bishop] = -bishop;
			vals[black | piece::knight] = -knight;
			vals[black | piece::pawn] = -pawn;

			return vals;
		}();
	}
}
