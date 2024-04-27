#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "piece_defines.hpp"
#include "position.hpp"
#include "types.hpp"

namespace chess
{
	namespace eval
	{
		using val_t = int8_t;

		static constexpr eval_t eval_min = std::numeric_limits<eval_t>::min();
		static constexpr eval_t eval_max = std::numeric_limits<eval_t>::max();

		// const eval_t king = 10'000;
		constexpr eval_t queen = 975;
		constexpr eval_t rook = 500;
		constexpr eval_t bishop = 325;
		constexpr eval_t knight = 320;
		constexpr eval_t pawn = 100;



		static constexpr std::array material_values = []() consteval
		{
			std::array<eval_t, n_of_piece_types> vals{};

			vals[white_queen] = queen;
			vals[white_rook] = rook;
			vals[white_bishop] = bishop;
			vals[white_knight] = knight;
			vals[white_pawn] = pawn;

			vals[black_queen] = -queen;
			vals[black_rook] = -rook;
			vals[black_bishop] = -bishop;
			vals[black_knight] = -knight;
			vals[black_pawn] = -pawn;

			return vals;
		}();

		inline constexpr eval_t eval(const piece piece) { return eval::material_values[piece.get_piece()]; }
	}
}
