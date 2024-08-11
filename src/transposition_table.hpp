#pragma once

#include <array>
#include <memory>
#include <random>

#include "config.hpp"
#include "defines.hpp"
#include "evaluation.hpp"

namespace chess
{
	static_assert(config::tt_size_in_mb / 1024 <= 16); // sanity check, size <= 16 GB

	using tt_key = ::util::strong_alias<uint64_t, struct tt_key_tag>;

	enum class tt_eval_type : uint8_t
	{
		alpha,
		beta,
		exact
	};

	class tt_entry
	{
	private:
		static constexpr depth_t invalid_depth = std::numeric_limits<depth_t>::min();

	public:
		tt_key key{};
		depth_t eval_depth{invalid_depth};
		tt_eval_type eval_type{};
		eval_t eval{};
		packed_move best_move{};

		bool is_valid() const { return eval_depth != invalid_depth; }
	};

	namespace detail
	{
		constexpr size_t tt_size_in_bytes = (config::tt_size_in_mb * 1024 * 1024);
		constexpr size_t tt_size_in_entries = tt_size_in_bytes / sizeof(tt_entry);

		static_assert(std::popcount(tt_size_in_entries) == 1);
		constexpr uint64_t key_mask = tt_size_in_entries - 1;

		struct tt_keys_t
		{
			std::array<std::array<tt_key, 64>, 12> piece_square_keys;
			std::array<tt_key, 8> en_passant_keys;
			tt_key black_to_move;
			tt_key w_castle_ks;
			tt_key w_castle_qs;
			tt_key b_castle_ks;
			tt_key b_castle_qs;
		};

		static const tt_keys_t tt_keys = []()
		{
			std::mt19937_64 rng{0xdeadbeefdeadbeef}; // seed our RNG with a constant so our keys are deterministic
			tt_keys_t keys{};

			for (auto& piece_square_keys : keys.piece_square_keys)
				for (auto& square_key : piece_square_keys)
					square_key = rng();

			for (auto& ep_key : keys.en_passant_keys)
				ep_key = rng();

			keys.black_to_move = rng();

			keys.w_castle_ks = rng();
			keys.w_castle_qs = rng();
			keys.b_castle_ks = rng();
			keys.b_castle_qs = rng();

			return keys;
		}();
	}

	template <color color, piece piece>
	inline tt_key piece_square_key(const size_t idx)
	{
		return detail::tt_keys.piece_square_keys[(piece << 1) | color][idx];
	}
	template <color color>
	inline tt_key piece_square_key(const piece piece, const size_t idx)
	{
		return detail::tt_keys.piece_square_keys[(piece << 1) | color][idx];
	}
	inline tt_key en_passant_key(const file file) { return detail::tt_keys.en_passant_keys[file]; }
	inline tt_key black_to_move_key() { return detail::tt_keys.black_to_move; }
	inline tt_key w_castle_ks_key() { return detail::tt_keys.w_castle_ks; }
	inline tt_key w_castle_qs_key() { return detail::tt_keys.w_castle_qs; }
	inline tt_key b_castle_ks_key() { return detail::tt_keys.b_castle_ks; }
	inline tt_key b_castle_qs_key() { return detail::tt_keys.b_castle_qs; }

	class transposition_table
	{
	private:
		std::vector<tt_entry> table;

	public:
		size_t occupied_entries = 0;
		size_t insertions = 0;
		size_t updates = 0;

		size_t hit = 0;
		size_t miss = 0;

		transposition_table() : table{detail::tt_size_in_entries, tt_entry{}} {}

		template <bool terminal = false>
		inline_toggle_member void store(const tt_key key, const depth_t eval_depth, const tt_eval_type eval_type,
		    eval_t eval, const size_t ply, const packed_move best_move)
		{
			tt_entry& entry = get_entry(key);

			if (!entry.is_valid()) ++occupied_entries;

			if (key != entry.key)
				++insertions;
			else
				++updates;

			// if (key != entry.key || eval_type == eval_type::exact)
			{
				if constexpr (!terminal)
				{
					if (eval >= eval::mate_threshold)
						eval += ply;
					else if (eval <= -eval::mate_threshold)
						eval -= ply;
				}

				entry.key = key;
				entry.eval_depth = eval_depth;
				entry.eval_type = eval_type;
				entry.eval = eval;
				entry.best_move = best_move;
			}
		}

		inline_toggle_member void store(
		    const tt_key key, const depth_t eval_depth, const tt_eval_type eval_type, const eval_t eval)
		{
			store<true>(key, eval_depth, eval_type, eval, 0, 0);
		}

		inline_toggle_member bool probe(eval_t& eval, packed_move& best_move, const tt_key key,
		    const depth_t eval_depth, const eval_t alpha, const eval_t beta, const size_t ply)
		{
			const tt_entry& entry = get_entry(key);

			if (entry.key != key)
			{
				++miss;
				return false; // no hit
			}

			best_move = entry.best_move; // either a move, or 0

			if constexpr (config::tt_require_exact_depth_match)
			{
				if (entry.eval_depth != eval_depth)
				{
					++miss;
					return false;
				}
			}
			else // nominal case
			{
				if (entry.eval_depth < eval_depth)
				{
					++miss;
					return false; // hit was too shallow
				}
			}

			auto cached_eval = entry.eval;

			if (cached_eval >= eval::mate_threshold)
				cached_eval -= ply;
			else if (cached_eval <= -eval::mate_threshold)
				cached_eval += ply;

			if (entry.eval_type == tt_eval_type::exact)
			{
				eval = cached_eval;
				++hit;
				return true;
			}
			else if (entry.eval_type == tt_eval_type::alpha && cached_eval <= alpha)
			{
				eval = alpha;
				++hit;
				return true;
			}
			else if (entry.eval_type == tt_eval_type::beta && cached_eval >= beta)
			{
				eval = beta;
				++hit;
				return true;
			}

			// found an alpha or beta eval, but it was out of bounds, return no hit
			++miss;
			return false;
		}

	private:
		const tt_entry& get_entry(const tt_key key) const { return table[key & detail::key_mask]; }
		tt_entry& get_entry(const tt_key key) { return table[key & detail::key_mask]; }
	};
}
