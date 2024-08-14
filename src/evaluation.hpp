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
		constexpr std::array<int16_t, pse_size> ps_evals = {
		    // pawn midgame:
		    0, 0, 0, 0, 0, 0, 0, 0,                 //
		    364, 150, 177, 416, 217, 110, -89, 417, //
		    -25, 135, 105, 96, 205, 135, 75, 140,   //
		    15, 5, -20, 25, 90, 65, 95, 55,         //
		    -40, -15, -15, 10, -5, 25, 45, 0,       //
		    -25, -15, -70, -85, -25, 25, 35, 20,    //
		    -35, -25, -55, -75, -95, 15, 35, -10,   //
		    0, 0, 0, 0, 0, 0, 0, 0,                 //
		    // pawn endgame:
		    0, 0, 0, 0, 0, 0, 0, 0,             //
		    25, 125, 101, 17, 61, 50, 162, -38, //
		    75, 20, 85, 25, -10, 55, 45, 10,    //
		    35, 35, 5, 20, -10, -20, 0, 5,      //
		    40, 40, -5, -15, -20, 5, 0, 0,      //
		    30, 15, -5, 45, -10, -25, -15, -10, //
		    40, 40, 50, 15, 35, 5, 15, 5,       //
		    0, 0, 0, 0, 0, 0, 0, 0,             //
		    // knight midgame:
		    -419, 85, -180, -10, 15, 258, 153, -128,    //
		    -170, -205, -95, 145, 206, 225, -147, 109,  //
		    -126, 330, 76, 35, 200, 245, 165, 235,      //
		    -20, -15, 170, 25, 45, 161, -4, 277,        //
		    -65, 181, 5, -40, -10, 50, 145, -33,        //
		    -34, 10, -15, -60, 40, -35, -24, 12,        //
		    -156, -230, -25, -40, -80, 101, -135, -258, //
		    -168, -70, -177, -90, -90, -160, -65, 31,   //
		    // knight endgame:
		    -144, -95, 50, 20, -55, -56, -34, 56,     //
		    60, -24, 36, 24, 49, -30, 0, -1,          //
		    -61, -105, 45, 25, -25, -20, 40, -25,     //
		    -30, -5, -45, 75, 45, -10, 5, -38,        //
		    -25, -48, 0, 45, 5, -25, 5, -43,          //
		    -80, -35, -10, -15, -25, -15, -5, -54,    //
		    20, -45, -40, -35, -25, -120, -40, -11,   //
		    -188, -110, -81, -30, -40, -80, -65, -35, //
		    // bishop midgame:
		    -140, -170, -254, -21, 67, 185, 170, -167, //
		    -98, -85, 75, -100, 225, 192, 175, -49,    //
		    130, 60, 10, 67, 230, -95, 337, 232,       //
		    0, -75, -5, 41, 175, 140, 25, 205,         //
		    -9, 0, 55, 75, 85, 25, -25, -60,           //
		    45, -70, -25, 10, 30, 50, 0, 80,           //
		    115, -60, 150, -5, -50, -70, 5, 2,         //
		    40, -20, -45, -100, -20, -65, 112, 74,     //
		    // bishop endgame:
		    50, 75, 35, 35, -5, 20, -5, 4,         //
		    22, 41, 0, 5, -5, 17, -28, 10,         //
		    0, -10, -1, -14, -50, 65, -27, -28,    //
		    -20, 59, 25, 46, -40, -15, 45, -20,    //
		    0, 0, -1, 5, 5, 20, 36, -5,            //
		    0, 5, 5, -15, 5, -15, 0, -35,          //
		    -85, -30, -45, -20, -5, -20, -35, -65, //
		    -75, -15, -30, 5, -25, -45, -11, -40,  //
		    // rook midgame:
		    -85, -45, 50, 100, 100, 90, 75, 180,       //
		    70, 0, 90, 85, 179, 265, 163, 217,         //
		    45, 60, 100, 131, 75, -36, 165, 29,        //
		    70, 69, -25, 70, 15, 37, 15, -80,          //
		    20, -35, -10, -120, 45, 11, -30, 105,      //
		    -155, -113, -15, -90, -35, -34, 5, -65,    //
		    -60, -177, -125, -95, -110, -75, -80, -75, //
		    -100, -85, -85, -70, -40, -75, -60, -120,  //
		    // rook endgame:
		    85, 95, 35, 40, 60, 45, 75, 10,  //
		    60, 30, 70, 50, 40, 15, -8, 36,  //
		    60, 50, 64, 56, 25, 104, 35, 60, //
		    35, 60, 85, 35, 85, 86, 50, 90,  //
		    40, 70, 85, 110, 25, 55, 4, 15,  //
		    55, 65, 15, 55, 30, 35, 85, 25,  //
		    0, 61, 65, 35, 15, 40, 35, 16,   //
		    5, 5, 20, 20, 15, 0, 20, 0,      //
		    // queen midgame:
		    95, 71, 45, 115, 115, 221, 366, -76,       //
		    -95, 60, 120, 89, 242, 332, -253, 260,     //
		    60, -30, 75, -9, 100, 195, 213, 120,       //
		    -30, 59, 120, -65, -15, -10, 300, 140,     //
		    -15, -115, -50, 15, -180, -80, 20, 24,     //
		    -101, -85, -85, -105, -40, 5, 190, 138,    //
		    -261, -95, -40, -100, -100, -135, 55, -14, //
		    31, -130, -145, -25, -31, -55, -230, -199, //
		    // queen endgame:
		    -80, 10, 30, 0, 55, -18, -125, 141,    //
		    56, 25, 15, 85, -36, -63, 302, 1,      //
		    -75, 35, 25, 5, 40, 35, -9, 106,       //
		    75, 35, -30, 60, 160, 100, -100, 30,   //
		    -80, 110, 55, 40, 120, 140, 85, 125,   //
		    20, 50, -14, 25, -15, -65, -220, -296, //
		    165, -35, 10, 50, 15, -40, -85, -15,   //
		    0, -40, -45, -75, -327, -70, 49, -15,  //
		    // king midgame:
		    -85, 62, 204, 15, 449, -349, -40, 67,   //
		    -210, 312, 0, 249, 115, 176, -76, -26,  //
		    205, 35, -80, 105, 11, 104, 245, -111,  //
		    -70, 40, -32, -276, -61, 25, 95, 115,   //
		    183, 55, 55, 5, 10, 30, 172, -110,      //
		    -165, -15, -15, -25, -100, 0, -70, -50, //
		    -50, 25, -55, -20, -50, -70, -20, 35,   //
		    -51, 45, 0, -25, -25, -55, 5, -30,      //
		    // king endgame:
		    -135, 7, -80, 64, -102, 88, 55, -70,   //
		    40, -58, 15, -34, -25, -41, 49, -30,   //
		    5, 10, 0, -20, -50, -66, -36, -15,     //
		    20, 15, -3, 49, 10, 1, -45, 5,         //
		    -47, 0, -5, 0, 0, 10, -34, 20,         //
		    25, 20, 0, -5, 5, -15, 0, -10,         //
		    25, -45, 0, -25, -25, -10, -15, -25,   //
		    104, -35, -10, -30, -35, -30, -25, -5, //
		};

#else
		// The ps_evals array will be mutable, unavailable at compile time,
		// and provided by tuning.cpp.
		extern std::array<int16_t, pse_size> ps_evals;
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
