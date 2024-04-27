#pragma once

#include <chrono>

namespace chess::util
{
	using timepoint = std::chrono::milliseconds::rep;
	timepoint time_in_ms();
}
