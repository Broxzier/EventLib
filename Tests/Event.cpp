#include "CppUnitTest.h"

#include <EventLib/Event.hpp>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EventLibTest
{

TEST_CLASS(EventTest)
{
public:
	TEST_METHOD(EventConnect)
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

	TEST_METHOD(EventDisconnect)
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

	TEST_METHOD(EventConnectSimultaneously)
	{
		int  i = 0;
		auto IncrementI = [&i]() -> void { i++; };

		el::Event<void()> Signal;

		// Run two threads that connect a function to a shared event
		std::thread ThreadA([&]() {
			for (int i = 0; i < 5000; i++)
				Signal.Connect(IncrementI);
		});
		std::thread ThreadB([&]() {
			for (int i = 0; i < 5000; i++)
				Signal.Connect(IncrementI);
		});

		ThreadA.join();
		ThreadB.join();

		// Should call the IncrementI lambda 10000 times
		Signal();

		Assert::AreEqual(i, 10000);
	}
};

} // namespace EventTest
