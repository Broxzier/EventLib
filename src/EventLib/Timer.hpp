#pragma region Copyright (c) 2017 Hielke Morsink
/*****************************************************************************
 * EventLib, a C++ library to provide classes for event-based programming.
 *
 * EventLib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#pragma once

#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <thread>

#ifndef EL_NO_THREADSAFETY_CHECKS
#include <mutex>
#endif // EL_NO_THREADSAFETY_CHECKS

#include "Event.hpp"

namespace el
{

template <typename Time>
class TimerManager;

template <typename Time>
class Timer : public std::enable_shared_from_this<Timer<Time>>
{
	using _TimeManager = TimerManager<Time>;

public:
	Timer() = default;
	Timer(const Time & aInterval, bool aLooping, _TimeManager * aManager)
	    : Interval(aInterval)
	    , Watch(aInterval)
	    , Looping(aLooping)
	    , Manager(aManager)
	{
	}

	const Time & GetTimeLeft() const
	{
		return Watch;
	}

	const Time & GetInterval() const
	{
		return Interval;
	}

	void SetTimeLeft(const Time & aTimeLeft)
	{
		Watch = aTimeLeft;
	}

	void Reset()
	{ // Restarts the countdown from the start
		Watch = Interval;
	}

	bool HasFinished() const
	{ // Returns true when the timer has finished and is not looping
		return !Looping && Watch <= Time(0);
	}

	void Tick(const Time & aDeltaTime)
	{
		if (!Paused)
		{ // Timer is not paused
			Watch -= aDeltaTime;
			if (Watch <= Time(0))
			{ // Timer has reached 0
				OnTrigger();

				if (Looping)
				{ // The timer should start again
					Reset();
				}
			}
		}
	}

	void Pause()
	{ // Set paused to true
		if (!Paused)
		{
			Paused = true;
			OnPause();
		}
	}

	void Resume()
	{ // Set paused to false
		if (Paused)
		{
			Paused = false;
			OnResume();
		}
	}

	void Delete()
	{ // Removes the timer from the timer manager
		Manager->Remove(shared_from_this());
	}

private:
	Time           Interval;
	Time           Watch;
	bool           Paused = false;
	bool           Looping;
	_TimeManager * Manager;

public: // Events
	Event<void()> OnTrigger;
	Event<void()> OnPause;
	Event<void()> OnResume;
};

template <typename Time>
class TimerManager
{
	using _Timer    = Timer<Time>;
	using _TimerPtr = std::shared_ptr<_Timer>;

public:
	TimerManager()                     = default;
	TimerManager(const TimerManager &) = delete;

	template <typename T, typename Callable>
	static void CreateThreaded(const T aSleepTime, Callable aFunc)
	{ // Create a detached thread to execute the logic after a timeout
		std::thread thread([=]() {
			std::this_thread::sleep_for(Time(aSleepTime));
			aFunc();
		});
		thread.detach();
	}

	std::shared_ptr<_Timer> & Create(const Time aSleepTime, bool aLooping = false)
	{ // Create a timer and returns a shared pointer to it
		_TimerPtr ptr = std::make_shared<_Timer>(_Timer(aSleepTime, aLooping, this));
#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> lock(TimersMutex);
#endif // EL_NO_THREADSAFETY_CHECKS
		Timers.push_back(ptr);
		return Timers.back();
	}

	void Remove(const _TimerPtr & ptr)
	{ // Removes a timer from the list
		// Since the list contains shared pointers, we don't have to destruct it here
		Timers.remove(ptr);
	}

	void UpdateTimers(const Time & aDeltaTime)
	{ // Removes finished timers, and ticks running timers
#ifndef EL_NO_THREADSAFETY_CHECKS
		TimersMutex.lock();
#endif // EL_NO_THREADSAFETY_CHECKS

		Timers.remove_if([](const _TimerPtr & aTimer) -> bool {
			return aTimer->HasFinished();
		});

		// Create a copy, in case the list gets changed
		std::list<_TimerPtr> TimersCopy = Timers;

#ifndef EL_NO_THREADSAFETY_CHECKS
		TimersMutex.unlock();
#endif // EL_NO_THREADSAFETY_CHECKS

		std::for_each(TimersCopy.begin(), TimersCopy.end(), [&aDeltaTime](_TimerPtr & aTimer) {
			aTimer->Tick(aDeltaTime);
		});
	}

private:
	std::list<_TimerPtr> Timers;
#ifndef EL_NO_THREADSAFETY_CHECKS
	std::mutex TimersMutex;
#endif // EL_NO_THREADSAFETY_CHECKS
};

} // namespace el
