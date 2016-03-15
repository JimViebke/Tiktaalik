#pragma once

#include <vector>

#include "piece.h"

namespace layouts
{
	extern std::vector<std::vector<Piece>> test_board;

	/* std::vector<std::vector<Piece>> default_board = {
		{ Piece(black, rook), Piece(black, knight), Piece::black_bishop, Piece::black_queen, Piece::black_king, Piece::black_bishop, Piece::black_knight, Piece::black_rook },
		{ Piece::black_pawn, Piece::black_pawn, Piece::black_pawn, Piece::black_pawn, Piece::black_pawn, Piece::black_pawn, Piece::black_pawn, Piece::black_pawn },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::white_pawn, Piece::white_pawn, Piece::white_pawn, Piece::white_pawn, Piece::white_pawn, Piece::white_pawn, Piece::white_pawn, Piece::white_pawn },
		{ Piece::white_rook, Piece::white_knight, Piece::white_bishop, Piece::white_queen, Piece::white_king, Piece::white_bishop, Piece::white_knight, Piece::white_rook }
	};

	std::vector<std::vector<Piece>> empty_board = {
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::black_king, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::empty },
		{ Piece::empty, Piece::empty, Piece::empty, Piece::empty, Piece::white_king, Piece::empty, Piece::empty, Piece::empty }
	}; */
}
