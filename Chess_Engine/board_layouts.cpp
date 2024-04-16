
#include "board_layouts.hpp"

// Finish implementing FEN parsing and remove all of this.

#define empty piece(piece::empty)

#define white_king piece(piece::white | piece::king)
#define white_queen piece(piece::white | piece::queen)
#define white_rook piece(piece::white | piece::rook)
#define white_bishop piece(piece::white | piece::bishop)
#define white_knight piece(piece::white | piece::knight)
#define white_pawn piece(piece::white | piece::pawn)

#define black_king piece(piece::black | piece::king)
#define black_queen piece(piece::black | piece::queen)
#define black_rook piece(piece::black | piece::rook)
#define black_bishop piece(piece::black | piece::bishop)
#define black_knight piece(piece::black | piece::knight)
#define black_pawn piece(piece::black | piece::pawn)

namespace chess
{
	position layouts::test_board = {
		black_rook, empty, black_bishop, black_queen, black_king, black_bishop, black_knight, black_rook,
		black_pawn, black_pawn, black_pawn, black_pawn, empty, black_pawn, black_pawn, black_pawn,
		empty, empty, black_knight, empty, empty, empty, empty, empty,
		empty, empty, empty, empty, black_pawn, empty, empty, empty,
		empty, empty, empty, empty, white_pawn, empty, empty, empty,
		empty, empty, empty, empty, empty, white_knight, empty, empty,
		white_pawn, white_pawn, white_pawn, white_pawn, empty, white_pawn, white_pawn, white_pawn,
		white_rook, white_knight, white_bishop, white_queen, white_king, white_bishop, empty, white_rook
	};

	position layouts::start_board = {
		black_rook, black_knight, black_bishop, black_queen, black_king, black_bishop, black_knight, black_rook,
		black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn,
		empty, empty, empty, empty, empty, empty, empty, empty,
		empty, empty, empty, empty, empty, empty, empty, empty,
		empty, empty, empty, empty, empty, empty, empty, empty,
		empty, empty, empty, empty, empty, empty, empty, empty,
		white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn,
		white_rook, white_knight, white_bishop, white_queen, white_king, white_bishop, white_knight, white_rook
	};
}

#undef empty

#undef white_king
#undef white_queen
#undef white_rook
#undef white_bishop
#undef white_knight
#undef white_pawn

#undef black_king
#undef black_queen
#undef black_rook
#undef black_bishop
#undef black_knight
#undef black_pawn
