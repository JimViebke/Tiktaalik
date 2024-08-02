#include "board.hpp"
#include "transposition_table.hpp"

namespace chess
{
	board::board(const position& loaded_position, const color_t color_to_move, const bool w_castle_ks, const bool w_castle_qs, const bool b_castle_ks, const bool b_castle_qs, const file en_passant_file, const int8_t fifty_move_counter)
	{
		set_white_can_castle_ks(w_castle_ks);
		set_white_can_castle_qs(w_castle_qs);
		set_black_can_castle_ks(b_castle_ks);
		set_black_can_castle_qs(b_castle_qs);

		set_en_passant_file(en_passant_file.value());
		set_fifty_move_counter(fifty_move_counter);

		key = generate_key(*this, loaded_position, color_to_move);

		eval = loaded_position.evaluate_position();

		bitboards = chess::get_bitboards(loaded_position);
	}

	std::array<board, positions_size> boards{};

	tt::key generate_key(const board& board, const position& position, const color_t color_to_move)
	{
		tt::key key = 0;

		for (size_t i = 0; i < position._position.size(); ++i)
		{
			const piece piece = position.piece_at(i);
			if (piece.is_occupied())
			{
				key ^= tt::z_keys.piece_square_keys[piece.value()][i];
			}
		}

		const file en_passant_file = board.get_en_passant_file();
		if (en_passant_file != empty)
		{
			key ^= tt::z_keys.en_passant_keys[en_passant_file];
		}

		if (color_to_move == black)
		{
			key ^= tt::z_keys.black_to_move;
		}

		key ^= (tt::z_keys.w_castle_ks * board.white_can_castle_ks());
		key ^= (tt::z_keys.w_castle_qs * board.white_can_castle_qs());
		key ^= (tt::z_keys.b_castle_ks * board.black_can_castle_ks());
		key ^= (tt::z_keys.b_castle_qs * board.black_can_castle_qs());

		return key;
	}
}
