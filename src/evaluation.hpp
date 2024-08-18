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
		    0, 0, 0, 0, 0, 0, 0, 0,              //
		    55, 55, 55, 55, 45, 55, 45, 55,      //
		    55, 55, 55, 45, 45, 55, 50, 55,      //
		    5, -5, -10, 40, 30, 30, 35, 20,      //
		    -15, -5, 0, 5, 15, 0, 5, 0,          //
		    -25, -25, -35, -40, -40, -10, 0, 20, //
		    -30, -30, -20, -35, -30, 15, 25, 0,  //
		    0, 0, 0, 0, 0, 0, 0, 0,              //
		    // pawn endgame:
		    0, 0, 0, 0, 0, 0, 0, 0,          //
		    55, 55, 55, 50, 45, 45, 45, 50,  //
		    55, 55, 40, 40, 35, 35, 40, 25,  //
		    35, 25, 5, -15, -5, 0, 10, 10,   //
		    10, 5, -20, -30, -25, 0, 5, -10, //
		    5, 5, -10, -5, -5, -5, -5, -20,  //
		    15, 20, 0, -15, 10, 5, 10, -5,   //
		    0, 0, 0, 0, 0, 0, 0, 0,          //
		    // knight midgame:
		    -55, -25, -10, 30, 25, 20, 15, -35,     //
		    -15, -25, 30, 45, 55, 45, 15, 5,        //
		    -5, -5, 10, 35, 50, 45, 30, 25,         //
		    -30, 5, 15, 25, 35, 55, 40, 30,         //
		    -15, -25, 0, 15, 35, 25, 25, 0,         //
		    -55, -45, -10, 10, 5, -5, 0, -35,       //
		    -45, -35, -25, -35, -25, -35, -25, -45, //
		    -45, -50, -55, -45, -55, -45, -35, -35, //
		    // knight endgame:
		    -45, -5, 0, 5, -20, 15, -5, -55,        //
		    -20, -10, -15, 30, 25, 20, 10, -5,      //
		    -5, 5, 10, 15, 20, 35, 5, 10,           //
		    -30, 15, 10, 15, 0, 20, 30, 5,          //
		    -40, -35, 5, 0, 5, 0, -15, -15,         //
		    -45, -45, -20, -25, -5, -30, -30, -45,  //
		    -55, -30, -55, -45, -45, -40, -35, -55, //
		    -50, -45, -55, -45, -55, -55, -50, -50, //
		    // bishop midgame:
		    -25, 10, 20, -10, 35, -20, 10, -20,    //
		    -5, -20, 35, 5, 35, 55, 20, 0,         //
		    30, 15, 5, 35, 55, 45, 45, 35,         //
		    -5, 5, 30, 50, 40, 55, 20, 0,          //
		    0, 30, 15, 30, 35, 20, 15, -20,        //
		    0, -5, 25, 35, 15, 20, 5, 15,          //
		    -15, -15, 15, -30, -5, 10, 10, -20,    //
		    -30, 20, -15, -35, -45, -10, -25, -25, //
		    // bishop endgame:
		    -5, 25, 25, -20, 5, -15, -5, -5,        //
		    -15, 5, 10, 15, 5, -5, 25, 0,           //
		    25, 10, 20, 30, 30, 15, -5, 15,         //
		    -10, 20, 0, 25, 20, 30, 30, 0,          //
		    10, 5, 15, 10, 5, 10, 15, 5,            //
		    -20, -5, 10, 0, 0, 0, 5, 5,             //
		    -30, -25, -10, 0, -5, -15, -5, -10,     //
		    -25, -30, -45, -25, -40, -25, -35, -40, //
		    // rook midgame:
		    55, 30, 45, 55, 25, 50, 45, 55,         //
		    40, 55, 45, 45, 45, 50, 55, 40,         //
		    25, 25, 45, 40, 35, 55, 50, 55,         //
		    15, 45, 45, 45, 35, 50, 45, 30,         //
		    -15, -15, 5, 15, 10, 15, 35, 5,         //
		    -25, -45, -30, -30, 5, 0, 25, -15,      //
		    -55, -35, -45, -35, -25, -30, -35, -35, //
		    -45, -35, -25, -10, -25, -15, 5, -35,   //
		    // rook endgame:
		    35, 35, 45, 35, 35, 35, 40, 40,         //
		    40, 35, 45, 40, 35, 45, 45, 40,         //
		    35, 40, 55, 35, 40, 40, 40, 40,         //
		    30, 40, 35, 45, 40, 50, 35, 0,          //
		    10, 25, 30, 15, 5, 25, 20, -5,          //
		    0, 5, 10, 0, 10, 15, 0, 5,              //
		    -25, -15, -15, -10, -10, -30, -25, -35, //
		    -35, -30, -20, -40, -35, -40, -15, -30, //
		    // queen midgame:
		    15, 30, 40, 30, 45, 45, 50, 50,         //
		    -10, 20, 25, 45, 35, 45, 55, 45,        //
		    15, -20, 20, 55, 45, 55, 55, 45,        //
		    -15, -15, 5, 10, 40, 40, 55, 35,        //
		    -20, -10, -5, 5, 30, -5, 35, 45,        //
		    -35, -25, -20, -20, -25, -15, 15, -5,   //
		    -15, -35, -35, -25, -15, -45, -20, -10, //
		    -5, -35, -35, -25, -45, -50, -45, -5,   //
		    // queen endgame:
		    -15, 25, 35, 35, 45, 45, 5, 45,         //
		    5, 35, 25, 35, 45, 45, 40, 25,          //
		    -5, 30, 45, 45, 45, 55, 55, 45,         //
		    -5, 15, 25, 35, 45, 45, 50, 15,         //
		    -40, -5, 0, 5, 20, 15, 35, 10,          //
		    -35, -10, -5, -10, -20, -30, 5, -15,    //
		    -5, -40, -55, -35, -45, -55, -20, -25,  //
		    -40, -50, -45, -55, -55, -55, -55, -40, //
		    // king midgame:
		    0, 30, 40, 25, 30, 35, 40, -20,    //
		    5, 40, 45, 45, 35, 50, 35, 30,     //
		    20, 35, 45, 35, 20, 30, 25, 30,    //
		    5, 55, 40, 30, 35, 35, 35, 5,      //
		    25, 25, 30, 15, 20, 10, -5, 5,     //
		    -5, 15, 5, 0, -5, -10, -5, -25,    //
		    15, 5, -15, -35, -40, -25, 20, 20, //
		    -5, 25, -10, -45, -10, -45, 25, 0, //
		    // king endgame:
		    10, 20, 45, 30, -10, 15, 55, -15,     //
		    20, 25, 35, 45, 25, 35, 45, 25,       //
		    15, 35, 25, 35, 10, 35, 30, 5,        //
		    25, 40, 35, 20, 25, 30, 30, 20,       //
		    10, 20, 20, 20, 15, 20, 5, -5,        //
		    -10, 0, 0, -5, 0, -10, -20, -15,      //
		    -5, -10, -15, -10, -5, -10, -15, -35, //
		    15, -5, -20, -50, -45, -40, -35, -25, //
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
