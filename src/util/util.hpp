#pragma once

#include <chrono>

namespace chess::util
{
	using timepoint = std::chrono::milliseconds::rep;
	timepoint time_in_ms();

	void log(const std::string& output);

	std::vector<std::string> tokenize(const std::string& str);
	void to_lower(std::string& str);
}
