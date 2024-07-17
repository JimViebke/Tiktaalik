
#include <chrono>
#include <format>
#include <fstream>

#include "util.hpp"

namespace chess::util
{
	timepoint time_in_ms()
	{
		using namespace std::chrono;
		return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	}

	void log(const std::string& output)
	{
		using namespace std::chrono;

		const zoned_time zoned_time{ current_zone(), system_clock::now() };

		std::stringstream ss;
		ss << std::format("{:%T} ", zoned_time) << output << '\n';

		std::ofstream logfile("tiktaalik.log", std::ios_base::app);
		logfile << ss.str();
		logfile.flush();
	}

	std::vector<std::string> tokenize(const std::string& str)
	{
		std::stringstream ss(str);
		std::string token;
		std::vector<std::string> tokens;

		while (std::getline(ss, token, ' '))
		{
			if (token.size() > 0)
			{
				tokens.push_back(token);
			}
		}

		return tokens;
	}
}
