
#include <iostream>
#include <string>

#include "bitboard.hpp"

namespace chess
{
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
		uint64_t bit = uint64_t(bounds_check(rank, file));
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
	const movemasks knight_movemasks = make_knight_movemasks();
}
