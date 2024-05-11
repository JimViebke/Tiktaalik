#pragma once

#include <stdint.h>

namespace chess
{
	namespace config
	{
		static constexpr size_t engine_target_depth = 8;
	}

	namespace tt::config
	{
		static constexpr size_t size_in_mb = 1024;
		static constexpr bool require_exact_depth_match = false;
		static constexpr bool use_tt_move_ordering = false;
	}
}
