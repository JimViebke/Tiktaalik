
#include "board_layouts.h"

#define empty Piece(piece::empty)

#define white_king Piece(piece::white_king)
#define white_queen Piece(white_queen)
#define white_rook Piece(white_rook)
#define white_bishop Piece(white_bishop)
#define white_knight Piece(white_knight)
#define white_pawn Piece(white_pawn)

#define black_king Piece(black_king)
#define black_queen Piece(black_queen)
#define black_rook Piece(black_rook)
#define black_bishop Piece(black_bishop)
#define black_knight Piece(black_knight)
#define black_pawn Piece(black_pawn)

std::vector<Piece> layouts::test_board = {
	black_rook, black_knight, black_bishop, black_queen, black_king, black_bishop, black_knight, black_rook,
	black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn,
	white_rook, white_knight, white_bishop, white_queen, white_king, white_bishop, white_knight, white_rook
};

std::vector<Piece> layouts::start_board = {
	black_rook, black_knight, black_bishop, black_queen, black_king, black_bishop, black_knight, black_rook,
	black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn, black_pawn,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn, white_pawn,
	white_rook, white_knight, white_bishop, white_queen, white_king, white_bishop, white_knight, white_rook
};

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
