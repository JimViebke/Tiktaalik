#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "config.hpp"
#include "defines.hpp"

namespace chess
{
	namespace eval
	{
		constexpr size_t max_ply = 256;

		constexpr eval_t mate = std::numeric_limits<eval_t>::max() - max_ply;
		constexpr eval_t mate_threshold = mate - max_ply;

		inline bool found_mate(const eval_t eval)
		{
			return eval >= eval::mate_threshold || eval <= -eval::mate_threshold;
		}

		constexpr std::array<eval_t, n_of_piece_types - 1> material_values = {100, 320, 325, 500, 975};

		constexpr std::array<phase_t, n_of_piece_types - 1> phase_weights = {0, 3, 3, 5, 10};

		constexpr size_t total_phase = 16 * phase_weights[pawn] +  //
		                               4 * phase_weights[knight] + //
		                               4 * phase_weights[bishop] + //
		                               4 * phase_weights[rook] +   //
		                               2 * phase_weights[queen];   //
		static_assert(std::popcount(total_phase) == 1);

		template <color color, piece piece>
		inline constexpr eval_t piece_eval()
		{
			constexpr eval_t eval = eval::material_values[piece];
			return (color == white) ? eval : -eval;
		}
		template <color color>
		inline constexpr eval_t piece_eval(const piece piece)
		{
			const eval_t eval = eval::material_values[piece];
			return (color == white) ? eval : -eval;
		}

		constexpr size_t pse_size = 64 * n_of_piece_types * 2;

#if !tuning
		constexpr std::array<int8_t, pse_size> ps_evals = {
		    // pawn midgame:
		    0, 0, 0, 0, 0, 0, 0, 0,               //
		    125, 125, 127, 126, 127, 100, 6, 127, //
		    -30, 115, 115, 90, 125, 125, 70, 125, //
		    15, 5, -20, 25, 90, 70, 95, 55,       //
		    -40, -15, -15, 10, -5, 25, 45, 0,     //
		    -25, -15, -70, -85, -25, 25, 35, 20,  //
		    -35, -25, -55, -75, -95, 15, 35, -10, //
		    0, 0, 0, 0, 0, 0, 0, 0,               //
		    // pawn endgame:
		    0, 0, 0, 0, 0, 0, 0, 0,             //
		    85, 125, 111, 97, 76, 50, 127, 22,  //
		    70, 25, 85, 25, 15, 55, 45, 10,     //
		    35, 35, 5, 20, -10, -15, 0, 5,      //
		    40, 40, -5, -15, -20, 5, 0, 0,      //
		    30, 15, -5, 45, -10, -25, -15, -10, //
		    40, 40, 50, 15, 35, 5, 15, 5,       //
		    0, 0, 0, 0, 0, 0, 0, 0,             //
		    // knight midgame:
		    -128, 70, -120, -5, 35, 127, 113, -52,      //
		    -125, -125, -100, 120, 126, 125, -117, 99,  //
		    -125, 125, 60, 40, 125, 125, 125, 125,      //
		    -20, -20, 125, 25, 50, 126, 11, 127,        //
		    -75, 126, 5, -40, -10, 45, 125, -15,        //
		    -35, 10, -15, -65, 40, -35, -25, 11,        //
		    -126, -125, -25, -40, -80, 106, -125, -128, //
		    -128, -70, -127, -95, -90, -125, -65, 26,   //
		    // knight endgame:
		    -128, -90, 35, 20, -55, -27, -19, 41,      //
		    40, -45, 36, 34, 80, 0, -10, -1,           //
		    -61, -45, 50, 25, -5, 15, 55, 10,          //
		    -30, -5, -35, 75, 45, 5, 5, 17,            //
		    -20, -33, 0, 45, 5, -25, 10, -50,          //
		    -80, -30, -10, -10, -25, -15, 0, -54,      //
		    10, -80, -40, -35, -25, -120, -50, -71,    //
		    -128, -115, -101, -25, -40, -95, -65, -30, //
		    // bishop midgame:
		    -125, -125, -124, -31, 66, 125, 115, -128, //
		    -88, -100, 75, -100, 125, 127, 125, -59,   //
		    110, 60, 10, 56, 125, -80, 127, 127,       //
		    0, -75, -5, 25, 125, 125, 25, 125,         //
		    -19, 5, 60, 75, 80, 20, -25, -55,          //
		    40, -75, -25, 10, 30, 50, 0, 80,           //
		    115, -65, 125, -5, -50, -65, 5, 6,         //
		    20, -20, -45, -95, -25, -65, 102, 38,      //
		    // bishop endgame:
		    50, 65, 15, 35, -5, 30, 10, -11,       //
		    22, 45, 0, 10, 15, 32, -8, 15,         //
		    5, -10, -1, -10, -25, 65, 23, 7,       //
		    -15, 59, 25, 50, -25, -10, 50, 0,      //
		    0, 0, -1, 5, 5, 15, 36, -5,            //
		    0, 5, 5, -15, 0, -15, 0, -35,          //
		    -85, -30, -40, -20, -5, -15, -35, -65, //
		    -70, -20, -35, 10, -25, -45, -11, -30, //
		    // rook midgame:
		    -60, -20, 60, 100, 105, 80, 75, 125,      //
		    70, 20, 90, 80, 124, 125, 127, 127,       //
		    55, 55, 100, 126, 75, -15, 125, 39,       //
		    70, 80, -10, 75, 25, 35, 25, -80,         //
		    15, -20, -5, -120, 55, 30, -40, 115,      //
		    -125, -100, -5, -75, -15, -24, 15, -55,   //
		    -50, -127, -115, -90, -95, -65, -65, -55, //
		    -100, -85, -80, -65, -35, -70, -60, -115, //
		    // rook endgame:
		    75, 85, 30, 35, 60, 50, 75, 25, //
		    60, 25, 70, 50, 60, 60, 3, 61,  //
		    55, 50, 64, 56, 25, 95, 45, 55, //
		    35, 55, 80, 30, 75, 85, 50, 90, //
		    40, 65, 80, 110, 20, 45, 5, 15, //
		    50, 60, 10, 50, 20, 30, 80, 20, //
		    -5, 41, 60, 30, 10, 40, 30, 11, //
		    5, 5, 20, 20, 20, 0, 20, -5,    //
		    // queen midgame:
		    85, 66, 40, 85, 95, 127, 125, 14,           //
		    -85, 40, 125, 74, 127, 127, -87, 125,       //
		    50, -40, 70, 21, 90, 125, 127, 115,         //
		    -35, 59, 115, -65, 25, 10, 125, 120,        //
		    -15, -105, -50, 15, -125, -60, 30, 55,      //
		    -96, -85, -90, -105, -40, 10, 126, 38,      //
		    -126, -95, -40, -95, -100, -125, 60, -15,   //
		    10, -125, -125, -25, -113, -45, -128, -128, //
		    // queen endgame:
		    -75, 15, 30, 20, 60, 42, 20, 76,        //
		    50, 40, 25, 90, 39, 77, 127, 86,        //
		    -70, 40, 25, -4, 50, 80, 52, 111,       //
		    70, 30, -30, 65, 120, 80, 35, 45,       //
		    -95, 90, 55, 35, 75, 120, 80, 105,      //
		    10, 35, -14, 25, -20, -65, -125, -126,  //
		    50, -40, 10, 40, 10, -40, -90, -25,     //
		    -5, -45, -60, -80, -127, -85, -36, -71, //
		    // king midgame:
		    -96, 27, 125, 64, 127, -128, 15, 21,   //
		    -125, 127, 16, 127, 60, 120, -61, -26, //
		    125, 30, -55, 65, 0, 70, 125, -85,     //
		    -45, 25, -22, -126, -41, 20, 90, 105,  //
		    127, 40, 55, 5, 5, 35, 127, -105,      //
		    -125, -15, -15, -25, -95, 0, -70, -50, //
		    -50, 20, -55, -25, -50, -70, -20, 35,  //
		    -51, 45, 0, -15, -25, -55, 5, -30,     //
		    // king endgame:
		    -126, 22, -65, 53, -48, 42, 50, -60,   //
		    25, -23, 10, -14, -15, -31, 44, -20,   //
		    20, 10, -5, -15, -50, -60, -15, -20,   //
		    15, 20, -3, 24, 5, 1, -45, 10,         //
		    -37, 0, -5, 0, 0, 10, -24, 20,         //
		    15, 15, 0, -5, 5, -15, 0, -10,         //
		    20, -40, 0, -25, -25, -10, -15, -25,   //
		    100, -40, -10, -30, -35, -30, -25, -5, //
		};

#else
		// The ps_evals array will be mutable, unavailable at compile time,
		// and provided by tuning.cpp.
		extern std::array<int8_t, pse_size> ps_evals;
#endif
		static_assert(ps_evals.size() == 64 * n_of_piece_types * 2);

		namespace detail
		{
			template <color color>
			inline constexpr_if_not_tuning eval_t piece_square_eval(const piece piece, size_t index)
			{
				if constexpr_if_not_tuning (color == black) index ^= 0b111000;
				const eval_t eval = ps_evals[piece * 128 + index];
				return (color == white) ? eval : -eval;
			}
			template <color color, piece piece>
			inline constexpr_if_not_tuning eval_t piece_square_eval(size_t index)
			{
				if constexpr_if_not_tuning (color == black) index ^= 0b111000;
				const eval_t eval = ps_evals[piece * 128 + index];
				return (color == white) ? eval : -eval;
			}
		}

		template <color color>
		inline constexpr_if_not_tuning eval_t piece_square_eval_mg(const piece piece, const size_t index)
		{
			return detail::piece_square_eval<color>(piece, index);
		}
		template <color color, piece piece>
		inline constexpr_if_not_tuning eval_t piece_square_eval_mg(const size_t index)
		{
			return detail::piece_square_eval<color, piece>(index);
		}
		template <color color>
		inline constexpr_if_not_tuning eval_t piece_square_eval_eg(const piece piece, const size_t index)
		{
			return detail::piece_square_eval<color>(piece, index + 64);
		}
		template <color color, piece piece>
		inline constexpr_if_not_tuning eval_t piece_square_eval_eg(const size_t index)
		{
			return detail::piece_square_eval<color, piece>(index + 64);
		}

#if !tuning
		// If a white knight on e3 is worth N points, then
		// a black knight on e6 should be worth -N points.
		static_assert(piece_square_eval_mg<white, knight>(to_index(rank(2), file(4))) ==
		              piece_square_eval_mg<black, knight>(to_index(rank(5), file(4))) * -1);
		// test queens
		static_assert(piece_square_eval_mg<white, queen>(to_index(rank(0), file(7))) ==
		              piece_square_eval_mg<black, queen>(to_index(rank(7), file(7))) * -1);
		// test c pawns
		static_assert(piece_square_eval_eg<white, pawn>(to_index(rank(1), file(2))) ==
		              piece_square_eval_eg<black, pawn>(to_index(rank(6), file(2))) * -1);
#endif
	}
}
