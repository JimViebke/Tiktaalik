#pragma once

/*
Jim Viebke
2024-04-20
*/

#include "position.hpp"

namespace chess
{
	template<rank::type rank, file::type file>
	__forceinline bool can_be_captured_by(const position& position, const piece_t attacking_piece)
	{
		if constexpr (bounds_check(rank, file))
		{
			constexpr size_t index = to_index(rank, file);
			return position.piece_at(index).is(attacking_piece);
		}
		else
		{
			return false;
		}
	}

	template<size_t index>
		requires (index < 64)
	bool knight_is_attacking(const position& position, const piece_t attacking_knight)
	{
		constexpr rank::type rank = index / 8;
		constexpr file::type file = index % 8;

		size_t attackers = 0;

		attackers += can_be_captured_by<rank - 2, file + 1>(position, attacking_knight);
		attackers += can_be_captured_by<rank - 1, file + 2>(position, attacking_knight);
		attackers += can_be_captured_by<rank + 1, file + 2>(position, attacking_knight);
		attackers += can_be_captured_by<rank + 2, file + 1>(position, attacking_knight);
		attackers += can_be_captured_by<rank + 2, file - 1>(position, attacking_knight);
		attackers += can_be_captured_by<rank + 1, file - 2>(position, attacking_knight);
		attackers += can_be_captured_by<rank - 1, file - 2>(position, attacking_knight);
		attackers += can_be_captured_by<rank - 2, file - 1>(position, attacking_knight);

		return attackers != 0;
	}

	namespace detail
	{
		template<size_t idx, typename array_t>
		consteval void load_fn_ptrs(array_t& array)
		{
			array[idx] = knight_is_attacking<idx>;

			if constexpr (idx > 0)
				load_fn_ptrs<idx - 1, array_t>(array);
		}
	}

	template<typename T, size_t N>
	consteval void load_fn_ptrs(std::array<T, N>& array)
	{
		detail::load_fn_ptrs<N - 1>(array);
	}

	using knight_attack_fn_ptr = bool(*)(const position&, const piece_t);
	constexpr std::array<knight_attack_fn_ptr, 64> knight_attacks = []() consteval
	{
		std::array<knight_attack_fn_ptr, 64> arr{};
		load_fn_ptrs(arr);
		return arr;
	}();

}
