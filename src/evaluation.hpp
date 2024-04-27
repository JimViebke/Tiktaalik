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

		using piece_square_array = std::array<val_t, 64>;

		// via: https://www.chessprogramming.org/Simplified_Evaluation_Function
		static constexpr piece_square_array pawn_squares = {
			0, 0, 0, 0, 0, 0, 0, 0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			5, 5, 10, 25, 25, 10, 5, 5,
			0, 0, 0, 20, 20, 0, 0, 0,
			5, -5, -10, 0, 0, -10, -5, 5,
			5, 10, 10, -20, -20, 10, 10, 5,
			0, 0, 0, 0, 0, 0, 0, 0 };
		static constexpr piece_square_array knight_squares = {
			-50, -40, -30, -30, -30, -30, -40, -50,
			-40, -20, 0, 0, 0, 0, -20, -40,
			-30, 0, 10, 15, 15, 10, 0, -30,
			-30, 5, 15, 20, 20, 15, 5, -30,
			-30, 0, 15, 20, 20, 15, 0, -30,
			-30, 5, 10, 15, 15, 10, 5, -30,
			-40, -20, 0, 5, 5, 0, -20, -40,
			-50, -40, -30, -30, -30, -30, -40, -50 };
		static constexpr piece_square_array bishop_squares = {
			-20, -10, -10, -10, -10, -10, -10, -20,
			-10, 0, 0, 0, 0, 0, 0, -10,
			-10, 0, 5, 10, 10, 5, 0, -10,
			-10, 5, 5, 10, 10, 5, 5, -10,
			-10, 0, 10, 10, 10, 10, 0, -10,
			-10, 10, 10, 10, 10, 10, 10, -10,
			-10, 5, 0, 0, 0, 0, 5, -10,
			-20, -10, -10, -10, -10, -10, -10, -20 };
		static constexpr piece_square_array rook_squares = {
			0, 0, 0, 0, 0, 0, 0, 0,
			5, 10, 10, 10, 10, 10, 10, 5,
			-5, 0, 0, 0, 0, 0, 0, -5,
			-5, 0, 0, 0, 0, 0, 0, -5,
			-5, 0, 0, 0, 0, 0, 0, -5,
			-5, 0, 0, 0, 0, 0, 0, -5,
			-5, 0, 0, 0, 0, 0, 0, -5,
			0, 0, 0, 5, 5, 0, 0, 0 };
		static constexpr piece_square_array queen_squares = {
			-20, -10, -10, -5, -5, -10, -10, -20,
			-10, 0, 0, 0, 0, 0, 0, -10,
			-10, 0, 5, 5, 5, 5, 0, -10,
			-5, 0, 5, 5, 5, 5, 0, -5,
			0, 0, 5, 5, 5, 5, 0, -5,
			-10, 5, 5, 5, 5, 5, 0, -10,
			-10, 0, 5, 0, 0, 0, 0, -10,
			-20, -10, -10, -5, -5, -10, -10, -20 };
		static constexpr piece_square_array king_squares = {
			-30, -40, -40, -50, -50, -40, -40, -30,
			-30, -40, -40, -50, -50, -40, -40, -30,
			-30, -40, -40, -50, -50, -40, -40, -30,
			-30, -40, -40, -50, -50, -40, -40, -30,
			-20, -30, -30, -40, -40, -30, -30, -20,
			-10, -20, -20, -20, -20, -20, -20, -10,
			20, 20, 0, 0, 0, 0, 20, 20,
			20, 30, 10, 0, 0, 10, 30, 20 };

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

		/*
		Flip lookup indexes so we can map 0-63 to:
		{ 56, 57, 58, 59, 60, 61, 62, 63,
		  ...
		  8, 9, 10, 11, 12, 13, 14, 15,
		  0, 1, 2, 3, 4, 5, 6, 7 }
		*/
		static constexpr std::array<uint8_t, 64> mirror_idx = []() consteval
		{
			std::array<uint8_t, 64> array{};
			auto it = array.begin();
			for (size_t i = 0; i < 64; i += 8)
			{
				for (size_t j = 0; j < 8; ++j)
				{
					*it++ = uint8_t((56ull - i) + j);
				}
			}
			return array;
		}();

		static constexpr std::array<piece_square_array, n_of_piece_types> piece_square_evals = []() consteval
		{
			std::array<piece_square_array, n_of_piece_types> array{};

			array[white_pawn] = pawn_squares;
			array[white_knight] = knight_squares;
			array[white_bishop] = bishop_squares;
			array[white_rook] = rook_squares;
			array[white_queen] = queen_squares;
			array[white_king] = king_squares;

			// for each of six piece types (indexes 0,2,4,6,8,10) * 64 squares
			for (size_t i = 0; i < 11; i += 2)
			{
				for (size_t j = 0; j < 64; ++j)
				{
					// Copy the evaluation (mirrored across the centerline), and invert the value.
					array[i + 1][mirror_idx[j]] = 0 - array[i][j];
				}
			}

			return array;
		}();

		// If a white knight on e3 is worth N points, then
		// a black knight on e6 should be worth -N points.
		static_assert(piece_square_evals[white_knight][to_index(rank(2), file(4))] ==
					  piece_square_evals[black_knight][to_index(rank(5), file(4))] * -1);
		// test queens
		static_assert(piece_square_evals[white_queen][to_index(rank(0), file(7))] ==
					  piece_square_evals[black_queen][to_index(rank(7), file(7))] * -1);
		// test c pawns
		static_assert(piece_square_evals[white_pawn][to_index(rank(1), file(2))] ==
					  piece_square_evals[black_pawn][to_index(rank(6), file(2))] * -1);
	}
}