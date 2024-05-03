#pragma once

#include <immintrin.h>
#include <stdint.h>

#include "util/strong_alias.hpp"

namespace chess
{
	enum class result : int8_t
	{
		unknown,
		white_wins_by_checkmate,
		black_wins_by_checkmate,
		draw_by_stalemate
	};

	namespace detail
	{
		struct file_tag {};
		struct rank_tag {};
	}

	using file = ::util::strong_alias<int8_t, detail::file_tag>;
	using rank = ::util::strong_alias<int8_t, detail::rank_tag>;

	static_assert(std::is_assignable<file, file>::value);
	static_assert(std::is_assignable<rank, rank>::value);
	static_assert(not std::is_assignable<file, rank>::value);
	static_assert(not std::is_assignable<rank, file>::value);

	using uint128_t = __m128i;
	using uint256_t = __m256i;

	using eval_t = int16_t;

	template<typename T>
	constexpr bool bounds_check(const T rank_or_file)
	{
		return rank_or_file.value() < 8 && rank_or_file.value() >= 0;
	}
	constexpr bool bounds_check(const rank rank, const file file)
	{
		return bounds_check(rank) && bounds_check(file);
	}

	constexpr size_t to_index(const rank rank, const file file)
	{
		return rank.value() * 8ull + file.value();
	}
}
