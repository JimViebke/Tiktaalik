
#include "board_layouts.h"

#define empty Piece(color::none, piece::empty)

#define white_king Piece(color::white, piece::king)
#define white_queen Piece(color::white, piece::queen)
#define white_rook Piece(color::white, piece::rook)
#define white_bishop Piece(color::white, piece::bishop)
#define white_knight Piece(color::white, piece::knight)
#define white_pawn Piece(color::white, piece::pawn)

#define black_king Piece(color::black, piece::king)
#define black_queen Piece(color::black, piece::queen)
#define black_rook Piece(color::black, piece::rook)
#define black_bishop Piece(color::black, piece::bishop)
#define black_knight Piece(color::black, piece::knight)
#define black_pawn Piece(color::black, piece::pawn)

std::vector<Piece> layouts::test_board = {
	empty, empty, empty, empty, empty, empty, empty, empty,
	white_pawn, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, empty, empty, empty,
	empty, empty, empty, empty, empty, black_knight, empty, white_king
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
