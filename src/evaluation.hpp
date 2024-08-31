#pragma once

#include <array>
#include <iostream>
#include <limits>

#include "bitboard.hpp"
#include "config.hpp"
#include "defines.hpp"

namespace chess::eval
{
	constexpr eval_t mate = std::numeric_limits<eval_t>::max() - max_ply;
	constexpr eval_t mate_threshold = mate - max_ply;

	inline bool found_mate(const eval_t eval) { return eval >= eval::mate_threshold || eval <= -eval::mate_threshold; }

	constexpr std::array<phase_t, n_of_piece_types - 1> phase_weights = {0, 3, 3, 5, 10};

	constexpr size_t total_phase = 16 * phase_weights[pawn] +  //
	                               4 * phase_weights[knight] + //
	                               4 * phase_weights[bishop] + //
	                               4 * phase_weights[rook] +   //
	                               2 * phase_weights[queen];   //
	static_assert(std::popcount(total_phase) == 1);

	// Piece-square evals.
	constexpr size_t pse_start = 0;
	constexpr size_t pse_size = 64 * n_of_piece_types * 2;

	// Piece count evals (bishop pair, knight pair, last pawn, etc).
	constexpr size_t pce_start = pse_size;
	constexpr size_t pce_size = 10 * (n_of_piece_types - 1);

	// clang-format off
#if !tuning
	constexpr std::array<int16_t, pse_size + pce_size> weights = {

	// pawn midgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	    165,  275,  125,  235,  205,  270,   60,  -85,
	    -45,  -10,   40,   50,  140,  130,   30,  -70,
	    -55,   20,    5,   45,   45,   15,   20,  -80,
	    -80,  -15,  -20,   20,   30,    0,    0,  -80,
	    -75,  -15,  -20,  -30,   -5,  -10,   55,  -55,
	    -95,  -10,  -55,  -60,  -45,   35,   60,  -75,
	      0,    0,    0,    0,    0,    0,    0,    0,
	// pawn endgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	    380,  350,  325,  255,  275,  250,  345,  415,
	    200,  210,  170,  130,   90,   80,  160,  170,
	     60,   30,   10,  -15,  -30,  -15,   15,   25,
	     20,    0,  -25,  -40,  -40,  -40,  -15,  -20,
	     -5,   -5,  -35,  -15,  -20,  -30,  -30,  -35,
	     20,    0,    5,    5,    5,  -20,  -20,  -30,
	      0,    0,    0,    0,    0,    0,    0,    0,

	// knight midgame:
	   -390, -195,  -55,  -60,  210, -195,   15, -240,
	   -150,  -80,  185,  100,   90,  190,   40,   10,
	    -80,  165,  105,  175,  245,  330,  205,  150,
	     -5,   60,   60,  145,  110,  190,   70,   80,
	     -5,   40,   55,   50,   85,   70,   75,   10,
	    -25,    5,   50,   45,   65,   65,   80,   -5,
	    -35,  -95,   -5,   20,   20,   65,  -10,   -5,
	   -235,  -20, -105,  -60,   -5,  -30,  -10,  -30,
	// knight endgame:
	    -90,  -85,  -35,  -80, -110,  -65, -165, -200,
	    -55,  -20,  -80,  -25,  -45,  -90,  -70, -130,
	    -65,  -70,    5,  -10,  -50,  -60,  -75, -130,
	    -50,  -10,   35,   25,   25,   -5,   -5,  -60,
	    -55,  -30,   15,   40,   15,   20,  -10,  -60,
	    -70,  -25,  -25,   10,    0,  -30,  -65,  -70,
	   -110,  -55,  -40,  -35,  -25,  -65,  -65, -125,
	    -65, -140,  -65,  -45,  -70,  -65, -135, -160,

	// bishop midgame:
	    -50,   40, -200,  -75,  -60,  -70,   35,   10,
	    -25,   70,    0,  -10,  125,  160,   90,  -60,
	      5,  110,  135,  125,  125,  175,  115,   40,
	     25,   50,   80,  150,  125,  130,   50,   35,
	     25,   70,   65,   90,  110,   60,   60,   45,
	     35,   75,   65,   70,   60,   95,   70,   55,
	     55,   70,   70,   30,   55,   80,  105,   50,
	    -35,   30,    5,  -15,   10,   10,  -55,  -20,
	// bishop endgame:
	    -35,  -50,   -5,  -15,  -10,  -20,  -35,  -60,
	    -20,  -20,    5,  -25,  -25,  -40,  -20,  -30,
	     -5,  -25,  -15,  -15,  -20,  -10,  -10,    0,
	    -10,   10,   15,   10,   15,    5,    0,   -5,
	    -25,   -5,   15,   30,    5,   15,  -15,  -30,
	    -35,  -20,   10,   10,   20,   -5,  -20,  -45,
	    -45,  -50,  -25,   -5,   -5,  -30,  -45,  -75,
	    -55,  -25,  -65,  -20,  -30,  -45,  -15,  -40,

	// rook midgame:
	    115,  160,   95,  195,  205,   60,   85,  100,
	    110,  115,  200,  210,  270,  235,   95,  135,
	      5,   70,  100,  100,   50,  175,  185,   75,
	    -40,    0,   30,   85,   70,  100,   10,  -25,
	    -70,  -45,    0,   15,   40,   -5,   45,  -30,
	    -85,  -30,  -15,  -25,   20,   10,   10,  -55,
	    -85,  -15,  -25,   -5,   15,   35,    5, -135,
	    -25,  -10,   20,   40,   45,   15,  -50,  -25,
	// rook endgame:
	     45,   30,   60,   35,   30,   50,   40,   35,
	     35,   40,   25,   25,  -20,    0,   35,   20,
	     45,   40,   30,   35,   35,   -5,   -5,   10,
	     45,   35,   55,   25,   30,   20,   25,   40,
	     45,   45,   45,   40,   15,   20,    5,   10,
	     30,   30,   20,   30,   10,    5,   10,    0,
	     30,   20,   35,   35,   10,   10,    5,   40,
	     15,   35,   35,   30,   20,   15,   40,  -20,

	// queen midgame:
	    -95,  -55,   50,   35,  255,  230,  135,   80,
	    -55,  -90,  -15,   10,  -60,  160,   70,  135,
	    -20,  -35,   20,    0,   50,  175,  110,  135,
	    -70,  -65,  -40,  -35,  -15,   25,  -15,  -10,
	    -20,  -70,  -25,  -30,  -15,  -10,    0,  -10,
	    -40,   10,  -30,   -5,  -15,    0,   25,    5,
	    -75,  -15,   30,    0,   20,   35,    5,   15,
	      5,  -35,  -20,   25,  -40,  -65,  -60, -110,
	// queen endgame:
	     30,  110,   75,   85,  -10,  -15,   10,   75,
	    -10,   70,  100,  125,  180,   50,   85,    0,
	    -25,   40,   40,  145,  140,   55,   65,   25,
	     50,   90,   80,  130,  170,  125,  170,  120,
	    -20,  105,   75,  140,  110,  100,  115,   75,
	     10,  -50,   65,   35,   50,   65,   55,   50,
	    -10,  -25,  -55,   -5,  -10,  -25,  -70,  -60,
	    -55,  -40,  -20, -100,   20,  -35,  -20,  -60,

	// king midgame:
	    -70,  455,  355,  185, -270, -170,   60,  125,
	    430,  125,   65,  250,   70,   70, -105, -265,
	    130,  150,  185,   55,   90,  230,  255,  -45,
	    -50,  -65,    5,  -80,  -95,  -70,  -40, -150,
	   -180,  -10, -100, -200, -205, -160, -150, -160,
	     -5,  -40,  -95, -175, -155, -140,  -50,  -90,
	     15,   10,  -50, -180, -140,  -75,   15,   35,
	    -45,   80,   15, -160,   -5,  -95,   60,   55,
	// king endgame:
	   -160, -160, -115,  -90,    5,   45,  -15,  -60,
	   -115,    0,    0,  -20,   10,   55,   50,   55,
	    -20,    5,   10,   10,   10,   45,   40,   15,
	    -30,   40,   40,   55,   55,   65,   50,   10,
	    -35,  -20,   45,   70,   75,   60,   30,  -20,
	    -60,  -15,   25,   55,   60,   45,   10,  -20,
	    -85,  -40,    5,   35,   35,   15,  -25,  -60,
	   -130, -105,  -65,  -20,  -80,  -30,  -80, -135,

	// Piece count evals:
	   270, 230, 215, 210, 205, 195, 175, 180,   0,   0, // 0-8 pawns
	   615, 665, 615,   0,   0,   0,   0,   0,   0,   0, // 0-10 knights
	   635, 710,1015,   0,   0,   0,   0,   0,   0,   0, // 0-10 bishops
	  1060, 985, 775,   0,   0,   0,   0,   0,   0,   0, // 0-10 rooks
	  2005,1450,1960,   0,   0,   0,   0,   0,   0,   0, // 0-9 queens
	};

#else
	// In tuning builds, the weights array is mutable, unavailable at
	// compile time, and defined in tuning.cpp.
	extern std::array<int16_t, pse_size + pce_size> weights;
#endif
	// clang-format on

	namespace detail
	{
		template <color color>
		inline constexpr_if_not_tuning eval_t piece_square_eval(const piece piece, size_t index)
		{
			if constexpr_if_not_tuning (color == black) index ^= 0b111000;
			const eval_t eval = weights[pse_start + piece * 128 + index];
			return (color == white) ? eval : -eval;
		}
		template <color color, piece piece>
		inline constexpr_if_not_tuning eval_t piece_square_eval(size_t index)
		{
			if constexpr_if_not_tuning (color == black) index ^= 0b111000;
			const eval_t eval = weights[pse_start + piece * 128 + index];
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

	template <color color>
	inline constexpr_if_not_tuning eval_t piece_count_eval(const piece piece, const size_t count)
	{
		const eval_t eval = weights[pce_start + piece * 10 + count];
		return (color == white) ? eval : -eval;
	}
	template <color color, piece piece>
	inline constexpr_if_not_tuning eval_t piece_count_eval(const size_t count)
	{
		const eval_t eval = weights[pce_start + piece * 10 + count];
		return (color == white) ? eval : -eval;
	}
	template <color color>
	inline constexpr_if_not_tuning eval_t piece_count_eval(const piece piece, const bitboards& bbs)
	{
		return piece_count_eval<color>(piece, bbs.count<color>(piece));
	}
	template <color color, piece piece>
	inline constexpr_if_not_tuning eval_t piece_count_eval(const bitboards& bbs)
	{
		return piece_count_eval<color, piece>(bbs.count<color, piece>());
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
