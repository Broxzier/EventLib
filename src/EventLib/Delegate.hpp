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

#include <functional> // std::bind, std::function
#include <utility>    // std::forward

namespace el
{

template <typename T>
class Delegate;

template <typename R, typename... Args>
class Delegate<R(Args...)>
{
	using Function = std::function<R(Args...)>;

public:
	Delegate(Function func)
	{
		callable = func;
	}

	template <typename Owner>
	Delegate(Owner * aOwner, R (Owner::*aMethod)(Args...))
	    : Delegate(std::bind(aMethod, aOwner))
	{
	}

	Delegate<R(Args...)> & operator()(Args... args)
	{
		callable(std::forward<Args>(args)...);
		return *this;
	}

private:
	Function callable;
};

} // namespace el
