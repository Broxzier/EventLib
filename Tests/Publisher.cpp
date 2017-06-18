#include "CppUnitTest.h"

#include <EventLib/Publisher.hpp>
#include <functional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace EventLibTest
{

TEST_CLASS(PublisherTest)
{
public:
	TEST_METHOD(PublisherConnect)
	{
		int i = 0;
		auto IncrementI = [&i](int aValue) -> void { i += aValue; };

		el::Publisher<int> Publisher;

		Publisher.Register(0, std::bind(IncrementI, 1));
		Publisher.Register(1, std::bind(IncrementI, 10));
		Publisher.Register(123, std::bind(IncrementI, 100));
		Publisher.Register(123, std::bind(IncrementI, 1000));

		// Calls only the functions that are registered to key `1`
		Publisher.Publish(1);

		Assert::AreEqual(i, 10); // i == 10

		Publisher(1); // Can also publish with the () operator

		Assert::AreEqual(i, 20); // i == 20

		Publisher(123);

		Assert::AreEqual(i, 1120); // i == 1120
	}

	TEST_METHOD(PublisherDisconnect)
	{
		int i = 0;
		auto IncrementI = [&i](int aValue) -> void { i++; };

		el::Publisher<int> Publisher;

		el::Connection connection = Publisher.Register(0, std::bind(IncrementI, 1));

		Assert::AreEqual(i, 0); // i == 0

		Publisher(0);

		Assert::AreEqual(i, 1); // i == 1

		connection.Disconnect();
		Publisher(0);

		Assert::AreEqual(i, 1); // i == 1
	}
};

} // namespace EventTest
