#pragma once

#include <string>

#include "bitboard.hpp"
#include "config.hpp"
#include "defines.hpp"

namespace chess
{
	class move
	{
	private:
		using T = uint16_t;

	public:
		constexpr move() {}
		// The passed move must be a valid move in UCI format.
		// The bitboards of the position at the start of the move must be passed
		// so we can retrieve and store the moving piece type.
		constexpr move(const std::string& str, const bitboards& bitboards) : _move{0}
		{
			_move |= T(str[0] - 'a') << start_file_offset;
			_move |= T(8 - (str[1] - '0')) << start_rank_offset;
			_move |= T(str[2] - 'a') << end_file_offset;
			_move |= T(8 - (str[3] - '0')) << end_rank_offset;

			piece piece{};
			if (str.length() == 4)
			{
				const bitboard from = 1ull << get_start_index();
				piece = (from & bitboards.pawns)     ? pawn
				        : (from & bitboards.knights) ? knight
				        : (from & bitboards.bishops) ? bishop
				        : (from & bitboards.rooks)   ? rook
				        : (from & bitboards.queens)  ? queen
				                                     : king;
			}
			else if (str.length() == 5)
			{
				switch (str[4])
				{
				case 'n': piece = knight; break;
				case 'b': piece = bishop; break;
				case 'r': piece = rook; break;
				case 'q': piece = queen; break;
				default: break;
				}
				_move |= promotion_bits << promotion_offset;
			}
			_move |= T{piece} << moved_piece_offset;
		}

		template <piece piece, chess::piece promoted_piece>
		force_inline_toggle static move make_move(const size_t start_index, const size_t end_index)
		{
			move move;
			move.set_start_index(start_index);
			move.set_end_index(end_index);

			// Record the type of piece that was moved, or in the case of promotion,
			// record the promoted-to type.
			if constexpr (promoted_piece == empty)
			{
				move.set_moved_piece<piece>();
			}
			else
			{
				move.set_moved_piece<promoted_piece>();
				move.set_promotion();
			}

			return move;
		}

		constexpr rank get_start_rank() const { return (_move >> start_rank_offset) & square_mask; }
		constexpr file get_start_file() const { return (_move >> start_file_offset) & square_mask; }
		constexpr size_t get_start_index() const { return (_move >> start_index_offset) & index_mask; }
		constexpr rank get_end_rank() const { return (_move >> end_rank_offset) & square_mask; }
		constexpr file get_end_file() const { return (_move >> end_file_offset) & square_mask; }
		constexpr size_t get_end_index() const { return (_move >> end_index_offset) & index_mask; }
		constexpr piece get_moved_piece() const { return (_move >> moved_piece_offset) & moved_piece_mask; }
		constexpr bool is_promotion() const { return (_move >> promotion_offset) & promotion_bits; }
		constexpr std::string to_string() const
		{
			std::string result;
			result += get_start_file() + 'a';
			result += 8 - get_start_rank() + '0';
			result += get_end_file() + 'a';
			result += 8 - get_end_rank() + '0';

			if (is_promotion())
			{
				switch (get_moved_piece())
				{
				case knight: result += 'n'; break;
				case bishop: result += 'b'; break;
				case rook: result += 'r'; break;
				case queen: result += 'q'; break;
				default: result += '?';
				}
			}

			return result;
		}

		constexpr void set_start_index(const size_t index) { _move |= index << start_index_offset; }
		constexpr void set_end_index(const size_t index) { _move |= index << end_index_offset; }
		template <piece piece>
		constexpr void set_moved_piece()
		{
			_move |= T(piece) << moved_piece_offset;
		}
		constexpr void set_promotion() { _move |= (1 << promotion_offset); }

		constexpr bool operator==(const move&) const = default;
		constexpr operator bool() const { return _move != 0; }

	private:
		// bitfield sizes
		static constexpr size_t square_bits = 3;
		static constexpr size_t moved_piece_bits = 3;
		static constexpr size_t promotion_bits = 1;

		// bitfield positions
		static constexpr size_t start_index_offset = 0;
		static constexpr size_t start_file_offset = start_index_offset;
		static constexpr size_t start_rank_offset = start_file_offset + square_bits;
		static constexpr size_t end_index_offset = start_rank_offset + square_bits;
		static constexpr size_t end_file_offset = end_index_offset;
		static constexpr size_t end_rank_offset = end_file_offset + square_bits;
		static constexpr size_t moved_piece_offset = end_rank_offset + square_bits;
		static constexpr size_t promotion_offset = moved_piece_offset + moved_piece_bits;

		// bitfield masks, not including offsets
		static constexpr uint64_t square_mask = (1ull << square_bits) - 1;
		static constexpr uint64_t index_mask = (square_mask << square_bits) | square_mask;
		static constexpr uint64_t moved_piece_mask = (1ull << moved_piece_bits) - 1;
		static constexpr uint64_t promotion_mask = (1ull << promotion_bits) - 1;

		T _move{};
	};

	constexpr auto& operator<<(auto& lhs, const move move) { return lhs << move.to_string(); }

	namespace detail
	{
		constexpr move move_1 = move{"b6e3", bitboards{}};
		constexpr move move_2 = move{"f2f1r", bitboards{}};
		static_assert(move_1.to_string() == "b6e3");
		static_assert(move_2.to_string() == "f2f1r");
		static_assert(move_1 != move_2);
	}
}
