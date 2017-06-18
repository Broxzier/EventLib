#include "CppUnitTest.h"

#include <EventLib/EventQueue.hpp>
#include <functional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EventLibTest
{

TEST_CLASS(EventQueueTest)
{
public:
	TEST_METHOD(EventQueueConnect)
	{
		int  i = 0;
		auto IncrementI = [&i]() -> void { i++; };

		el::EventQueue<void()> Queue;

		Queue.Connect(IncrementI);
		Queue.Connect(IncrementI);

		Queue();

		Assert::AreEqual(i, 2); // i == 2

		Queue();

		Assert::AreEqual(i, 2); // i == 2
	}

	TEST_METHOD(EventQueueDisconnect)
	{
		int  i = 0;
		auto IncrementI = [&i](int aValue) -> void { i += aValue; };

		el::EventQueue<void()> Queue;

		el::Connection connection1 = Queue.Connect(std::bind(IncrementI, 1));
		el::Connection connection2 = Queue.Connect(std::bind(IncrementI, 10));
		el::Connection connection3 = Queue.Connect(std::bind(IncrementI, 100));

		connection1.Disconnect();

		Queue();

		Assert::AreEqual(i, 110);
	}
};

} // namespace EventTest
