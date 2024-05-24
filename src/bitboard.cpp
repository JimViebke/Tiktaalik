
#include <iostream>
#include <string>

#include "bitboard.hpp"

namespace chess
{
	std::array<bitboard, 2> qb_and_qr_bitboards;
	bitboard qbr_attack_masks;
	bitboard captured_piece;

	void print_bitboard(const bitboard bitboard)
	{
		std::string str;
		str.reserve(64ull + 8 + 2);
		for (size_t i = 0; i < 64; ++i)
		{
			str += ((bitboard & (1ull << i)) ? '1' : '.');

			if (i % 8 == 7) str += '\n';
		}
		str += '\n';

		std::cout << str;
	}

	static void set_bit_if_in_bounds(bitboard& mask, const rank rank, const file file)
	{
		const uint64_t bit = uint64_t(bounds_check(rank, file));
		const uint64_t shift = to_index(rank, file) & (64ull - 1); // make sure the shift is always valid
		mask |= (bit << shift);
	}

	static movemasks make_knight_movemasks()
	{
		movemasks movemasks{};

		for (size_t idx = 0; idx < 64; ++idx)
		{
			bitboard mask = 0;
			rank rank = idx / 8;
			file file = idx % 8;

			set_bit_if_in_bounds(mask, rank - 2, file + 1);
			set_bit_if_in_bounds(mask, rank - 1, file + 2);
			set_bit_if_in_bounds(mask, rank + 1, file + 2);
			set_bit_if_in_bounds(mask, rank + 2, file + 1);
			set_bit_if_in_bounds(mask, rank + 2, file - 1);
			set_bit_if_in_bounds(mask, rank + 1, file - 2);
			set_bit_if_in_bounds(mask, rank - 1, file - 2);
			set_bit_if_in_bounds(mask, rank - 2, file - 1);

			movemasks[idx] = mask;
		}

		return movemasks;
	}

	static movemasks make_bishop_movemasks()
	{
		const bitboard nw_se = 0b10000000'01000000'00100000'00010000'00001000'00000100'00000010'00000001;
		const bitboard ne_sw = 0b00000001'00000010'00000100'00001000'00010000'00100000'01000000'10000000;

		movemasks movemasks{};

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				bitboard movemask;

				if (rank > file)
					movemask = nw_se << (8 * (rank - file));
				else
					movemask = nw_se >> (8 * (file - rank));

				if (rank > (7 - file))
					movemask ^= ne_sw << (8 * (rank - (7 - file)));
				else
					movemask ^= ne_sw >> (8 * ((7 - file) - rank));

				movemasks[to_index(rank, file)] = movemask;
			}
		}

		return movemasks;
	}

	static movemasks make_rook_movemasks()
	{
		const bitboard rank_mask = 0xFF;
		const bitboard file_mask = 0x0101010101010101;

		movemasks movemasks{};

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				movemasks[to_index(rank, file)] = (rank_mask << (rank * 8)) ^ (file_mask << file);
			}
		}

		return movemasks;
	}

	const movemasks knight_movemasks = make_knight_movemasks();
	const movemasks bishop_movemasks = make_bishop_movemasks();
	const movemasks rook_movemasks = make_rook_movemasks();
}
