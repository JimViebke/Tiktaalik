#pragma once

#include <stdint.h>

namespace chess::config
{
	constexpr bool verify_key_phase_eval = false;

	constexpr size_t tt_size_in_mb = 1024 * 1;
	constexpr bool tt_require_exact_depth_match = false;
}

#if defined __clang__
	#define force_inline_directive [[clang::always_inline]]
	#define noinline_directive [[clang::noinline]]
#else
	#error "Compiler unrecognized. Add configuration for this compiler."
#endif

/*
Some functions are specified as inline, some are specified as force_inline, and some are not specified and left up to
the compiler. However, for certain kinds of profiling and debugging, we have a number of functions that we want to
prevent from inlining. Instead of writing these functions as "inline/force_inline/(unspecified)", write these functions
as "inline/force_inline/(unspecified), unless specified otherwise". In the "otherwise" case, force them to not inline.
*/
#if !_DEBUG && 1
	#define inline_unspecified
	#define inline_toggle inline
	#define inline_toggle_member inline
	#define force_inline_toggle force_inline_directive
#else
	#define inline_unspecified noinline_directive
	#define inline_toggle [[maybe_unused]] noinline_directive
	#define inline_toggle_member noinline_directive
	#define force_inline_toggle noinline_directive
#endif

/*
Toggle between accessing evals at compile time (when not tuning),
and modifying evals at runtime (when tuning).
*/

#define tuning 0

#if tuning
	#define constexpr_if_not_tuning
	#define consteval_if_not_tuning
#else
	#define constexpr_if_not_tuning constexpr
	#define consteval_if_not_tuning consteval
#endif
