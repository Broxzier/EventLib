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

#include <unordered_map>
#include <utility>

#ifndef EL_NO_THREADSAFETY_CHECKS
#include <mutex>
#endif // EL_NO_THREADSAFETY_CHECKS

#include "Connection.hpp"
#include "Event.hpp"

namespace el
{

template <typename Key, typename... Args>
class Publisher
{
public:
	template <typename Callable>
	Connection Register(const Key & aKey, const Callable & aSlot, Location aLocation = Back)
	{ // Connect a slot to a named event in the map
#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(MapMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		return Map[aKey].Connect(aSlot, aLocation);
	}

	void Publish(const Key & aKey, Args... aArguments)
	{ // Triggers event by name
#ifndef EL_NO_THREADSAFETY_CHECKS
		MapMutex.lock();
#endif // EL_NO_THREADSAFETY_CHECKS

		auto it = Map.find(aKey);
		if (it == Map.end())
		{ // No event found for the given key
#ifndef EL_NO_THREADSAFETY_CHECKS
			MapMutex.unlock();
#endif // EL_NO_THREADSAFETY_CHECKS
			return;
		}

		// Make cope to iterate over, in case an event tries to edit the map
		Event<void(Args...)> & EventCopy = it->second;

#ifndef EL_NO_THREADSAFETY_CHECKS
		MapMutex.unlock();
#endif // EL_NO_THREADSAFETY_CHECKS

		EventCopy(std::forward<Args>(aArguments)...);
	}

	void operator()(const Key & aKey, Args... aArguments)
	{
		Publish(aKey, std::forward<Args>(aArguments)...);
	}

private:
	std::unordered_map<Key, Event<void(Args...)>> Map;
#ifndef EL_NO_THREADSAFETY_CHECKS
	std::mutex MapMutex;
#endif // EL_NO_THREADSAFETY_CHECKS
};

} // namespace el
