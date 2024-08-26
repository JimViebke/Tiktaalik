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
	    330,  320,  315,  280,  215,  235,   70,  160,
	     95,  115,   80,  110,  105,  110,  155,   85,
	     -5,   -5,  -10,   20,   40,   50,   45,   30,
	    -40,  -40,  -30,  -10,    0,   10,   20,  -25,
	    -45,  -45,  -55,  -55,  -45,    5,   15,   -5,
	    -50,  -55,  -60,  -55,  -55,    5,   20,  -35,
	      0,    0,    0,    0,    0,    0,    0,    0,
	// pawn endgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	     30,   45,   30,   10,   35,   10,   75,   40,
	     45,   40,   35,   20,   15,   15,   10,   15,
	     25,   20,    5,  -15,  -10,  -15,    5,   -5,
	     10,   10,  -10,  -30,  -20,  -15,   -5,   -5,
	      0,    5,  -10,   -5,  -10,  -15,  -10,  -20,
	     20,   20,    5,   10,   15,   -5,    0,    0,
	      0,    0,    0,    0,    0,    0,    0,    0,

	// knight midgame:
	   -225, -195,  -55,   20,  -30,  225,   80, -300,
	    105,  -40,   85,  195,  150,  100,   70,  160,
	      5,    5,   75,  160,  225,  225,  200,  120,
	     30,   20,   70,  115,   75,  175,   60,  115,
	    -45,   45,   35,   35,   60,   45,   90,   25,
	    -30,  -10,    0,   45,   50,    0,    5,   -5,
	    -60,  -45,   -5,    0,   -5,   10,  -50,   15,
	   -140,  -35,  -65,  -75,  -45,  -35,  -15,  125,
	// knight endgame:
	    -50,   45,   50,   20,   60,  -40,   20,   -5,
	    -35,   35,   15,    5,   25,   35,   25,  -10,
	     15,   35,   40,   25,    0,   20,  -15,    5,
	     15,   20,   35,   30,   40,   25,   40,  -10,
	      5,   10,   25,   35,   35,   45,   10,    0,
	    -40,   -5,   10,   15,   10,   10,   15,  -20,
	    -55,  -15,  -35,  -15,  -15,  -10,   -5,  -55,
	    -50,  -45,  -45,  -15,  -30,  -35,  -75, -125,

	// bishop midgame:
	   -125,  -40,    0,  -60,   15,  -40,  145, -155,
	     45,  -15,   95,   30,   75,  110,   15,   25,
	     35,   70,   90,  100,  165,  180,  195,  110,
	      0,    5,   95,   90,   80,  190,   -5,   30,
	      5,  -25,   35,   80,   95,    0,   30,    0,
	    -25,   15,    5,   20,    5,   10,   -5,   40,
	     15,  -20,   35,  -25,    0,  -20,   15,    0,
	    -10,   15,  -20,  -25,  -50,  -20,  -50,  -75,
	// bishop endgame:
	     55,   45,   25,   40,   35,   25,   -5,   35,
	      0,   40,   15,   35,   25,   10,   25,   10,
	     20,   20,   25,   15,   15,   20,   -5,    5,
	     20,   30,   20,   30,   30,    5,   45,   25,
	     15,   30,   25,   10,   20,   40,   20,   20,
	     15,   20,   20,   20,   25,   15,    5,   10,
	     -5,    0,    0,   15,    0,   15,  -10,  -10,
	     -5,  -10,  -35,   -5,   -5,  -15,    0,   10,

	// rook midgame:
	    140,  160,  120,  170,  160,  200,  180,  225,
	    105,  120,  180,  135,  130,  160,  195,  265,
	     70,  115,   85,   90,  150,  160,  145,  220,
	    -20,   10,  -10,   85,   45,   80,  135,   65,
	    -95,  -90,  -90,   -5,  -25,  -40,   35,   40,
	   -115,  -90,  -80,  -70,  -10,  -30,   50,   -5,
	   -135, -100,  -70,  -60,  -70,  -35,  -20, -110,
	    -90,  -60,  -55,  -50,  -50,  -50,    0,  -55,
	// rook endgame:
	     25,   35,   45,   30,   35,   25,   40,   10,
	     55,   45,   35,   55,   50,   45,   30,   10,
	     70,   60,   75,   70,   60,   45,   50,   15,
	     75,   85,   90,   70,   65,   60,   45,   50,
	     85,   85,   90,   60,   65,   80,   50,   30,
	     60,   65,   55,   50,   35,   35,   20,   25,
	     55,   40,   35,   30,   40,   20,   15,   30,
	     25,   30,   30,   30,   25,   10,    5,   15,

	// queen midgame:
	    125,  180,  135,  135,  225,  310,  305,  330,
	     85,   20,   50,  130,  185,  190,   95,  325,
	     10,   30,   60,  110,  175,  325,  295,  215,
	    -55,  -30,  -10,   30,   95,  120,  205,   80,
	    -40,  -70,  -15,   -5,  -15,   20,   60,   15,
	    -60,  -55,  -45,  -45,  -30,  -40,  -20,   50,
	    -30,  -60,  -35,  -40,  -20,  -70,  -60,  -25,
	    -10,  -75,  -25,  -25,  -40,  -65,  -40,   25,
	// queen endgame:
	    -20,  -30,   45,   45,   15,  -45,  -55,  -85,
	     20,   70,  100,   70,   45,   60,   95,  -85,
	     45,   85,   90,   65,   45,  -35,  -30,  -10,
	     80,   95,  105,  100,   80,   85,   10,   80,
	     20,  110,   65,   90,  100,   55,   45,  105,
	     70,   65,   65,   60,   30,   40,   50,  -15,
	     -5,   35,    0,   15,  -15,    5,  -10,  -15,
	    -30,    0,  -25,  -40,  -55,  -50,  -25,  -45,

	// king midgame:
	    240,   45,  230,  -20,  -65,   35,  -15,  235,
	    145,   65,   65,   85,   65,  190,   10,  110,
	    -65,   35,   95,   25,  130,  115,   80,  135,
	   -125,   90,   30,   90,   40,   60,   75,   -5,
	      5,  -20,  -10,   10,   15,    5,  -45,  -85,
	    -85,    0,   -5,  -20,  -40,  -45,  -25,  -50,
	     80,    0,  -40,  -70,  -65,  -45,   10,   10,
	    -30,   65,  -10, -100,  -10,  -70,   25,   25,
	// king endgame:
	    -35,   45,  -10,   55,   45,   40,   55,  -85,
	     -5,   45,   40,   35,   20,   15,   60,   25,
	     55,   55,   35,   45,    0,   20,   45,   -5,
	     45,   40,   45,   25,   30,   25,   25,   25,
	     15,   35,   25,   25,   25,   25,   30,   20,
	     15,   10,   15,   10,   15,   10,    5,    0,
	    -10,    0,    0,    0,    0,    0,  -10,  -10,
	     30,  -20,  -20,  -10,  -45,  -20,  -25,  -35,

	// Piece count evals:
	   140, 100, 110, 110, 100,  85,  65,  25,   0,   0, // 0-8 pawns
	   280, 290, 325,   0,   0,   0,   0,   0,   0,   0, // 0-10 knights
	   295, 335, 190,   0,   0,   0,   0,   0,   0,   0, // 0-10 bishops
	   500, 435, -20,   0,   0,   0,   0,   0,   0,   0, // 0-10 rooks
	   930, 520,   0,   0,   0,   0,   0,   0,   0,   0, // 0-9 queens
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
