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

	// Piece-count evals (bishop pair, knight pair, last pawn, etc).
	constexpr size_t pce_start = pse_size;
	constexpr size_t pce_size = 10 * (n_of_piece_types - 1);

	// File piece-count evals (doubled pawns, paired rooks on a file, etc).
	constexpr size_t fpce_start = pce_start + pce_size;
	constexpr size_t fpce_size = 8 * (n_of_piece_types - 1);

	// clang-format off
#if !tuning
	constexpr std::array<int16_t, pse_size + pce_size + fpce_size> weights = {

	// pawn midgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	     85,  140,   65,  120,   95,  140,   40,  -35,
	    -10,    0,   25,   30,   70,   70,   15,  -25,
	    -15,   15,   10,   25,   25,   15,   10,  -30,
	    -30,    0,   -5,   15,   20,    5,    0,  -35,
	    -25,   -5,    0,  -10,    5,    0,   25,  -20,
	    -35,    0,  -20,  -25,  -15,   25,   25,  -30,
	      0,    0,    0,    0,    0,    0,    0,    0,
	// pawn endgame:
	      0,    0,    0,    0,    0,    0,    0,    0,
	    180,  165,  155,  120,  135,  120,  165,  195,
	     90,  100,   80,   60,   45,   40,   80,   80,
	     25,   15,    5,   -5,  -10,   -5,   10,   10,
	      5,    0,  -10,  -20,  -20,  -15,   -5,   -5,
	     -5,    0,  -15,   -5,  -10,  -10,  -10,  -15,
	      5,    0,    5,    5,    5,  -10,   -5,  -15,
	      0,    0,    0,    0,    0,    0,    0,    0,

	// knight midgame:
	   -185,  -90,  -25,  -30,   95,  -90,   -5, -110,
	    -80,  -40,   80,   45,   40,   80,   20,    5,
	    -50,   75,   50,   75,  110,  160,   95,   65,
	     -5,   25,   25,   60,   50,   90,   30,   35,
	     -5,   20,   25,   25,   35,   30,   30,    5,
	    -15,    0,   20,   20,   30,   30,   35,   -5,
	    -20,  -40,   -5,    5,   10,   35,   -5,   -5,
	   -110,  -15,  -50,  -30,   -5,  -15,   -5,   -5,
	// knight endgame:
	    -50,  -45,  -20,  -40,  -55,  -35,  -80, -100,
	    -25,  -15,  -40,  -15,  -25,  -45,  -40,  -70,
	    -30,  -35,   -5,   -5,  -25,  -35,  -40,  -65,
	    -25,  -10,   15,   10,   10,  -10,   -5,  -35,
	    -30,  -20,    5,   15,    5,    5,  -10,  -35,
	    -35,  -15,  -15,    0,   -5,  -20,  -35,  -35,
	    -55,  -30,  -20,  -20,  -15,  -35,  -35,  -60,
	    -30,  -65,  -35,  -25,  -35,  -30,  -70,  -85,

	// bishop midgame:
	    -25,   25,  -95,  -35,  -30,  -20,   20,   15,
	    -15,   25,   -5,   -5,   60,   80,   50,  -25,
	     -5,   55,   60,   60,   55,   85,   55,   15,
	     15,   20,   35,   65,   60,   60,   25,   15,
	     10,   35,   30,   45,   50,   30,   30,   20,
	     20,   35,   30,   35,   30,   45,   35,   25,
	     25,   35,   35,   15,   25,   40,   50,   25,
	    -20,   15,    5,  -10,    5,    5,  -25,  -10,
	// bishop endgame:
	    -20,  -30,   -5,  -10,   -5,  -15,  -20,  -35,
	    -10,  -10,    0,  -15,  -15,  -25,  -15,  -20,
	      0,  -15,  -10,  -10,  -10,  -10,  -10,    0,
	    -10,    5,    5,    5,    5,    0,   -5,   -5,
	    -15,   -5,    5,   10,    0,    5,  -10,  -15,
	    -20,  -10,    5,    5,    5,   -5,  -10,  -20,
	    -25,  -25,  -15,   -5,   -5,  -20,  -25,  -40,
	    -30,  -15,  -35,  -10,  -15,  -25,  -10,  -20,

	// rook midgame:
	     55,   75,   40,   85,   70,   40,   55,   55,
	     40,   45,   80,   80,  110,  105,   40,   70,
	     -5,   30,   35,   45,   20,   75,   80,   35,
	    -25,  -15,    5,   30,   20,   45,    5,  -15,
	    -35,  -25,  -15,    0,    5,   -5,   20,  -20,
	    -45,  -20,  -15,  -15,    0,    0,   -5,  -35,
	    -45,  -10,  -20,  -10,    0,   10,  -10,  -75,
	    -20,  -15,    0,   15,   15,    0,  -35,  -25,
	// rook endgame:
	     20,   10,   25,   15,   20,   15,   10,   10,
	     20,   20,   15,   15,   -5,    0,   15,    5,
	     20,   15,   15,   15,   15,   -5,   -5,    0,
	     20,   20,   25,   10,   15,    5,    5,   15,
	     20,   20,   25,   15,   10,    5,    0,    5,
	     15,   15,   10,   10,    5,    0,    5,    0,
	     10,    5,   15,   15,    5,    5,    5,   20,
	     10,   20,   20,   10,   10,   10,   20,   -5,

	// queen midgame:
	    -45,  -25,   15,   25,  110,  120,   65,   40,
	    -45,  -55,  -20,   -5,  -40,   60,   30,   55,
	    -20,  -25,    0,  -10,   20,   80,   40,   55,
	    -40,  -40,  -30,  -25,  -15,    5,  -15,  -10,
	    -15,  -40,  -15,  -20,  -15,  -10,   -5,  -10,
	    -25,   -5,  -20,  -10,  -15,   -5,    5,   -5,
	    -45,  -15,    5,   -5,    0,   10,  -10,    0,
	     -5,  -25,  -15,    5,  -20,  -35,  -40,  -60,
	// queen endgame:
	     10,   50,   40,   35,    0,  -15,    0,   30,
	      5,   40,   55,   65,   90,   35,   40,   10,
	    -10,   25,   20,   75,   65,   25,   40,   15,
	     25,   45,   45,   65,   85,   60,   90,   60,
	    -10,   55,   35,   65,   55,   45,   55,   35,
	      5,  -20,   35,   20,   30,   30,   30,   35,
	      0,   -5,  -20,    0,    0,  -10,  -25,  -25,
	    -20,  -15,   -5,  -40,   10,  -10,   -5,  -25,

	// king midgame:
	    -50,  205,  165,  105, -105,  -55,   30,   70,
	    200,   60,   35,  130,   55,   70,  -25,  -90,
	     40,   60,   95,   35,   35,  100,  125,   15,
	    -20,  -20,   25,  -20,  -15,  -20,   -5,  -70,
	    -70,    5,  -35, -100,  -90,  -50,  -65,  -80,
	      0,   -5,  -30,  -70,  -60,  -55,  -15,  -45,
	      0,    0,  -25,  -90,  -70,  -40,    0,   10,
	    -30,   30,    0,  -80,  -10,  -50,   15,   15,
	// king endgame:
	    -60,  -60,  -40,  -35,   10,   25,    5,  -20,
	    -40,   15,   15,    0,   15,   30,   30,   30,
	     10,   20,   15,   15,   20,   35,   30,   10,
	      0,   30,   25,   35,   30,   40,   30,   15,
	     -5,    0,   30,   45,   45,   35,   25,    5,
	    -15,    0,   20,   35,   35,   30,   15,    5,
	    -25,   -5,   15,   30,   30,   20,    5,  -15,
	    -45,  -35,  -15,    5,  -20,    0,  -20,  -45,

	// Piece-count evals:
	   125, 110, 100, 100, 100,  90,  85,  85,   0,   0, // 0-8 pawns
	   295, 320, 295,   0,   0,   0,   0,   0,   0,   0, // 0-10 knights
	   305, 340, 305,   0,   0,   0,   0,   0,   0,   0, // 0-10 bishops
	   505, 470, 470,   0,   0,   0,   0,   0,   0,   0, // 0-10 rooks
	   955, 690, 690,   0,   0,   0,   0,   0,   0,   0, // 0-9 queens

	// File piece-count evals:
	     0, -15, -30, -30,   0,   0,   0,   0, // pawns
	     0,   0,   0,   0,   0,   0,   0,   0, // knights
	     0,   0,   0,   0,   0,   0,   0,   0, // bishops
	     0,  25,   0,   0,   0,   0,   0,   0, // rooks
	     0,   0,   0,   0,   0,   0,   0,   0, // queens
	};

#else
	// In tuning builds, the weights array is mutable, unavailable at
	// compile time, and defined in tuning.cpp.
	extern std::array<int16_t, pse_size + pce_size + fpce_size> weights;
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

	template <color color>
	inline constexpr_if_not_tuning eval_t file_piece_count_eval(const piece piece, const size_t file_count)
	{
		const eval_t eval = weights[fpce_start + piece * 8 + file_count];
		return (color == white) ? eval : -eval;
	}
	template <color color, piece piece>
	inline constexpr_if_not_tuning eval_t file_piece_count_eval(const size_t file_count)
	{
		const eval_t eval = weights[fpce_start + piece * 8 + file_count];
		return (color == white) ? eval : -eval;
	}
	template <color color>
	inline constexpr_if_not_tuning eval_t file_piece_count_eval(
	    const piece piece, const file file, const bitboards& bbs)
	{
		return file_piece_count_eval<color>(piece, bbs.file_count<color>(piece, file));
	}
	template <color color, piece piece>
	inline constexpr_if_not_tuning eval_t file_piece_count_eval(const file file, const bitboards& bbs)
	{
		return file_piece_count_eval<color, piece>(bbs.file_count<color, piece>(file));
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
