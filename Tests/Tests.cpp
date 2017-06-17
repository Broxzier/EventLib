#include "CppUnitTest.h"
#include "stdafx.h"

#include <EventLib/Event.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EventTest
{

TEST_CLASS(EventTest)
{
public:
	TEST_METHOD(Connect)
	{
		int  i          = 0;
		auto IncrementI = [&i]() -> void { i++; };

		el::Event<void()> Signal;
		Signal();

		Assert::AreEqual(i, 0); // i == 0

		Signal.Connect(IncrementI);
		Signal();

		Assert::AreEqual(i, 1); // i == 1
	}

	TEST_METHOD(Disconnect)
	{
		int  i = 0;
		auto IncrementI = [&i]() -> void { i++; };

		el::Event<void()> Signal;
		auto connection = Signal.Connect(IncrementI);
		Signal();

		Assert::AreEqual(i, 1); // i == 1

		connection.Disconnect();
		Signal();

		Assert::AreEqual(i, 1); // i == 1
	}
};

} // namespace EventTest
