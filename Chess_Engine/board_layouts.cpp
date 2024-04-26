
#include "board_layouts.hpp"
#include "piece.hpp"

// Finish implementing FEN parsing and remove all of this.

namespace chess::layouts
{
	const auto _ = piece(piece::empty);

	const auto K = piece(piece::white | piece::king);
	const auto Q = piece(piece::white | piece::queen);
	const auto R = piece(piece::white | piece::rook);
	const auto B = piece(piece::white | piece::bishop);
	const auto N = piece(piece::white | piece::knight);
	const auto P = piece(piece::white | piece::pawn);

	const auto k = piece(piece::black | piece::king);
	const auto q = piece(piece::black | piece::queen);
	const auto r = piece(piece::black | piece::rook);
	const auto b = piece(piece::black | piece::bishop);
	const auto n = piece(piece::black | piece::knight);
	const auto p = piece(piece::black | piece::pawn);

	position start_board = {
		r, n, b, q, k, b, n, r,
		p, p, p, p, p, p, p, p,
		_, _, _, _, _, _, _, _,
		_, _, _, _, _, _, _, _,
		_, _, _, _, _, _, _, _,
		_, _, _, _, _, _, _, _,
		P, P, P, P, P, P, P, P,
		R, N, B, Q, K, B, N, R
	};

	position bad_capture_for_white = {
		r, n, b, q, k, b, _, _,
		p, p, p, p, p, p, p, p,
		_, _, _, _, _, _, _, _,
		_, _, _, r, _, n, _, _,
		_, _, _, _, P, _, _, _,
		_, _, P, _, _, _, Q, _,
		P, P, P, N, _, P, P, P,
		R, _, B, _, K, B, N, R
	};

	position late_generation = {
		r, n, b, q, k, b, _, _,
		p, p, p, p, p, p, p, p,
		_, _, _, _, _, _, _, _,
		_, _, _, r, P, n, _, _,
		_, _, _, _, _, _, _, _,
		_, _, P, _, _, _, Q, _,
		P, P, P, N, _, P, P, P,
		R, _, B, _, K, B, N, R
	};

	position kiwipete = {
		r, _, _, _, k, _, _, r,
		p, _, p, p, q, p, b, _,
		b, n, _, _, p, n, p, _,
		_, _, _, P, N, _, _, _,
		_, p, _, _, P, _, _, _,
		_, _, N, _, _, Q, _, p,
		P, P, P, B, B, P, P, P,
		R, _, _, _, K, _, _, R,
	};

	position grau_v_colle_white_mate_in_3 = {
		_, k, _, _, _, _, _, r,
		p, P, _, _, _, p, p, p,
		_, _, _, p, _, _, b, _,
		_, B, N, _, n, _, _, _,
		_, Q, _, _, P, _, _, _,
		P, _, B, _, _, _, _, _,
		K, P, _, _, _, P, _, P,
		_, _, _, _, _, _, _, q
	};

	position white_to_gain_material_in_3 = {
		_, _, r, r, _, q, _, _,
		_, _, p, _, _, _, k, _,
		p, p, Q, _, _, b, p, _,
		_, _, _, _, _, _, _, p,
		P, _, B, p, _, _, _, _,
		_, _, _, P, _, R, _, P,
		_, _, _, B, _, _, P, _,
		_, _, _, _, _, _, _, K
	};
}
