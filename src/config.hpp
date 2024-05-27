#pragma once

#include <stdint.h>

namespace chess
{
	namespace config
	{
		static constexpr size_t engine_target_depth = 8;

		static constexpr bool verify_cached_sliding_piece_bitboards = false;
		static constexpr bool verify_incremental_static_eval = false;
		static constexpr bool verify_incremental_zobrist_key = false;
	}

	namespace tt::config
	{
		static constexpr size_t size_in_mb = 1024 * 1;
		static constexpr bool require_exact_depth_match = false;
		static constexpr bool use_tt_move_ordering = true;
	}
}

#if 1
#define inline_unspecified
#define inline_toggle inline
#define inline_toggle_member inline
#define force_inline_toggle [[clang::always_inline]] [[maybe_unused]] static
#else
#define inline_unspecified [[clang::noinline]] static
#define inline_toggle [[clang::noinline]] [[maybe_unused]] static
#define inline_toggle_member [[clang::noinline]]
#define force_inline_toggle [[clang::noinline]] [[maybe_unused]] static
#endif
