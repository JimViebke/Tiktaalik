#pragma once

#include <array>

#include "piece.hpp"
#include "types.hpp"

namespace chess
{
	using position = std::array<piece, 64>;

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
