#pragma once

//#define TIMER(str) Timer str = Timer(#str,__LINE__*strlen(__FILE__))

#include "Engone/Logger.h"

namespace engone {

	// Time since epoch in seconds, microsecond precision
	double GetSystemTime();
	// the time of your computer clock, i think 
	std::string GetClock();
	// optimized version of GetClock
	// str must be at least 8 bytes in size.
	void GetClock(char* str);

	enum FormatTimePeriod : uint8_t {
		FormatTimeNS = 1, // nanoseconds
		FormatTimeUS = 2, // microseconds
		FormatTimeMS = 4, // milliseconds 
		FormatTimeS = 8, // seconds
		FormatTimeM = 16, // minutes
		FormatTimeH = 32, // hours
		FormatTimeD = 64, // days
		FormatTimeW = 128, // weeks
	};
	typedef int FormatTimePeriods;
	// if compact is true, result will be 1d 5h 1m 13s, false is 1 day 5 hours 1 minutes 13 seconds.
	// flags are used to determine what literals you want. day, hour, minute, second is default.
	// the smallest literal will be what time should be in. with default. time is in seconds
	// If you want precision, this function is best.
	// If you only use the day flag when the time is smaller than a day, an empty string will be returned.
	std::string FormatTime(uint64_t time, bool compact = false, FormatTimePeriods flags = FormatTimeD | FormatTimeH | FormatTimeM | FormatTimeS);
	// if compact is true, result will be 1d 5h 1m 13s, false is 1 day 5 hours 1 minutes 13 seconds.
	// flags are used to determine what literals you want. day, hour, minute, second is default.
	// If you want precision, use this function but with uin64_t in nanoseconds instead. Doubles are a bit special.
	// If you only use the day flag when the time is smaller than a day, an empty string will be returned.
	std::string FormatTime(double seconds, bool compact = false, FormatTimePeriods flags = FormatTimeD | FormatTimeH | FormatTimeM | FormatTimeS);

	// instead std::this_thread::sleep_for, microsecond precision
	// void Sleep(double seconds);

// #ifdef ENGONE_LOGGER
// 	// this class is based on engone logger.
// 	// it doesn't work without it. You could use std::cout instead
// 	// but i dont want to.
// 	class Timer {
// 	public:
// 		Timer(const char* name) : name(name), startTime(GetSystemTime()) {}
// 		~Timer() {
// 			stop();
// 		}
// 		void stop() {
// 			if (startTime == 0) return;
// 			double diff = GetSystemTime() - startTime;
// 			startTime = 0;
// 			if (name)
// 				log::out << name;
// 			else
// 				log::out << "Time";
// 			log::out << ": " << diff << "\n";
// 		}

// private:
// 	const char* name = nullptr;
// 	double startTime = 0;
// };
// #endif
}