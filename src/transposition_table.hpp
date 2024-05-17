#pragma once

#include <array>
#include <memory>
#include <random>

#include "config.hpp"
#include "evaluation.hpp"
#include "types.hpp"

namespace chess::tt
{
	static_assert(config::size_in_mb / 1024 <= 16); // sanity check, size <= 16 GB

	enum class eval_type : uint8_t
	{
		alpha,
		beta,
		exact
	};

	class entry
	{
	private:
		static constexpr depth_t invalid_depth = std::numeric_limits<depth_t>::min();

	public:
		key key{};
		depth_t eval_depth{ invalid_depth };
		eval_type eval_type{};
		eval_t eval{};

		bool is_valid() const { return eval_depth != invalid_depth; }
	};

	namespace detail
	{
		struct z_keys_t
		{
			std::array<std::array<key, 12>, 64> piece_square_keys;
			key black_to_move;
			std::array<key, 8> en_passant_keys;
			key w_castle_ks;
			key w_castle_qs;
			key b_castle_ks;
			key b_castle_qs;
		};

		static const z_keys_t z_keys = []()
		{
			std::mt19937_64 rng{ 0xdeadbeefdeadbeef }; // seed our RNG with a constant so our keys are deterministic
			z_keys_t keys{};

			for (auto& piece_square_keys : keys.piece_square_keys)
				for (auto& piece_square_key : piece_square_keys)
					piece_square_key = rng();

			keys.black_to_move = rng();

			for (auto& ep_key : keys.en_passant_keys)
				ep_key = rng();

			keys.w_castle_ks = rng();
			keys.w_castle_qs = rng();
			keys.b_castle_ks = rng();
			keys.b_castle_qs = rng();

			return keys;
		}();

		constexpr size_t tt_size_in_bytes = (config::size_in_mb * 1024 * 1024);
		constexpr size_t tt_size_in_entries = tt_size_in_bytes / sizeof(entry);

		static_assert(std::popcount(tt_size_in_entries) == 1);
		constexpr uint64_t key_mask = tt_size_in_entries - 1;
	}

	class transposition_table
	{
	private:
		std::vector<entry> table;

	public:
		size_t occupied_entries = 0;
		size_t insertions = 0;
		size_t updates = 0;

		transposition_table() : table{ detail::tt_size_in_entries, entry{} } {}

		void store(const key key, const depth_t eval_depth, const eval_type eval_type, const eval_t eval)
		{
			entry& entry = get_entry(key);

			if (!entry.is_valid())
				++occupied_entries;

			if (key != entry.key)
				++insertions;
			else
				++updates;

			entry.key = key;
			entry.eval_depth = eval_depth;
			entry.eval_type = eval_type;
			entry.eval = eval;
		}

		bool probe(eval_t& eval, const key key, const depth_t eval_depth, const eval_t alpha, const eval_t beta) const
		{
			const entry& entry = get_entry(key);

			if (entry.key != key) return false; // no hit

			if constexpr (config::require_exact_depth_match)
			{
				if (entry.eval_depth != eval_depth) return false;
			}
			else // nominal case
			{
				if (entry.eval_depth < eval_depth) return false; // hit was too shallow
			}

			if (entry.eval_type == eval_type::exact)
			{
				eval = entry.eval;
				return true;
			}
			else if (entry.eval_type == eval_type::alpha &&
					 entry.eval <= alpha)
			{
				eval = alpha;
				return true;
			}
			else if (entry.eval_type == eval_type::beta &&
					 entry.eval >= beta)
			{
				eval = beta;
				return true;
			}

			// found an alpha or beta eval, but it was out of bounds, return no hit
			return false;
		}

		bool simple_exact_probe(eval_t& eval, const key key) const
		{
			const entry& entry = get_entry(key);
			eval = entry.eval;
			return entry.key == key && entry.eval_type == eval_type::exact;
		}

	private:
		const entry& get_entry(const key key) const
		{
			return table[key & detail::key_mask];
		}
		entry& get_entry(const key key)
		{
			return table[key & detail::key_mask];
		}
	};
}
