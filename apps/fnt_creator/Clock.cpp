#include "Clock.h"

Clock::Clock()
{
	reset();
}

float Clock::getDeltaTime() const
{
	return mDeltaTime.count();
}

float Clock::getTotalTime() const
{
	Duration totalTime;

	if (mStopped)
	{
		totalTime = (mStopTime - mBaseTime);
	}
	else
	{
		totalTime = ((mCurrTime - mBaseTime) - mPausedTime);
	}

	return totalTime.count();
}

float Clock::getDeltaTimeInSecs() const
{
	return mDeltaTime.count() / 1000.0f;
}

void Clock::start()
{
	if (mStopped)
	{
		mCurrTime = std::chrono::high_resolution_clock::now();
		mPausedTime += (mCurrTime - mStopTime);
		mPrevTime = mCurrTime;
		mStopped = false;
	}
}

void Clock::stop()
{
	if (!mStopped)
	{
		mStopTime = std::chrono::high_resolution_clock::now();
		mStopped = true;
	}
}

void Clock::reset()
{
	auto currTime = std::chrono::high_resolution_clock::now();

	mBaseTime = currTime;
	mPrevTime = currTime;
	mCurrTime = currTime;
	mStopped = false;
}

void Clock::update()
{
	if (!mStopped)
	{
		mCurrTime = std::chrono::high_resolution_clock::now();
		mDeltaTime = mCurrTime - mPrevTime;
		mPrevTime = mCurrTime;
	}
}
