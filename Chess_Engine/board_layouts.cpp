
#include "board_layouts.hpp"

// Finish implementing FEN parsing and remove all of this.

#define _ piece(piece::empty)

#define K piece(piece::white | piece::king)
#define Q piece(piece::white | piece::queen)
#define R piece(piece::white | piece::rook)
#define B piece(piece::white | piece::bishop)
#define N piece(piece::white | piece::knight)
#define P piece(piece::white | piece::pawn)

#define k piece(piece::black | piece::king)
#define q piece(piece::black | piece::queen)
#define r piece(piece::black | piece::rook)
#define b piece(piece::black | piece::bishop)
#define n piece(piece::black | piece::knight)
#define p piece(piece::black | piece::pawn)

namespace chess
{

	position layouts::start_board = {
		r, n, b, q, k, b, n, r,
		p, p, p, p, p, p, p, p,
		_, _, _, _, _, _, _, _,
		_, _, _, _, _, _, _, _,
		_, _, _, _, _, _, _, _,
		_, _, _, _, _, _, _, _,
		P, P, P, P, P, P, P, P,
		R, N, B, Q, K, B, N, R
	};

	position layouts::test_board = {
		r, n, b, q, k, b, _, _,
		p, p, p, p, p, p, p, p,
		_, _, _, _, _, _, _, _,
		_, _, _, r, _, n, _, _,
		_, _, _, _, P, _, _, _,
		_, _, P, _, _, _, Q, _,
		P, P, P, N, _, P, P, P,
		R, _, B, _, K, B, N, R
	};

	position layouts::black_to_move_ply_4 = {
		r, _, b, q, k, b, _, _,
		p, p, p, p, _, p, p, Q,
		_, _, n, _, p, _, _, _,
		_, _, _, _, r, _, _, _,
		_, _, _, _, N, _, _, _,
		_, _, P, _, _, P, _, _,
		P, P, P, _, _, _, P, P,
		R, _, B, _, K, B, N, R
	};
}

#undef _

#undef K
#undef Q
#undef R
#undef B
#undef N
#undef P

#undef k
#undef q
#undef r
#undef b
#undef n
#undef p
