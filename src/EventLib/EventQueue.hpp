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

#include <memory>
#include <queue>

#ifndef EL_NO_THREADSAFETY_CHECKS
#include <mutex>
#endif // EL_NO_THREADSAFETY_CHECKS

#include "Connection.hpp"
#include "Delegate.hpp"

namespace el
{

template <typename Signature>
class EventQueue;

template <typename R, typename... Args>
class EventQueue<R(Args...)>
{
	using _Slot = Delegate<R(Args...)>;

	struct Entry
	{
		Connection             connection;
		std::shared_ptr<_Slot> slot;
	};

public:
	template <typename Callable>
	Connection Connect(const Callable & aSlot)
	{
		std::shared_ptr<_Slot> SlotPtr = std::make_shared<_Slot>(aSlot);
		Connection             connection(SlotPtr);

#ifndef EL_NO_THREADSAFETY_CHECKS
		std::lock_guard<std::mutex> Lock(QueueMutex);
#endif // EL_NO_THREADSAFETY_CHECKS

		Queue.push(Entry{ connection, SlotPtr });
		return connection;
	}

	inline void operator()(Args... aArguments)
	{
#ifndef EL_NO_THREADSAFETY_CHECKS
		QueueMutex.lock();
#endif // EL_NO_THREADSAFETY_CHECKS

		while (!Queue.empty())
		{ // Run over the entire queue
			Entry entry = Queue.front();
			Queue.pop();

#ifndef EL_NO_THREADSAFETY_CHECKS
			QueueMutex.unlock();
#endif // EL_NO_THREADSAFETY_CHECKS

			if (entry.connection.Connected())
			{ // Slot is connected
				(*entry.slot)(std::forward<Args>(aArguments)...);
			}

#ifndef EL_NO_THREADSAFETY_CHECKS
			QueueMutex.lock();
#endif // EL_NO_THREADSAFETY_CHECKS
		}

#ifndef EL_NO_THREADSAFETY_CHECKS
		QueueMutex.unlock();
#endif // EL_NO_THREADSAFETY_CHECKS
	}

	inline void Execute(Args... aArguments)
	{
		operator()(std::forward<Args>(aArguments)...);
	}

private:
	std::queue<Entry> Queue;
#ifndef EL_NO_THREADSAFETY_CHECKS
	std::mutex QueueMutex;
#endif // EL_NO_THREADSAFETY_CHECKS
};

} // namespace el
