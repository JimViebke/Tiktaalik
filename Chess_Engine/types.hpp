#pragma once

#include <array>
#include <stdint.h>

#include "piece.hpp"
#include "util.hpp"

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
		struct file_tag{};
		struct rank_tag{};
	}

	using file = util::strong_alias<int8_t, detail::file_tag>;
	using rank = util::strong_alias<int8_t, detail::rank_tag>;

	static_assert(std::is_assignable<file, file>::value);
	static_assert(std::is_assignable<rank, rank>::value);
	static_assert(not std::is_assignable<file, rank>::value);
	static_assert(not std::is_assignable<rank, file>::value);

	static constexpr color_t white = piece::white;
	static constexpr color_t black = piece::black;
}
