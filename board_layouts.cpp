
#include "board_layouts.h"

#define empty Piece(color::none, piece::empty)
#define white_king Piece(color::white, piece::king)
#define black_king Piece(color::black, piece::king)

std::vector<std::vector<Piece>> layouts::test_board = {
	{ empty, empty, empty, empty, black_king, empty, empty, empty },
	{ empty, empty, empty, empty, empty, empty, empty, empty },
	{ empty, empty, empty, empty, empty, empty, empty, empty },
	{ empty, empty, empty, empty, empty, empty, empty, empty },
	{ empty, empty, empty, empty, empty, empty, empty, empty },
	{ empty, empty, empty, empty, empty, empty, empty, empty },
	{ empty, empty, empty, empty, empty, empty, empty, empty },
	{ empty, empty, empty, empty, white_king, empty, empty, empty },
};

#undef empty
#undef white_king
#undef black_king
