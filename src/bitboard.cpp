
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
}
