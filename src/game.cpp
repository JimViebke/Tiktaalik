/* 2021 December 16 */

#include "bitboard.hpp"
#include "game.hpp"
#include "move.hpp"
#include "position.hpp"
#include "search.hpp"

namespace chess
{
	template<color_t color_to_move>
	void set_last_moved_info()
	{
		// During move generation, we optimize by noting that the only way a player can
		// start their turn in check from a knight or pawn is if the opponent moved a
		// knight or pawn, putting us into check. This is determined during move generation
		// by reading the type and position of the last moved piece from `board`.
		// However, this information is absent at the original root. Generate it here so
		// that generate_child_boards() doesn't need to handle the root edge case.

		constexpr color_t opp_color = other_color(color_to_move);
		constexpr piece_t opp_pawn = opp_color | pawn;
		constexpr piece_t opp_knight = opp_color | knight;

		const size_t king_idx = find_king_index<color_to_move>(positions[0]);
		const rank king_rank = king_idx / 8;
		const file king_file = king_idx % 8;

		board& board = boards[0];
		position& position = positions[0];

		const rank attacking_pawn_rank = king_rank + ((color_to_move == white) ? -1 : 1);
		if (bounds_check(attacking_pawn_rank))
		{
			if (bounds_check(king_file - 1) &&
				position.piece_at(attacking_pawn_rank, king_file - 1).is(opp_pawn))
			{
				board.set_moved_piece(opp_pawn);
				board.set_end_index(to_index(attacking_pawn_rank, king_file - 1));
				return;
			}
			else if (bounds_check(king_file + 1) &&
					 position.piece_at(attacking_pawn_rank, king_file + 1).is(opp_pawn))
			{
				board.set_moved_piece(opp_pawn);
				board.set_end_index(to_index(attacking_pawn_rank, king_file + 1));
				return;
			}
		}

		const bitboard attacking_knights = get_bitboard_for<opp_knight>(position) & knight_movemasks[king_idx];

		if (attacking_knights != 0)
		{
			board.set_moved_piece(opp_knight);
			board.set_end_index(get_next_bit(attacking_knights));
			return;
		}

		board.set_moved_piece(empty);
	}

	void set_last_moved_info(const color_t color_to_move)
	{
		if (color_to_move == white)
			set_last_moved_info<white>();
		else
			set_last_moved_info<black>();
	}

	// Simple, nonvalidating FEN parser
	color_t load_fen(const std::string& fen, position& _position)
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

		boards[0] = board{ positions[0], color_to_move,
			white_can_castle_ks, white_can_castle_qs, black_can_castle_ks, black_can_castle_qs,
			en_passant_file, fifty_move_counter };

		set_last_moved_info(color_to_move);

		return color_to_move;
	}

	template<color_t color_to_move>
	void Game::search(const size_t end_idx, const depth_t depth)
	{
		++nodes;

		eval_t alpha = -eval::mate;
		eval_t beta = eval::mate;
		eval_t eval = (color_to_move == white ? -eval::mate : eval::mate);

		eval_t tt_eval = 0; // ignored
		packed_move best_move = 0;
		detail::tt.probe(tt_eval, best_move, boards[0].get_key(), depth, alpha, beta, 0);

		const size_t begin_idx = first_child_index(0);

		detail::swap_tt_move_to_front(best_move, begin_idx, end_idx);

		for (size_t child_idx = begin_idx; child_idx != end_idx; ++child_idx)
		{
			eval_t ab = detail::alpha_beta<other_color(color_to_move)>(child_idx, 1, depth - 1, alpha, beta);

			if (!searching) return;

			if constexpr (color_to_move == white)
			{
				if (ab > eval)
				{
					eval = ab;
					update_pv(0, boards[child_idx]);
					send_info(eval);
					best_move = boards[child_idx].get_packed_move();
				}
				alpha = std::max(alpha, eval);
			}
			else
			{
				if (ab < eval)
				{
					eval = ab;
					update_pv(0, boards[child_idx]);
					send_info(eval);
					best_move = boards[child_idx].get_packed_move();
				}
				beta = std::min(beta, eval);
			}

			detail::swap_best_to_front<color_to_move>(child_idx + 1, end_idx);
		}

		// Store the best move in the TT.
		detail::tt.store(boards[0].get_key(), depth, tt::eval_type::exact, eval, 0, best_move);
	}

	template void Game::search<white>(const size_t, depth_t);
	template void Game::search<black>(const size_t, depth_t);
}
