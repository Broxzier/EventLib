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

#include <algorithm>
#include <list>

#ifndef EL_NO_THREADSAFETY_CHECKS
#include <mutex>
#endif // EL_NO_THREADSAFETY_CHECKS

#ifndef EL_DISABLE_GROUPING
#include <map>
#endif // EL_DISABLE_GROUPING

#include "Connection.hpp"
#include "Delegate.hpp"

namespace el
{

enum Location
{
	Front,
	Back
};

template <typename Signature, typename GroupType = unsigned int>
class Event;

template <typename R, typename... Args, typename GroupType>
class Event<R(Args...), GroupType>
{
	using _Myt  = Event<R(Args...)>;
	using _Slot = Delegate<R(Args...)>;

	struct Entry
	{
		Connection             connection;
		std::shared_ptr<_Slot> slot;
	};

public:
	Event() = default;

	// Provide copy constructor, because mutexes cannot be copied
	Event(const _Myt & aOther)
	{
#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(SlotsMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		UngroupedFrontSlots = aOther.UngroupedFrontSlots;
		UngroupedBackSlots  = aOther.UngroupedBackSlots;
#ifndef EL_DISABLE_GROUPING
		GroupedSlots = GroupedSlots;
#endif // EL_DISABLE_GROUPING
		bool Enabled = aOther.Enabled;
	}

	template <typename Callable>
	Connection Connect(const Callable & aSlot, const Location aLocation = Back)
	{ // Create new connection and put it in the list
		std::shared_ptr<_Slot> Slot = std::make_shared<_Slot>(aSlot);
		Connection             connection(Slot);

#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(SlotsMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		if (aLocation == Location::Front)
			UngroupedFrontSlots.push_front(Entry{ connection, Slot });
		else
			UngroupedBackSlots.push_back(Entry{ connection, Slot });
		return connection;
	}

	template <class Owner>
	Connection Connect(Owner * aOwner, R (Owner::*aMethod)(Args...), const Location aLocation = Back)
	{ // Connect member function with zero arguments as slot
		return Connect(std::bind(std::mem_fn(aMethod), aOwner), aLocation);
	}

#ifndef EL_DISABLE_GROUPING
	template <typename Callable>
	Connection Connect(const GroupType & aGroup, const Callable & aSlot, const Location aLocation = Back)
	{ // Create new connection, create a group in the map if needed, and put the callable in its list
		std::shared_ptr<_Slot> Slot = std::make_shared<_Slot>(aSlot);
		Connection             connection(Slot);

#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(SlotsMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		if (GroupedSlots.find(aGroup) == GroupedSlots.end())
		{ // The group does not yet exist
			GroupedSlots.insert(std::make_pair(aGroup, std::list<Entry>()));
		}

		if (aLocation == Location::Front)
			GroupedSlots[aGroup].push_back(Entry{ connection, Slot });
		else
			GroupedSlots[aGroup].push_back(Entry{ connection, Slot });
		return connection;
	}
#endif // EL_DISABLE_GROUPING

	void Disconnect(const Connection & aConnection)
	{ // Remove a slot from the list
		auto RemoveFrom = [](std::list<Entry> & aEntryList, const el::Connection & aConnection) {
			aEntryList.remove_if([&aConnection](const Entry & aEntry) -> bool {
				/* Compare raw pointers */
				return aConnection.SharesSlotWith(aEntry.connection);
			});
		};

#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(SlotsMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		RemoveFrom(UngroupedFrontSlots, aConnection);
		RemoveFrom(UngroupedBackSlots, aConnection);

#ifndef EL_DISABLE_GROUPING
		for (auto & Pair : GroupedSlots)
		{
			RemoveFrom(Pair.second, aConnection);
		}
#endif // EL_DISABLE_GROUPING
	}

	void Enable()
	{ // Sets the event to enabled
		Enabled = true;
	}

	void Disable()
	{ // Sets the event to disabled, causing the () operator to not call anything
		Enabled = false;
	}

	void Clear()
	{ // Clears all connections
#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(SlotsMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		UngroupedFrontSlots.clear();
		UngroupedBackSlots.clear();
#ifndef EL_DISABLE_GROUPING
		for (auto & List : GroupedSlots)
		{ // Clear all list in the map
			List.second.clear();
		}
		GroupedSlots.clear();
#endif // EL_DISABLE_GROUPING
	}

	inline void operator()(Args... aArguments)
	{ // Call all bound slots if enabled
		if (!Enabled)
			return;

		auto RunThrough = [&aArguments...](std::list<Entry> & aEntryList) {
			std::for_each(aEntryList.begin(), aEntryList.end(), [&](Entry & aEntry) {
				if (aEntry.connection.Connected() && !aEntry.connection.Blocking())
				{ /* Connection is not blocked*/
					(*aEntry.slot)(std::forward<Args>(aArguments)...);
				}
			});
		};

#ifndef EL_NO_THREADSAFETY_CHECKS
		SlotsMutex.lock();
#endif // EL_NO_THREADSAFETY_CHECKS

		// Copy event lists over to new lists, so that the list can be changed
		std::list<Entry> FrontListCopy = UngroupedFrontSlots;
		std::list<Entry> BackListCopy  = UngroupedBackSlots;
#ifndef EL_DISABLE_GROUPING
		std::map<GroupType, std::list<Entry>> MapCopy = GroupedSlots;
#endif // EL_DISABLE_GROUPING

#ifndef EL_NO_THREADSAFETY_CHECKS
		SlotsMutex.unlock();
#endif // EL_NO_THREADSAFETY_CHECKS

		// First execute ungrouped slots which were inserted at the front
		RunThrough(FrontListCopy);

#ifndef EL_DISABLE_GROUPING
		// Execute grouped entries, the map keeps them sorted
		for (auto & GroupPair : MapCopy)
		{ // Loop over all lists in the map
			RunThrough(GroupPair.second);
		}
#endif // EL_DISABLE_GROUPING

		// And lastly execute ungrouped slots inserted at the back
		RunThrough(BackListCopy);
	}

private:
	std::list<Entry> UngroupedFrontSlots;
	std::list<Entry> UngroupedBackSlots;
#ifndef EL_DISABLE_GROUPING
	std::map<GroupType, std::list<Entry>> GroupedSlots;
#endif // EL_DISABLE_GROUPING
	bool Enabled = true;
#ifndef EL_NO_THREADSAFETY_CHECKS
	std::mutex SlotsMutex;
#endif // EL_NO_THREADSAFETY_CHECKS
};

} // namespace el
