#pragma once

#include <array>
#include <memory>
#include <random>

#include <iostream>

#include "board.hpp"
#include "config.hpp"
#include "evaluation.hpp"
#include "position.hpp"
#include "util/strong_alias.hpp"

namespace chess
{
	static_assert(config::tt_size_in_mb / 1024 <= 16); // sanity check, size <= 16 GB

	using tt_key = ::util::strong_alias<uint64_t, struct tt_key_tag>;

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
		tt_key key{};
		depth_t eval_depth{ invalid_depth };
		eval_type eval_type{};
		eval_t eval{};

		bool is_valid() const { return eval_depth != invalid_depth; }
	};

	namespace detail
	{
		struct z_keys_t
		{
			std::array<std::array<tt_key, 12>, 64> piece_square_keys;
			tt_key black_to_move;
			std::array<tt_key, 8> en_passant_keys;
			tt_key w_castle_ks;
			tt_key w_castle_qs;
			tt_key b_castle_ks;
			tt_key b_castle_qs;
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

		constexpr size_t tt_size_in_bytes = (config::tt_size_in_mb * 1024 * 1024);
		constexpr size_t tt_size_in_entries = tt_size_in_bytes / sizeof(entry);

		static_assert(std::popcount(tt_size_in_entries) == 1);
		constexpr uint64_t key_mask = tt_size_in_entries - 1;
	}

	template<typename board_t>
	tt_key make_key(const position& position, const board_t& board)
	{
		using namespace detail;

		tt_key key = 0;

		for (size_t i = 0; i < position._position.size(); ++i)
		{
			const piece piece = position.piece_at(i);
			if (piece.is_occupied())
			{
				key ^= z_keys.piece_square_keys[i][piece.value()];
			}
		}

		if constexpr (board_t::black_to_move())
		{
			key ^= z_keys.black_to_move;
		}

		const file en_passant_file = board.en_passant_file();
		if (en_passant_file != empty)
		{
			key ^= z_keys.en_passant_keys[en_passant_file];
		}

		key ^= (z_keys.w_castle_ks * board.white_can_castle_ks());
		key ^= (z_keys.w_castle_qs * board.white_can_castle_qs());
		key ^= (z_keys.b_castle_ks * board.black_can_castle_ks());
		key ^= (z_keys.b_castle_qs * board.black_can_castle_qs());

		return key;
	}

	class transposition_table
	{
	private:
		using table_t = std::array<entry, detail::tt_size_in_entries>;
		std::unique_ptr<table_t> table;

	public:
		transposition_table() : table{ std::make_unique<table_t>() } {}

		void store(const tt_key key, const depth_t eval_depth, const eval_type eval_type, const eval_t eval)
		{
			entry& entry = get_entry(key);

			entry.key = key;
			entry.eval_depth = eval_depth;
			entry.eval_type = eval_type;
			entry.eval = eval;
		}

		bool probe(eval_t& eval, const tt_key key, const depth_t eval_depth, const eval_t alpha, const eval_t beta) const
		{
			const entry& entry = get_entry(key);

			// no hit
			if (entry.key != key) return false;

			// hit was too shallow
			if (entry.eval_depth < eval_depth) return false;
			// if (entry.key != key || entry.eval_depth != eval_depth) return false; // require exact match for test purposes

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

	private:
		const entry& get_entry(const tt_key key) const
		{
			return (*table)[key & detail::key_mask];
		}
		entry& get_entry(const tt_key key)
		{
			return (*table)[key & detail::key_mask];
		}
	};

}
