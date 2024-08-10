
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

	void bitboards::print() const
	{
		std::cout << "white:\n";
		print_bitboard(white);
		std::cout << "black:\n";
		print_bitboard(black);

		std::cout << "pawns:\n";
		print_bitboard(pawns);
		std::cout << "knights:\n";
		print_bitboard(knights);
		std::cout << "bishops:\n";
		print_bitboard(bishops);
		std::cout << "rooks:\n";
		print_bitboard(rooks);
		std::cout << "queens:\n";
		print_bitboard(queens);
		std::cout << "kings:\n";
		print_bitboard(kings);
	}

	static void set_bit_if_in_bounds(bitboard& mask, const rank rank, const file file)
	{
		const uint64_t bit = uint64_t(bounds_check(rank, file));
		const uint64_t shift = to_index(rank, file) & (64ull - 1); // make sure the shift is always valid
		mask |= (bit << shift);
	}

	static std::array<bitboard, 64> make_knight_attack_masks()
	{
		std::array<bitboard, 64> attack_masks{};

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

			attack_masks[idx] = mask;
		}

		return attack_masks;
	}
	static std::array<bitboard, 64> make_bishop_attack_masks()
	{
		const bitboard nw_se = 0b10000000'01000000'00100000'00010000'00001000'00000100'00000010'00000001;
		const bitboard ne_sw = 0b00000001'00000010'00000100'00001000'00010000'00100000'01000000'10000000;

		std::array<bitboard, 64> attack_masks{};

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				bitboard attack_mask;

				if (rank > file)
					attack_mask = nw_se << (8 * (rank - file));
				else
					attack_mask = nw_se >> (8 * (file - rank));

				if (rank > (7 - file))
					attack_mask ^= ne_sw << (8 * (rank - (7 - file)));
				else
					attack_mask ^= ne_sw >> (8 * ((7 - file) - rank));

				attack_masks[to_index(rank, file)] = attack_mask;
			}
		}

		return attack_masks;
	}
	static std::array<bitboard, 64> make_rook_attack_masks()
	{
		const bitboard rank_mask = 0xFF;
		const bitboard file_mask = 0x0101010101010101;

		std::array<bitboard, 64> attack_masks{};

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				attack_masks[to_index(rank, file)] = (rank_mask << (rank * 8)) ^ (file_mask << file);
			}
		}

		return attack_masks;
	}
	static std::array<bitboard, 64> make_king_attack_masks()
	{
		std::array<bitboard, 64> attack_masks{};

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				bitboard attack_mask{};

				set_bit_if_in_bounds(attack_mask, rank - 1, file - 1);
				set_bit_if_in_bounds(attack_mask, rank - 1, file);
				set_bit_if_in_bounds(attack_mask, rank - 1, file + 1);

				set_bit_if_in_bounds(attack_mask, rank, file - 1);
				set_bit_if_in_bounds(attack_mask, rank, file + 1);

				set_bit_if_in_bounds(attack_mask, rank + 1, file - 1);
				set_bit_if_in_bounds(attack_mask, rank + 1, file);
				set_bit_if_in_bounds(attack_mask, rank + 1, file + 1);

				attack_masks[to_index(rank, file)] = attack_mask;
			}
		}

		return attack_masks;
	}

	const std::array<bitboard, 64> knight_attack_masks = make_knight_attack_masks();
	const std::array<bitboard, 64> bishop_attack_masks = make_bishop_attack_masks();
	const std::array<bitboard, 64> rook_attack_masks = make_rook_attack_masks();
	const std::array<bitboard, 64> king_attack_masks = make_king_attack_masks();

	static std::array<bitboard, 64> make_rook_pext_masks()
	{
		constexpr bitboard not_rank_1_or_8 = ~rank_1 & ~rank_8;
		constexpr bitboard not_file_a_or_h = ~file_a & ~file_h;

		std::array<bitboard, 64> pext_masks;

		for (rank rank = 0; rank < 8; ++rank)
		{
			for (file file = 0; file < 8; ++file)
			{
				const bitboard file_mask = (file_a << file) & not_rank_1_or_8;
				const bitboard rank_mask = (rank_8 << (8 * rank)) & not_file_a_or_h;

				const size_t index = ((8 * rank) + file);

				const bitboard rook_bit = 1ull << index;

				pext_masks[index] = (file_mask | rank_mask) & ~rook_bit;
			}
		}

		return pext_masks;
	}
	const std::array<bitboard, 64> rook_pext_masks = make_rook_pext_masks();

	// Search in each of 4 directions, adding move bits until we find a blocker.
	static bitboard make_rook_move_mask(const rank start_rank, const file start_file, const bitboard blocker_mask)
	{
		bitboard moves = 0;

		// Search N.
		for (rank rank = start_rank - 1; rank >= 0; --rank)
		{
			const bitboard square = 1ull << to_index(rank, start_file);
			moves |= square;
			if ((square & blocker_mask) != 0u) break;
		}
		// Search E.
		for (file file = start_file + 1; file < 8; ++file)
		{
			const bitboard square = 1ull << to_index(start_rank, file);
			moves |= square;
			if ((square & blocker_mask) != 0u) break;
		}
		// Search S.
		for (rank rank = start_rank + 1; rank < 8; ++rank)
		{
			const bitboard square = 1ull << to_index(rank, start_file);
			moves |= square;
			if ((square & blocker_mask) != 0u) break;
		}
		// Search W.
		for (file file = start_file - 1; file >= 0; --file)
		{
			const bitboard square = 1ull << to_index(start_rank, file);
			moves |= square;
			if ((square & blocker_mask) != 0u) break;
		}

		return moves;
	}
	static rook_move_masks_t make_rook_move_masks()
	{
		rook_move_masks_t ptr = std::make_unique<rook_move_masks_t::element_type>();
		auto& move_masks = *ptr;

		for (size_t square = 0; square < 64; ++square)
		{
			// For each of 2^12 (4,096) possible blocker configurations,
			// generate the bitboard of possible moves.

			const bitboard rook_pext_mask = rook_pext_masks[square];
			const rank start_rank = square / 8;
			const file start_file = square % 8;

			for (size_t j = 0; j < 4096; ++j)
			{
				const bitboard blocker_mask = _pdep_u64(j, rook_pext_mask);
				const bitboard rook_move_mask = make_rook_move_mask(start_rank, start_file, blocker_mask);
				move_masks[square][j] = rook_move_mask;
			}
		}

		return ptr;
	}
	const rook_move_masks_t rook_move_masks = make_rook_move_masks();

	static std::array<bitboard, 64> make_bishop_pext_masks()
	{
		constexpr bitboard not_edges = ~rank_1 & ~rank_8 & ~file_a & ~file_h;

		std::array<bitboard, 64> pext_masks;

		for (size_t i = 0; i < 64; ++i)
		{
			pext_masks[i] = bishop_attack_masks[i] & not_edges;
		}

		return pext_masks;
	}
	const std::array<bitboard, 64> bishop_pext_masks = make_bishop_pext_masks();

	// Search in each of 4 directions, adding move bits until we find a blocker.
	static bitboard make_bishop_move_mask(const rank start_rank, const file start_file, const bitboard blocker_mask)
	{
		bitboard moves = 0;

		// Search NE.
		{
			rank rank = start_rank - 1;
			file file = start_file + 1;
			for (; rank >= 0 && file < 8; --rank, ++file)
			{
				const bitboard square = 1ull << to_index(rank, file);
				moves |= square;
				if ((square & blocker_mask) != 0u) break;
			}
		}
		// Search SE.
		{
			rank rank = start_rank + 1;
			file file = start_file + 1;
			for (; rank < 8 && file < 8; ++rank, ++file)
			{
				const bitboard square = 1ull << to_index(rank, file);
				moves |= square;
				if ((square & blocker_mask) != 0u) break;
			}
		}
		// Search SW.
		{
			rank rank = start_rank + 1;
			file file = start_file - 1;
			for (; rank < 8 && file >= 0; ++rank, --file)
			{
				const bitboard square = 1ull << to_index(rank, file);
				moves |= square;
				if ((square & blocker_mask) != 0u) break;
			}
		}
		// Search NW.
		{
			rank rank = start_rank - 1;
			file file = start_file - 1;
			for (; rank >= 0 && file >= 0; --rank, --file)
			{
				const bitboard square = 1ull << to_index(rank, file);
				moves |= square;
				if ((square & blocker_mask) != 0u) break;
			}
		}

		return moves;
	}
	static bishop_move_masks_t make_bishop_move_masks()
	{
		bishop_move_masks_t ptr = std::make_unique<bishop_move_masks_t::element_type>();
		auto& move_masks = *ptr;

		for (size_t square = 0; square < 64; ++square)
		{
			// For each of 2^9 (512) possible blocker configurations,
			// generate the bitboard of possible moves.

			const bitboard bishop_pext_mask = bishop_pext_masks[square];
			const rank start_rank = square / 8;
			const file start_file = square % 8;

			for (size_t j = 0; j < 512; ++j)
			{
				const bitboard blocker_mask = _pdep_u64(j, bishop_pext_mask);
				const bitboard bishop_move_mask = make_bishop_move_mask(start_rank, start_file, blocker_mask);
				move_masks[square][j] = bishop_move_mask;
			}
		}

		return ptr;
	}
	const bishop_move_masks_t bishop_move_masks = make_bishop_move_masks();
}
