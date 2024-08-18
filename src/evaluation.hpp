#pragma once

#include <array>
#include <iostream>
#include <limits>

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

	constexpr size_t pse_size = 64 * n_of_piece_types * 2;

#if !tuning
	// clang-format off
	constexpr std::array<int16_t, pse_size> ps_evals = {

	// pawn midgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	    155,  155,  155,  155,  145,  155,  145,  155,
	    155,  155,  155,  145,  145,  155,  150,  155,
	    105,   95,   90,  140,  130,  130,  135,  120,
	     85,   95,  100,  105,  115,  100,  105,  100,
	     75,   75,   65,   60,   60,   90,  100,  120,
	     70,   70,   80,   65,   70,  115,  125,  100,
	      0,    0,    0,    0,    0,    0,    0,    0,
	// pawn endgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	    155,  155,  155,  150,  145,  145,  145,  150,
	    155,  155,  140,  140,  135,  135,  140,  125,
	    135,  125,  105,   85,   95,  100,  110,  110,
	    110,  105,   80,   70,   75,  100,  105,   90,
	    105,  105,   90,   95,   95,   95,   95,   80,
	    115,  120,  100,   85,  110,  105,  110,   95,
	      0,    0,    0,    0,    0,    0,    0,    0,

	// knight midgame:
	    265,  295,  310,  350,  345,  340,  335,  285,
	    305,  295,  350,  365,  375,  365,  335,  325,
	    315,  315,  330,  355,  370,  365,  350,  345,
	    290,  325,  335,  345,  355,  375,  360,  350,
	    305,  295,  320,  335,  355,  345,  345,  320,
	    265,  275,  310,  330,  325,  315,  320,  285,
	    275,  285,  295,  285,  295,  285,  295,  275,
	    275,  270,  265,  275,  265,  275,  285,  285,
	// knight endgame:
	    275,  315,  320,  325,  300,  335,  315,  265,
	    300,  310,  305,  350,  345,  340,  330,  315,
	    315,  325,  330,  335,  340,  355,  325,  330,
	    290,  335,  330,  335,  320,  340,  350,  325,
	    280,  285,  325,  320,  325,  320,  305,  305,
	    275,  275,  300,  295,  315,  290,  290,  275,
	    265,  290,  265,  275,  275,  280,  285,  265,
	    270,  275,  265,  275,  265,  265,  270,  270,

	// bishop midgame:
	    300,  335,  345,  315,  360,  305,  335,  305,
	    320,  305,  360,  330,  360,  380,  345,  325,
	    355,  340,  330,  360,  380,  370,  370,  360,
	    320,  330,  355,  375,  365,  380,  345,  325,
	    325,  355,  340,  355,  360,  345,  340,  305,
	    325,  320,  350,  360,  340,  345,  330,  340,
	    310,  310,  340,  295,  320,  335,  335,  305,
	    295,  345,  310,  290,  280,  315,  300,  300,
	// bishop endgame:
	    320,  350,  350,  305,  330,  310,  320,  320,
	    310,  330,  335,  340,  330,  320,  350,  325,
	    350,  335,  345,  355,  355,  340,  320,  340,
	    315,  345,  325,  350,  345,  355,  355,  325,
	    335,  330,  340,  335,  330,  335,  340,  330,
	    305,  320,  335,  325,  325,  325,  330,  330,
	    295,  300,  315,  325,  320,  310,  320,  315,
	    300,  295,  280,  300,  285,  300,  290,  285,

	// rook midgame:
	    555,  530,  545,  555,  525,  550,  545,  555,
	    540,  555,  545,  545,  545,  550,  555,  540,
	    525,  525,  545,  540,  535,  555,  550,  555,
	    515,  545,  545,  545,  535,  550,  545,  530,
	    485,  485,  505,  515,  510,  515,  535,  505,
	    475,  455,  470,  470,  505,  500,  525,  485,
	    445,  465,  455,  465,  475,  470,  465,  465,
	    455,  465,  475,  490,  475,  485,  505,  465,
	// rook endgame:
	    535,  535,  545,  535,  535,  535,  540,  540,
	    540,  535,  545,  540,  535,  545,  545,  540,
	    535,  540,  555,  535,  540,  540,  540,  540,
	    530,  540,  535,  545,  540,  550,  535,  500,
	    510,  525,  530,  515,  505,  525,  520,  495,
	    500,  505,  510,  500,  510,  515,  500,  505,
	    475,  485,  485,  490,  490,  470,  475,  465,
	    465,  470,  480,  460,  465,  460,  485,  470,

	// queen midgame:
	    990, 1005, 1015, 1005, 1020, 1020, 1025, 1025,
	    965,  995, 1000, 1020, 1010, 1020, 1030, 1020,
	    990,  955,  995, 1030, 1020, 1030, 1030, 1020,
	    960,  960,  980,  985, 1015, 1015, 1030, 1010,
	    955,  965,  970,  980, 1005,  970, 1010, 1020,
	    940,  950,  955,  955,  950,  960,  990,  970,
	    960,  940,  940,  950,  960,  930,  955,  965,
	    970,  940,  940,  950,  930,  925,  930,  970,
	// queen endgame:
	    960, 1000, 1010, 1010, 1020, 1020,  980, 1020,
	    980, 1010, 1000, 1010, 1020, 1020, 1015, 1000,
	    970, 1005, 1020, 1020, 1020, 1030, 1030, 1020,
	    970,  990, 1000, 1010, 1020, 1020, 1025,  990,
	    935,  970,  975,  980,  995,  990, 1010,  985,
	    940,  965,  970,  965,  955,  945,  980,  960,
	    970,  935,  920,  940,  930,  920,  955,  950,
	    935,  925,  930,  920,  920,  920,  920,  935,

	// king midgame:
	      0,   30,   40,   25,   30,   35,   40,  -20,
	      5,   40,   45,   45,   35,   50,   35,   30,
	     20,   35,   45,   35,   20,   30,   25,   30,
	      5,   55,   40,   30,   35,   35,   35,    5,
	     25,   25,   30,   15,   20,   10,   -5,    5,
	     -5,   15,    5,    0,   -5,  -10,   -5,  -25,
	     15,    5,  -15,  -35,  -40,  -25,   20,   20,
	     -5,   25,  -10,  -45,  -10,  -45,   25,    0,
	// king endgame:
	     10,   20,   45,   30,  -10,   15,   55,  -15,
	     20,   25,   35,   45,   25,   35,   45,   25,
	     15,   35,   25,   35,   10,   35,   30,    5,
	     25,   40,   35,   20,   25,   30,   30,   20,
	     10,   20,   20,   20,   15,   20,    5,   -5,
	    -10,    0,    0,   -5,    0,  -10,  -20,  -15,
	     -5,  -10,  -15,  -10,   -5,  -10,  -15,  -35,
	     15,   -5,  -20,  -50,  -45,  -40,  -35,  -25,
	};
	// clang-format on

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
