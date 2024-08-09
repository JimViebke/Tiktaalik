#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "piece.hpp"
#include "piece_defines.hpp"
#include "types.hpp"

namespace chess
{
	namespace eval
	{
		static constexpr size_t max_ply = 256;

		static constexpr eval_t mate = std::numeric_limits<eval_t>::max() - max_ply;
		static constexpr eval_t mate_threshold = mate - max_ply;

		inline bool found_mate(const eval_t eval)
		{
			return eval >= eval::mate_threshold || eval <= -eval::mate_threshold;
		}

		constexpr eval_t queen_eval = 975;
		constexpr eval_t rook_eval = 500;
		constexpr eval_t bishop_eval = 325;
		constexpr eval_t knight_eval = 320;
		constexpr eval_t pawn_eval = 100;

		using piece_square_array = std::array<int8_t, 64>;

		// via: https://www.chessprogramming.org/Simplified_Evaluation_Function
		static constexpr piece_square_array pawn_squares = {
		    0, 0, 0, 0, 0, 0, 0, 0,         //
		    50, 50, 50, 50, 50, 50, 50, 50, //
		    10, 10, 20, 30, 30, 20, 10, 10, //
		    5, 5, 10, 25, 25, 10, 5, 5,     //
		    0, 0, 0, 20, 20, 0, 0, 0,       //
		    5, -5, -10, 0, 0, -10, -5, 5,   //
		    5, 10, 10, -20, -20, 10, 10, 5, //
		    0, 0, 0, 0, 0, 0, 0, 0          //
		};
		static constexpr piece_square_array knight_squares = {
		    -50, -40, -30, -30, -30, -30, -40, -50, //
		    -40, -20, 0, 0, 0, 0, -20, -40,         //
		    -30, 0, 10, 15, 15, 10, 0, -30,         //
		    -30, 5, 15, 20, 20, 15, 5, -30,         //
		    -30, 0, 15, 20, 20, 15, 0, -30,         //
		    -30, 5, 10, 15, 15, 10, 5, -30,         //
		    -40, -20, 0, 5, 5, 0, -20, -40,         //
		    -50, -40, -30, -30, -30, -30, -40, -50  //
		};
		static constexpr piece_square_array bishop_squares = {
		    -20, -10, -10, -10, -10, -10, -10, -20, //
		    -10, 0, 0, 0, 0, 0, 0, -10,             //
		    -10, 0, 5, 10, 10, 5, 0, -10,           //
		    -10, 5, 5, 10, 10, 5, 5, -10,           //
		    -10, 0, 10, 10, 10, 10, 0, -10,         //
		    -10, 10, 10, 10, 10, 10, 10, -10,       //
		    -10, 5, 0, 0, 0, 0, 5, -10,             //
		    -20, -10, -10, -10, -10, -10, -10, -20  //
		};
		static constexpr piece_square_array rook_squares = {
		    0, 0, 0, 0, 0, 0, 0, 0,       //
		    5, 10, 10, 10, 10, 10, 10, 5, //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    0, 0, 0, 5, 5, 0, 0, 0        //
		};
		static constexpr piece_square_array queen_squares = {
		    -20, -10, -10, -5, -5, -10, -10, -20, //
		    -10, 0, 0, 0, 0, 0, 0, -10,           //
		    -10, 0, 5, 5, 5, 5, 0, -10,           //
		    -5, 0, 5, 5, 5, 5, 0, -5,             //
		    0, 0, 5, 5, 5, 5, 0, -5,              //
		    -10, 5, 5, 5, 5, 5, 0, -10,           //
		    -10, 0, 5, 0, 0, 0, 0, -10,           //
		    -20, -10, -10, -5, -5, -10, -10, -20  //
		};
		static constexpr piece_square_array king_squares = {
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -20, -30, -30, -40, -40, -30, -30, -20, //
		    -10, -20, -20, -20, -20, -20, -20, -10, //
		    20, 20, 0, 0, 0, 0, 20, 20,             //
		    20, 30, 10, 0, 0, 10, 30, 20            //
		};

		static constexpr std::array material_values = []() consteval
		{
			std::array<eval_t, n_of_piece_types - 1> vals{};

			vals[chess::pawn >> 1] = pawn_eval;
			vals[chess::knight >> 1] = knight_eval;
			vals[chess::bishop >> 1] = bishop_eval;
			vals[chess::rook >> 1] = rook_eval;
			vals[chess::queen >> 1] = queen_eval;

			return vals;
		}();

		template <color_t color>
		inline constexpr eval_t piece_eval(const piece piece)
		{
			const eval_t eval = eval::material_values[piece.value() >> 1];
			return (color == white) ? eval : -eval;
		}

		static constexpr std::array<piece_square_array, n_of_piece_types> piece_square_evals = []() consteval
		{
			std::array<piece_square_array, n_of_piece_types> piece_square_evals{};

			piece_square_evals[chess::pawn >> 1] = pawn_squares;
			piece_square_evals[chess::knight >> 1] = knight_squares;
			piece_square_evals[chess::bishop >> 1] = bishop_squares;
			piece_square_evals[chess::rook >> 1] = rook_squares;
			piece_square_evals[chess::queen >> 1] = queen_squares;
			piece_square_evals[chess::king >> 1] = king_squares;

			return piece_square_evals;
		}();

		template <color_t color>
		inline constexpr eval_t piece_square_eval(const piece piece, size_t index)
		{
			if constexpr (color == black) index ^= 0b111000;
			const eval_t eval = piece_square_evals[piece.value() >> 1][index];
			return (color == white) ? eval : -eval;
		}

		// If a white knight on e3 is worth N points, then
		// a black knight on e6 should be worth -N points.
		static_assert(piece_square_eval<white>(knight, to_index(rank(2), file(4))) ==
		              piece_square_eval<black>(knight, to_index(rank(5), file(4))) * -1);
		// test queens
		static_assert(piece_square_eval<white>(queen, to_index(rank(0), file(7))) ==
		              piece_square_eval<black>(queen, to_index(rank(7), file(7))) * -1);
		// test c pawns
		static_assert(piece_square_eval<white>(pawn, to_index(rank(1), file(2))) ==
		              piece_square_eval<black>(pawn, to_index(rank(6), file(2))) * -1);
	}
}
