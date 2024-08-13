#pragma once

#include <immintrin.h>
#include <stdint.h>
#include <type_traits>

#include "util/strong_alias.hpp"

namespace chess
{
	using file = ::util::strong_alias<int8_t, struct file_tag>;
	using rank = ::util::strong_alias<int8_t, struct rank_tag>;

	static_assert(std::is_assignable<file, file>());
	static_assert(std::is_assignable<rank, rank>());
	static_assert(!std::is_assignable<file, rank>());
	static_assert(!std::is_assignable<rank, file>());

	using eval_t = ::util::strong_alias<int16_t, struct eval_tag>;
	using depth_t = ::util::strong_alias<int16_t, struct depth_tag>;
	using packed_move = ::util::strong_alias<uint16_t, struct packed_move_tag>;

	using piece = uint8_t;
	using color = uint8_t;

	constexpr color white = 0;
	constexpr color black = 1;

	constexpr inline color other_color(const color color) { return (color == white) ? black : white; }

	constexpr piece pawn = 0;
	constexpr piece knight = 1;
	constexpr piece bishop = 2;
	constexpr piece rook = 3;
	constexpr piece queen = 4;
	constexpr piece king = 5;

	constexpr size_t n_of_piece_types = 6;

	constexpr piece empty = king + 1;

	constexpr file no_ep_file = 8;

	using uint128_t = __m128i;
	using uint256_t = __m256i;

	template <typename T>
	constexpr bool bounds_check(const T rank_or_file)
	{
		return rank_or_file < 8 && rank_or_file >= 0;
	}
	constexpr bool bounds_check(const rank rank, const file file) { return bounds_check(rank) && bounds_check(file); }

	constexpr size_t to_index(const rank rank, const file file) { return rank * 8ull + file; }
}
