#pragma once

#include <immintrin.h>
#include <stdint.h>
#include <type_traits>

#include "util/strong_alias.hpp"

namespace chess
{
	enum class result : int8_t
	{
		unknown = 0,
		white_wins_by_checkmate,
		black_wins_by_checkmate,
		draw_by_stalemate
	};

	using piece_t = int8_t;
	using color_t = piece_t;

	using file = ::util::strong_alias<int8_t, struct file_tag>;
	using rank = ::util::strong_alias<int8_t, struct rank_tag>;

	static_assert(std::is_assignable<file, file>());
	static_assert(std::is_assignable<rank, rank>());
	static_assert(!std::is_assignable<file, rank>());
	static_assert(!std::is_assignable<rank, file>());

	using uint128_t = __m128i;
	using uint256_t = __m256i;

	using eval_t = ::util::strong_alias<int16_t, struct eval_tag>;
	using depth_t = ::util::strong_alias<int16_t, struct depth_tag>;
	using packed_move = ::util::strong_alias<uint16_t, struct packed_move_tag>;

	template <typename T>
	constexpr bool bounds_check(const T rank_or_file)
	{
		return rank_or_file < 8 && rank_or_file >= 0;
	}
	constexpr bool bounds_check(const rank rank, const file file) { return bounds_check(rank) && bounds_check(file); }


	template <typename T>
	T diff(const T a, const T b)
	{
		return std::abs(a.value() - b.value());
	}
	constexpr size_t to_index(const rank rank, const file file) { return rank * 8ull + file; }
}
