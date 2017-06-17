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

namespace el
{

class Connection
{
public:
	Connection() = default;

	template <typename T>
	Connection(std::shared_ptr<T> aSlot)
	    : SlotPtr(aSlot)
	    , ConnectedPtr(std::make_shared<bool>(true))
	{
	}

	bool Connected() const
	{ // Returns whether or not the connection exists
		return *ConnectedPtr;
	}

	bool Blocking() const
	{ // Returns whether or not the connection is blocking or not
		return *BlockingPtr;
	}

	void SetBlocking(bool aBlock)
	{ // Block connection
		*BlockingPtr = aBlock;
	}

	void Disconnect()
	{ // Close connection
		*ConnectedPtr = false;
	}

	bool SharesSlotWith(const Connection & other) const
	{ // Returns whether or not the given connection shares its slot
		void * const ptr = GetValidSlotAddress();
		return ptr != nullptr && ptr == other.GetValidSlotAddress();
	}

private:
	void * GetValidSlotAddress() const
	{ // Returns the address of the slot, nullptr if invalid or expired
		return SlotPtr.lock().get();
	}

private:
	std::shared_ptr<bool> ConnectedPtr = std::make_shared<bool>(false);
	std::shared_ptr<bool> BlockingPtr  = std::make_shared<bool>(false);
	std::weak_ptr<void>   SlotPtr;
};

class ScopedConnection : public Connection
{
public:
	// No default constructor, as assigning it will cause the destructor to be called, disconnecting it immediately
	ScopedConnection(const Connection & aConnection)
	    : Connection(aConnection)
	{
	}

	~ScopedConnection()
	{ // Disconnect on destroy
		Disconnect();
	}
};

} // namespace el
