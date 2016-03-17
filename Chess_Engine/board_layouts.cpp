
#include "board_layouts.h"

#define empty Piece(color::none, piece::empty)

#define white_king Piece(color::white, piece::king)
#define white_rook Piece(color::white, piece::rook)
#define white_bishop Piece(color::white, piece::bishop)

#define black_king Piece(color::black, piece::king)
#define black_rook Piece(color::black, piece::rook)
#define black_bishop Piece(color::black, piece::bishop)

std::vector<Piece> layouts::test_board = {
	 black_rook, empty, empty, empty, black_king, empty, empty, empty,
	 empty, empty, empty, empty, empty, empty, empty, empty,
	 empty, empty, empty, empty, empty, empty, empty, empty,
	 empty, empty, empty, empty, empty, empty, empty, empty,
	 empty, empty, empty, empty, empty, empty, empty, empty,
	 empty, empty, empty, empty, empty, empty, empty, empty,
	 empty, empty, empty, empty, empty, empty, empty, empty,
	 white_rook, empty, empty, empty, white_king, empty, empty, empty
};

#undef empty

#undef white_king
#undef white_rook
#undef white_bishop

#undef black_king
#undef black_rook
#undef black_bishop
