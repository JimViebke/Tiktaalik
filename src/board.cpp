#include <charconv>

#include "board.hpp"
#include "transposition_table.hpp"

namespace chess
{
	std::array<board, boards_size> boards{};

	color board::load_fen(const std::string& fen)
	{
		board_state = uint32_t{}; // Reset the fields we'll be modifying.
		bitboards = chess::bitboards{};
		move = chess::move{};

		auto fen_it = fen.cbegin();
		size_t idx = 0;

		// Edited from Wikipedia:

		// A FEN record contains six fields. The separator between fields is a space. The fields are:

		// 1. Piece placement (from white's perspective). Each rank is described, starting with rank 8 and ending with
		// rank 1; within each rank, the contents of each square are described from file "a" through file "h". Following
		// the Standard Algebraic Notation (SAN), each piece is identified by a single letter taken from the standard
		// English names (pawn = "P", knight = "N", bishop = "B", rook = "R", queen = "Q" and king = "K"). White pieces
		// are designated using upper-case letters ("PNBRQK") while black pieces use lowercase ("pnbrqk"). Empty squares
		// are noted using digits 1 through 8 (the number of empty squares), and "/" separates ranks.

		for (char c = *fen_it++; c != ' '; c = *fen_it++)
		{
			switch (c)
			{
			case 'P':
				bitboards.white |= (1ull << idx);
				bitboards.pawns |= (1ull << idx++);
				break;
			case 'N':
				bitboards.white |= (1ull << idx);
				bitboards.knights |= (1ull << idx++);
				break;
			case 'B':
				bitboards.white |= (1ull << idx);
				bitboards.bishops |= (1ull << idx++);
				break;
			case 'R':
				bitboards.white |= (1ull << idx);
				bitboards.rooks |= (1ull << idx++);
				break;
			case 'Q':
				bitboards.white |= (1ull << idx);
				bitboards.queens |= (1ull << idx++);
				break;
			case 'K':
				bitboards.white |= (1ull << idx);
				bitboards.kings |= (1ull << idx++);
				break;
			case 'p':
				bitboards.black |= (1ull << idx);
				bitboards.pawns |= (1ull << idx++);
				break;
			case 'n':
				bitboards.black |= (1ull << idx);
				bitboards.knights |= (1ull << idx++);
				break;
			case 'b':
				bitboards.black |= (1ull << idx);
				bitboards.bishops |= (1ull << idx++);
				break;
			case 'r':
				bitboards.black |= (1ull << idx);
				bitboards.rooks |= (1ull << idx++);
				break;
			case 'q':
				bitboards.black |= (1ull << idx);
				bitboards.queens |= (1ull << idx++);
				break;
			case 'k':
				bitboards.black |= (1ull << idx);
				bitboards.kings |= (1ull << idx++);
				break;
			default:
				if (c >= '1' && c <= '8')
				{
					idx += (size_t(c) - '0'); // Advance 1-8 squares.
				}
			}
		}

		// 2. Active color. "w" means White moves next, "b" means Black.

		const color color_to_move = (*fen_it == 'w') ? white : black;

		fen_it += 2; // step past color and space

		// 3. Castling availability. If neither side can castle, this is "-". Otherwise, this has one or more letters:
		// "K" (White can castle kingside), "Q" (White can castle queenside), "k" (Black can castle kingside), and/or
		// "q" (Black can castle queenside).

		for (char c = *fen_it++; c != ' '; c = *fen_it++)
		{
			switch (c)
			{
			case 'K': set_white_can_castle_ks(1); break;
			case 'Q': set_white_can_castle_qs(1); break;
			case 'k': set_black_can_castle_ks(1); break;
			case 'q': set_black_can_castle_qs(1); break;
			default: break;
			}
		}

		// 4. En passant target square in algebraic notation. If there's no en passant target square, this is "-". If a
		// pawn has just made a two-square move, this is the position "behind" the pawn. This is recorded regardless of
		// whether there is a pawn in position to make an en passant capture.

		if (*fen_it != '-')
		{
			const file ep_file = *fen_it++ - 'a'; // convert letter file a-h to index 0-7
			move.set_end_index(ep_file);
			set_ep_capture();
		}
		fen_it += 2;

		// 5. Halfmove clock. This is the number of halfmoves since the last capture or pawn advance. This is used to
		// determine if a draw can be claimed under the fifty-move rule.

		int8_t fifty_move_counter = 0;
		const auto halfmove_clock_end = std::find(fen_it, fen.cend(), ' ');
		std::from_chars(fen_it._Ptr, halfmove_clock_end._Ptr, fifty_move_counter);
		set_fifty_move_counter(fifty_move_counter);

		// 6. Fullmove number. The number of the full move. It starts at 1, and is incremented after Black's move.

		// (ignored)

		// Finish setting up the board:
		// - Add previous move info for check detection.
		set_previous_move_info(color_to_move);
		// - Generate the Zobrist hash key, game phase, and static evaluation from scratch.
		generate_key_phase_eval(color_to_move);

		return color_to_move;
	}

	void board::generate_eval() { generate_key_phase_eval<false, false, true>(0); }

	template <bool verify_key, bool verify_phase, bool verify_eval>
	void board::verify_key_phase_eval(const color color_to_move)
	{
		tt_key expected_key{};
		uint16_t expected_phase{};
		eval_t expected_mg_eval{};
		eval_t expected_eg_eval{};
		eval_t expected_persistent_eval{};
		eval_t expected_eval{};

		if constexpr (verify_key)
		{
			expected_key = key;
		}

		if constexpr (verify_phase)
		{
			expected_phase = phase;
		}

		if constexpr (verify_eval)
		{
			expected_mg_eval = mg_eval;
			expected_eg_eval = eg_eval;
			expected_persistent_eval = persistent_eval;
			expected_eval = eval;
		}

		generate_key_phase_eval<verify_key, verify_phase, verify_eval>(color_to_move);

		if (verify_key && key != expected_key)
		{
			std::cout << "Incremental and generated keys mismatch\n";
		}

		if (verify_phase && phase != expected_phase)
		{
			std::cout << "Incremental and generated phases mismatch\n";
		}

		if constexpr (verify_eval)
		{
			if (mg_eval != expected_mg_eval)
			{
				std::cout << "Incremental and generated mg_evals mismatch\n";
			}

			if (eg_eval != expected_eg_eval)
			{
				std::cout << "Incremental and generated eg_evals mismatch\n";
			}

			if (persistent_eval != expected_persistent_eval)
			{
				std::cout << "Incremental and generated persistent evals mismatch\n";
			}

			if (eval != expected_eval)
			{
				std::cout << "Incremental and generated evals mismatch\n";
			}
		}
	}

	template void board::verify_key_phase_eval(const color);        // For searching.
	template void board::verify_key_phase_eval<false>(const color); // For quiescing.
#if tuning
	template void board::verify_key_phase_eval<false, false, true>(const color);
#endif

	void board::set_previous_move_info(const color color_to_move)
	{
		// During move generation, we optimize by noting that the only way a player can
		// start their turn in check from a knight or pawn is if the opponent moved a
		// knight or pawn, putting us into check. This is determined during move generation
		// by reading the type and position of the last moved piece from `board`.
		// However, this information is absent at the original root. Generate it here so
		// that generate_child_boards() doesn't need to handle the root edge case.

		const bitboard our_pieces = (color_to_move == white) ? bitboards.get<white>() : bitboards.get<black>();
		const bitboard opp_pieces = (color_to_move == white) ? bitboards.get<black>() : bitboards.get<white>();

		const size_t king_idx = get_next_bit_index(our_pieces & bitboards.kings);
		const rank king_rank = king_idx / 8;
		const file king_file = king_idx % 8;

		const rank attacking_pawn_rank = king_rank + ((color_to_move == white) ? -1 : 1);
		if (bounds_check(attacking_pawn_rank))
		{
			const bitboard opp_pawns = opp_pieces & bitboards.pawns;

			const size_t pawn_lo_idx = to_index(attacking_pawn_rank, king_file - 1);
			const size_t pawn_hi_idx = to_index(attacking_pawn_rank, king_file + 1);

			if (bounds_check(king_file - 1) && (opp_pawns & (1ull << pawn_lo_idx)))
			{
				move.set_moved_piece<pawn>();
				move.set_end_index(pawn_lo_idx);
				return;
			}
			else if (bounds_check(king_file + 1) && (opp_pawns & (1ull << pawn_hi_idx)))
			{
				move.set_moved_piece<pawn>();
				move.set_end_index(pawn_hi_idx);
				return;
			}
		}

		const bitboard attacking_knights = opp_pieces & bitboards.knights & knight_attack_masks[king_idx];
		if (attacking_knights != 0u)
		{
			move.set_moved_piece<knight>();
			move.set_end_index(get_next_bit_index(attacking_knights));
			return;
		}

		move.set_moved_piece<empty>();
	}
}
