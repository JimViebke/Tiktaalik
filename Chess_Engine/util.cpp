
#include "util.hpp"

namespace chess::util
{
	timepoint time_in_ms()
	{
		using namespace std::chrono;
		return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	}
}
