/* 2021 December 16 */

#include "game.hpp"
#include "position.hpp"

namespace chess
{
	// Simple, nonvalidating FEN parser
	root_v load_fen(const std::string& fen, position& _position)
	{
		// reset the position in case this is not the first call to load_fen()
		_position = position{};

		auto position_it = _position._position.begin();
		auto fen_it = fen.cbegin();

		// Edited from Wikipedia:

		// A FEN record contains six fields. The separator between fields is a space. The fields are:

		// 1. Piece placement (from white's perspective). Each rank is described, starting with rank 8 and ending with rank 1; within each rank, the contents of each square are described from file "a" through file "h". Following the Standard Algebraic Notation (SAN), each piece is identified by a single letter taken from the standard English names (pawn = "P", knight = "N", bishop = "B", rook = "R", queen = "Q" and king = "K"). White pieces are designated using upper-case letters ("PNBRQK") while black pieces use lowercase ("pnbrqk"). Empty squares are noted using digits 1 through 8 (the number of empty squares), and "/" separates ranks.

		for (char c = *fen_it++; c != ' '; c = *fen_it++)
		{
			switch (c)
			{
				case 'P': *position_it++ = white_pawn; break;
				case 'N': *position_it++ = white_knight; break;
				case 'B': *position_it++ = white_bishop; break;
				case 'R': *position_it++ = white_rook; break;
				case 'Q': *position_it++ = white_queen; break;
				case 'K': *position_it++ = white_king; break;
				case 'p': *position_it++ = black_pawn; break;
				case 'n': *position_it++ = black_knight; break;
				case 'b': *position_it++ = black_bishop; break;
				case 'r': *position_it++ = black_rook; break;
				case 'q': *position_it++ = black_queen; break;
				case 'k': *position_it++ = black_king; break;

				default:
				{
					if (c >= '1' && c <= '8')
					{
						position_it += (size_t(c) - '0'); // advance N squares
					}
				}
			}
		}

		// 2. Active color. "w" means White moves next, "b" means Black.

		const color_t color_to_move = (*fen_it == 'w') ? white : black;

		fen_it += 2; // step past color and space

		// 3. Castling availability. If neither side can castle, this is "-". Otherwise, this has one or more letters: "K" (White can castle kingside), "Q" (White can castle queenside), "k" (Black can castle kingside), and/or "q" (Black can castle queenside).

		bool white_can_castle_ks = false;
		bool white_can_castle_qs = false;
		bool black_can_castle_ks = false;
		bool black_can_castle_qs = false;
		for (char c = *fen_it++; c != ' '; c = *fen_it++)
		{
			switch (c)
			{
				case 'K': white_can_castle_ks = true; break;
				case 'Q': white_can_castle_qs = true; break;
				case 'k': black_can_castle_ks = true; break;
				case 'q': black_can_castle_qs = true; break;
				default: break;
			}
		}

		// 4. En passant target square in algebraic notation. If there's no en passant target square, this is "-". If a pawn has just made a two-square move, this is the position "behind" the pawn. This is recorded regardless of whether there is a pawn in position to make an en passant capture.

		file en_passant_file{ empty };
		if (*fen_it != '-')
		{
			en_passant_file = *fen_it++ - 'a'; // convert letter file a-h to index 0-7
			++fen_it; // step past the rank
		}
		++fen_it; // step past the space

		// 5. Halfmove clock. This is the number of halfmoves since the last capture or pawn advance. This is used to determine if a draw can be claimed under the fifty-move rule.

		int8_t fifty_move_counter = 0;
		const auto halfmove_clock_end = std::find(fen_it, fen.cend(), ' ');
		std::from_chars(fen_it._Ptr, halfmove_clock_end._Ptr, fifty_move_counter);

		// 6. Fullmove number. The number of the full move. It starts at 1, and is incremented after Black's move.

		// (ignored)

		if (color_to_move == white)
		{
			return node<white>{board{ white_can_castle_ks, white_can_castle_qs, black_can_castle_ks, black_can_castle_qs,
				en_passant_file, fifty_move_counter }};
		}
		else
		{
			return node<black>{board{ white_can_castle_ks, white_can_castle_qs, black_can_castle_ks, black_can_castle_qs,
				en_passant_file, fifty_move_counter }};
		}
	}
}
