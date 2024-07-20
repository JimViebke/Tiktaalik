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
		packed_move best_move{};

		bool is_valid() const { return eval_depth != invalid_depth; }
	};

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

	namespace detail
	{
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

		size_t hit = 0;
		size_t miss = 0;

		transposition_table() : table{ detail::tt_size_in_entries, entry{} } {}

		template<bool terminal = false>
		inline_toggle_member void store(const key key, const depth_t eval_depth,
										const eval_type eval_type, eval_t eval,
										const size_t ply, const packed_move best_move)
		{
			entry& entry = get_entry(key);

			if (!entry.is_valid())
				++occupied_entries;

			if (key != entry.key)
				++insertions;
			else
				++updates;

			//if (key != entry.key || eval_type == eval_type::exact)
			{
				if constexpr (!terminal)
				{
					if (eval >= eval::mate_threshold) eval += ply;
					else if (eval <= -eval::mate_threshold) eval -= ply;
				}

				entry.key = key;
				entry.eval_depth = eval_depth;
				entry.eval_type = eval_type;
				entry.eval = eval;
				entry.best_move = best_move;
			}
		}

		inline_toggle_member void store(const key key, const depth_t eval_depth,
										const eval_type eval_type, const eval_t eval)
		{
			store<true>(key, eval_depth, eval_type, eval, 0, 0);
		}

		inline_toggle_member bool probe(eval_t& eval, packed_move& best_move,
										const key key, const depth_t eval_depth, const eval_t alpha, const eval_t beta,
										const size_t ply)
		{
			const entry& entry = get_entry(key);

			if (entry.key != key)
			{
				++miss;
				return false; // no hit
			}

			best_move = entry.best_move; // either a move, or 0

			if constexpr (config::require_exact_depth_match)
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

			if (cached_eval >= eval::mate_threshold) cached_eval -= ply;
			else if (cached_eval <= -eval::mate_threshold) cached_eval += ply;

			if (entry.eval_type == eval_type::exact)
			{
				eval = cached_eval;
				++hit;
				return true;
			}
			else if (entry.eval_type == eval_type::alpha &&
					 cached_eval <= alpha)
			{
				eval = alpha;
				++hit;
				return true;
			}
			else if (entry.eval_type == eval_type::beta &&
					 cached_eval >= beta)
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
