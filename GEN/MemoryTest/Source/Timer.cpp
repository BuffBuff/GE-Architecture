#include "Timer.h"

typedef std::chrono::high_resolution_clock cl;

void Timer::start()
{
	startTime = cl::now();
}

void Timer::stop()
{
	stopTime = cl::now();
}

long long Timer::millis()
{
	cl::duration dur = stopTime - startTime;
	std::chrono::milliseconds milliSec = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
	return milliSec.count();
}

long long Timer::micros()
{
	cl::duration dur = stopTime - startTime;
	std::chrono::microseconds microSec = std::chrono::duration_cast<std::chrono::microseconds>(dur);
	return microSec.count();
}
