#pragma once

#include <chrono>



using Duration = std::chrono::duration<float, std::milli>;
using TimePoint = std::chrono::high_resolution_clock::time_point;

class Clock
{
public:

	Clock();

	float getDeltaTime() const;
	float getTotalTime() const;
	float getDeltaTimeInSecs() const;

	void start();
	void stop();
	void reset();
	void update();

private:

	Duration mDeltaTime{ 0 };
	Duration mPausedTime{ 0 };
	TimePoint mBaseTime;
	TimePoint mStopTime;
	TimePoint mPrevTime;
	TimePoint mCurrTime;
	bool mStopped{ false };
};

