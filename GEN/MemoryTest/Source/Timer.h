#pragma once

#include <chrono>

class Timer
{
private:
	std::chrono::high_resolution_clock::time_point startTime;
	std::chrono::high_resolution_clock::time_point stopTime;

public:
	void start();
	void stop();
	long long millis();
	long long micros();
};
