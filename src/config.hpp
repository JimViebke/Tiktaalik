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

#if defined __clang__
#	define force_inline_directive [[clang::always_inline]]
#	define noinline_directive [[clang::noinline]]
#else
#	error "Compiler unrecognized. Add configuration for this compiler."
#endif

/*
Some functions are specified as inline, some are specified as force_inline, and some are not specified and left up to the compiler.
However, for certain kinds of profiling and debugging, we have a number of functions that we want to prevent from inlining.
Instead of writing these functions as "inline/force_inline/(unspecified)", write these functions as "inline/force_inline/(unspecified), unless specified otherwise".
In the "otherwise" case, force them to not inline.
*/
#if !_DEBUG && 1
#	define inline_unspecified
#	define inline_toggle        inline
#	define inline_toggle_member inline
#	define force_inline_toggle  force_inline_directive [[maybe_unused]] static
#else
#	define inline_unspecified   noinline_directive static
#	define inline_toggle        noinline_directive [[maybe_unused]] static
#	define inline_toggle_member noinline_directive
#	define force_inline_toggle  noinline_directive [[maybe_unused]] static
#endif
