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
		    564, 405, 397, 456, 287, 240, 66, 307,  //
		    140, 135, 135, 191, 170, 175, 260, 125, //
		    -15, -10, 0, 45, 65, 85, 50, 65,        //
		    -45, -45, -25, -20, 15, 20, 55, -5,     //
		    -75, -60, -75, -70, -55, 25, 35, 10,    //
		    -80, -75, -80, -100, -85, -5, 35, -15,  //
		    0, 0, 0, 0, 0, 0, 0, 0,                 //
		    // pawn endgame:
		    0, 0, 0, 0, 0, 0, 0, 0,               //
		    100, 145, 121, 67, 131, 115, 212, 82, //
		    125, 130, 110, 85, 90, 75, 95, 95,    //
		    105, 90, 70, 40, 50, 45, 80, 60,      //
		    90, 80, 55, 35, 40, 50, 60, 45,       //
		    80, 70, 50, 55, 50, 40, 50, 35,       //
		    110, 95, 70, 90, 85, 70, 70, 55,      //
		    0, 0, 0, 0, 0, 0, 0, 0,               //
		    // knight midgame:
		    -194, -365, -125, -200, 75, -187, 33, -258, //
		    65, 30, 95, 245, 251, 290, 38, 204,         //
		    -11, 75, 116, 205, 335, 425, 210, 185,      //
		    40, 5, 50, 130, 105, 146, 101, 132,         //
		    -70, 131, 40, 45, 35, 90, 105, 82,          //
		    -54, -35, 0, 100, 60, 5, 1, -28,            //
		    -131, -115, -40, -25, -20, -9, -5, -43,     //
		    102, -60, -127, -105, -10, -45, -35, -234,  //
		    // knight endgame:
		    61, 220, 180, 195, 175, 169, 111, 221,  //
		    125, 121, 171, 149, 149, 110, 165, 39,  //
		    129, 165, 175, 165, 125, 120, 150, 155, //
		    95, 170, 190, 190, 205, 180, 190, 127,  //
		    145, 122, 165, 175, 190, 170, 160, 127, //
		    25, 135, 145, 135, 160, 130, 145, 91,   //
		    40, 140, 100, 120, 130, 120, 115, 104,  //
		    -53, 20, 79, 115, 70, 40, 25, 0,        //
		    // bishop midgame:
		    -100, -265, -19, -106, 62, -55, 280, -152, //
		    -28, 170, 170, 115, 200, 147, 70, 161,     //
		    70, 75, 75, 177, 275, 325, 307, 152,       //
		    20, 55, 140, 176, 160, 140, 70, 55,        //
		    26, 125, 75, 100, 105, 50, 140, 70,        //
		    0, 45, 65, 60, 50, 60, 15, 100,            //
		    40, 15, 80, 5, 20, 60, 55, -33,            //
		    -55, 25, 5, -55, -85, -15, 12, -61,        //
		    // bishop endgame:
		    220, 260, 200, 225, 175, 180, 140, 189, //
		    182, 171, 185, 150, 160, 167, 162, 140, //
		    165, 200, 199, 171, 160, 155, 138, 177, //
		    190, 194, 170, 181, 190, 185, 205, 160, //
		    160, 160, 189, 195, 185, 185, 151, 125, //
		    155, 170, 160, 185, 180, 160, 185, 135, //
		    125, 135, 145, 155, 165, 130, 125, 140, //
		    140, 115, 95, 145, 160, 125, 84, 85,    //
		    // rook midgame:
		    185, 155, 120, 175, 160, 205, 120, 265,         //
		    75, 50, 125, 245, 194, 190, 308, 332,           //
		    45, 125, 60, 66, 215, 129, 185, 274,            //
		    -100, 34, 5, 15, -5, 27, -5, 165,               //
		    -160, -165, -125, -70, -25, -89, 5, 20,         //
		    -170, -148, -160, -170, -110, -59, -20, -40,    //
		    -230, -182, -175, -120, -140, -160, -130, -105, //
		    -180, -165, -155, -120, -130, -155, -55, -180,  //
		    // rook endgame:
		    335, 350, 380, 345, 345, 340, 360, 315, //
		    375, 390, 370, 315, 355, 345, 302, 291, //
		    375, 370, 384, 396, 350, 369, 350, 315, //
		    395, 370, 395, 400, 405, 391, 415, 330, //
		    385, 400, 390, 385, 360, 390, 359, 300, //
		    360, 365, 395, 380, 355, 335, 340, 290, //
		    345, 331, 345, 330, 335, 345, 340, 296, //
		    305, 335, 345, 325, 325, 325, 295, 330, //
		    // queen midgame:
		    500, 456, 315, 575, 510, 661, 546, 499, //
		    210, 210, 310, 429, 512, 492, 412, 630, //
		    180, 285, 295, 291, 415, 645, 608, 450, //
		    150, 164, 205, 240, 325, 300, 430, 370, //
		    130, 185, 170, 125, 225, 195, 210, 254, //
		    159, 135, 145, 135, 140, 180, 275, 283, //
		    69, 115, 155, 155, 175, 95, 55, 236,    //
		    81, 90, 135, 165, 149, 45, 80, 231,     //
		    // queen endgame:
		    265, 350, 500, 370, 450, 327, 380, 351, //
		    481, 515, 520, 500, 474, 462, 467, 336, //
		    490, 495, 510, 555, 520, 335, 431, 466, //
		    445, 540, 530, 570, 550, 565, 465, 455, //
		    420, 535, 490, 585, 510, 510, 520, 500, //
		    485, 465, 451, 480, 450, 435, 370, 359, //
		    450, 415, 410, 395, 360, 375, 455, 290, //
		    485, 390, 395, 335, 353, 385, 454, 375, //
		    // king midgame:
		    95, 247, 79, 225, 429, -79, 280, 397,      //
		    495, 202, 100, 299, 115, 251, 184, 344,    //
		    55, 285, 180, 170, 256, 259, 455, 224,     //
		    -85, 130, 83, 44, -16, 125, 125, -20,      //
		    -7, 120, 135, -35, -10, -55, -3, -90,      //
		    -120, -80, -45, -120, -55, -100, -20, -60, //
		    95, 15, -60, -120, -30, -60, 30, 40,       //
		    29, 145, 85, -120, 20, -60, 60, 60,        //
		    // king endgame:
		    -20, 42, 15, 14, -57, 53, 20, -95,     //
		    -55, 52, 50, 31, 15, -21, 59, -5,      //
		    35, 25, 55, 35, 5, -1, -16, -35,       //
		    60, 35, 57, 59, 45, 16, 20, 20,        //
		    23, 15, 20, 45, 35, 35, 16, 20,        //
		    40, 35, 15, 20, 15, 20, -5, -10,       //
		    -5, -5, -5, -10, -20, -15, -25, -25,   //
		    29, -55, -65, -45, -70, -35, -50, -70, //
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
