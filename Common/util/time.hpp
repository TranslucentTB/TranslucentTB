#pragma once
#include <chrono>
#include <ctime>

namespace Util {
	// Gets the number of T elapsed since the Unix epoch
	template<typename T = std::chrono::seconds>
	inline T GetTime()
	{
		using namespace std::chrono;
		return duration_cast<T>(system_clock::now().time_since_epoch());
	}
}

// Gets the time elapsed since the Unix epoch for use with C APIs
template<>
inline std::time_t Util::GetTime<std::time_t>()
{
	using std::chrono::system_clock;
	return system_clock::to_time_t(system_clock::now());
}