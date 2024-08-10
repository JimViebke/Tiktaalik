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

		static constexpr size_t pse_size = 64 * n_of_piece_types * 2;

		static constexpr std::array<int8_t, pse_size> piece_square_evals = {
		    // pawn mg
		    0, 0, 0, 0, 0, 0, 0, 0,         //
		    50, 50, 50, 50, 50, 50, 50, 50, //
		    10, 10, 20, 30, 30, 20, 10, 10, //
		    5, 5, 10, 25, 25, 10, 5, 5,     //
		    0, 0, 0, 20, 20, 0, 0, 0,       //
		    5, -5, -10, 0, 0, -10, -5, 5,   //
		    5, 10, 10, -20, -20, 10, 10, 5, //
		    0, 0, 0, 0, 0, 0, 0, 0,         //
		    // pawn eg
		    0, 0, 0, 0, 0, 0, 0, 0,         //
		    50, 50, 50, 50, 50, 50, 50, 50, //
		    10, 10, 20, 30, 30, 20, 10, 10, //
		    5, 5, 10, 25, 25, 10, 5, 5,     //
		    0, 0, 0, 20, 20, 0, 0, 0,       //
		    5, -5, -10, 0, 0, -10, -5, 5,   //
		    5, 10, 10, -20, -20, 10, 10, 5, //
		    0, 0, 0, 0, 0, 0, 0, 0,         //
		    // knight mg
		    -50, -40, -30, -30, -30, -30, -40, -50, //
		    -40, -20, 0, 0, 0, 0, -20, -40,         //
		    -30, 0, 10, 15, 15, 10, 0, -30,         //
		    -30, 5, 15, 20, 20, 15, 5, -30,         //
		    -30, 0, 15, 20, 20, 15, 0, -30,         //
		    -30, 5, 10, 15, 15, 10, 5, -30,         //
		    -40, -20, 0, 5, 5, 0, -20, -40,         //
		    -50, -40, -30, -30, -30, -30, -40, -50, //
		    // knight eg
		    -50, -40, -30, -30, -30, -30, -40, -50, //
		    -40, -20, 0, 0, 0, 0, -20, -40,         //
		    -30, 0, 10, 15, 15, 10, 0, -30,         //
		    -30, 5, 15, 20, 20, 15, 5, -30,         //
		    -30, 0, 15, 20, 20, 15, 0, -30,         //
		    -30, 5, 10, 15, 15, 10, 5, -30,         //
		    -40, -20, 0, 5, 5, 0, -20, -40,         //
		    -50, -40, -30, -30, -30, -30, -40, -50, //
		    // bishop mg
		    -20, -10, -10, -10, -10, -10, -10, -20, //
		    -10, 0, 0, 0, 0, 0, 0, -10,             //
		    -10, 0, 5, 10, 10, 5, 0, -10,           //
		    -10, 5, 5, 10, 10, 5, 5, -10,           //
		    -10, 0, 10, 10, 10, 10, 0, -10,         //
		    -10, 10, 10, 10, 10, 10, 10, -10,       //
		    -10, 5, 0, 0, 0, 0, 5, -10,             //
		    -20, -10, -10, -10, -10, -10, -10, -20, //
		    // bishop eg
		    -20, -10, -10, -10, -10, -10, -10, -20, //
		    -10, 0, 0, 0, 0, 0, 0, -10,             //
		    -10, 0, 5, 10, 10, 5, 0, -10,           //
		    -10, 5, 5, 10, 10, 5, 5, -10,           //
		    -10, 0, 10, 10, 10, 10, 0, -10,         //
		    -10, 10, 10, 10, 10, 10, 10, -10,       //
		    -10, 5, 0, 0, 0, 0, 5, -10,             //
		    -20, -10, -10, -10, -10, -10, -10, -20, //
		    // rook mg
		    0, 0, 0, 0, 0, 0, 0, 0,       //
		    5, 10, 10, 10, 10, 10, 10, 5, //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    0, 0, 0, 5, 5, 0, 0, 0,       //
		    // rook eg
		    0, 0, 0, 0, 0, 0, 0, 0,       //
		    5, 10, 10, 10, 10, 10, 10, 5, //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    -5, 0, 0, 0, 0, 0, 0, -5,     //
		    0, 0, 0, 5, 5, 0, 0, 0,       //
		    // queen mg
		    -20, -10, -10, -5, -5, -10, -10, -20, //
		    -10, 0, 0, 0, 0, 0, 0, -10,           //
		    -10, 0, 5, 5, 5, 5, 0, -10,           //
		    -5, 0, 5, 5, 5, 5, 0, -5,             //
		    0, 0, 5, 5, 5, 5, 0, -5,              //
		    -10, 5, 5, 5, 5, 5, 0, -10,           //
		    -10, 0, 5, 0, 0, 0, 0, -10,           //
		    -20, -10, -10, -5, -5, -10, -10, -20, //
		    // queen eg
		    -20, -10, -10, -5, -5, -10, -10, -20, //
		    -10, 0, 0, 0, 0, 0, 0, -10,           //
		    -10, 0, 5, 5, 5, 5, 0, -10,           //
		    -5, 0, 5, 5, 5, 5, 0, -5,             //
		    0, 0, 5, 5, 5, 5, 0, -5,              //
		    -10, 5, 5, 5, 5, 5, 0, -10,           //
		    -10, 0, 5, 0, 0, 0, 0, -10,           //
		    -20, -10, -10, -5, -5, -10, -10, -20, //
		    // king mg
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -20, -30, -30, -40, -40, -30, -30, -20, //
		    -10, -20, -20, -20, -20, -20, -20, -10, //
		    20, 20, 0, 0, 0, 0, 20, 20,             //
		    20, 30, 10, 0, 0, 10, 30, 20,           //
		    // king eg
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -30, -40, -40, -50, -50, -40, -40, -30, //
		    -20, -30, -30, -40, -40, -30, -30, -20, //
		    -10, -20, -20, -20, -20, -20, -20, -10, //
		    20, 20, 0, 0, 0, 0, 20, 20,             //
		    20, 30, 10, 0, 0, 10, 30, 20,           //
		};
		static_assert(piece_square_evals.size() == 64 * n_of_piece_types * 2);

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
			const eval_t eval = eval::material_values[piece >> 1];
			return (color == white) ? eval : -eval;
		}

		namespace detail
		{
			template <color_t color>
			inline constexpr eval_t piece_square_eval(const piece piece, size_t index)
			{
				if constexpr (color == black) index ^= 0b111000;
				const eval_t eval = piece_square_evals[(piece >> 1) * 128 + index];
				return (color == white) ? eval : -eval;
			}
		}

		template <color_t color>
		inline constexpr eval_t piece_square_eval_mg(const piece piece, const size_t index)
		{
			return detail::piece_square_eval<color>(piece, index);
		}
		template <color_t color>
		inline constexpr eval_t piece_square_eval_eg(const piece piece, const size_t index)
		{
			return detail::piece_square_eval<color>(piece, index + 64);
		}

		// If a white knight on e3 is worth N points, then
		// a black knight on e6 should be worth -N points.
		static_assert(piece_square_eval_mg<white>(knight, to_index(rank(2), file(4))) ==
		              piece_square_eval_mg<black>(knight, to_index(rank(5), file(4))) * -1);
		// test queens
		static_assert(piece_square_eval_mg<white>(queen, to_index(rank(0), file(7))) ==
		              piece_square_eval_mg<black>(queen, to_index(rank(7), file(7))) * -1);
		// test c pawns
		static_assert(piece_square_eval_eg<white>(pawn, to_index(rank(1), file(2))) ==
		              piece_square_eval_eg<black>(pawn, to_index(rank(6), file(2))) * -1);
	}
}
