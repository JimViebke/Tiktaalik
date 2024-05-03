#pragma once

#include <array>

#include "evaluation.hpp"
#include "piece.hpp"
#include "types.hpp"

namespace chess
{
	class alignas(64) position
	{
	public:
		std::array<piece, 64> _position;

		const piece& piece_at(const rank rank, const file file) const
		{
			return _position[to_index(rank, file)];
		}
		piece& piece_at(const rank rank, const file file)
		{
			return _position[to_index(rank, file)];
		}
		const piece& piece_at(const size_t index) const
		{
			return _position[index];
		}
		piece& piece_at(const size_t index)
		{
			return _position[index];
		}

		bool is_empty(const rank rank, const file file) const
		{
			return piece_at(rank, file).is_empty();
		}
		bool is_empty(const size_t index) const
		{
			return piece_at(index).is_empty();
		}
		bool is_occupied(const rank rank, const file file) const
		{
			return !is_empty(rank, file);
		}
		bool is_occupied(const size_t index) const
		{
			return !is_empty(index);
		}

		eval_t evaluate_position() const
		{
			eval_t eval = 0;

			for (size_t i = 0; i < _position.size(); ++i)
			{
				eval += eval::eval(_position[i]); // evaluate material
				eval += eval::piece_square_eval(_position[i], i); // evaluate position
			}

			return eval;
		}
	};

	static_assert(sizeof(position) == 64 && alignof(position) == 64);
}
