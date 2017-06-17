#include <EventLib/Event.hpp>
#include <EventLib/EventQueue.hpp>
#include <EventLib/Publisher.hpp>
#include <cstdio>
#include <functional>
#include <iostream>
#include <thread>

void function(float value)
{
	std::printf("Function: %f\n", value);
}

class Functor
{
public:
	void operator()(float value)
	{
		std::printf("Functor: %f\n", value);
	}
};

void test1()
{
	struct Stats
	{
		unsigned int MeleeDamage;
		unsigned int Defence;
	};

	// Create event
	el::Event<void(Stats &)> PlayerStats;

	// "Equip" weapons and armour
	PlayerStats.Connect([](Stats & aStats) { aStats.MeleeDamage += 300; }); // Main hand weapon
	PlayerStats.Connect([](Stats & aStats) { aStats.MeleeDamage += 150; }); // Off-hand weapon
	PlayerStats.Connect([](Stats & aStats) { aStats.Defence += 50; });      // Platebody

	// Calculate stats
	Stats stats = { 0 };
	PlayerStats(stats);
	std::printf("Player stats are:\nMelee damage = %d\nDefence = %d", stats.MeleeDamage, stats.Defence);
}

void test2()
{
	auto lambda1 = [](float value) {
		// Print floating point value
		std::printf("lambda: %f\n", value);
	};
	std::function<void(float)> func(lambda1);
	func(1);

	Functor                    t;
	std::function<void(float)> functor(t);
	functor(2);

	auto bound = std::bind(t, 4.0f);
	bound();
}

// Dependent bool type from https://rmf.io/cxx11/type-traits-galore#dependent_boolean
template <bool B, typename...>
struct dependent_bool_type : std::integral_constant<bool, B>
{
};
template <bool B, typename... T>
using Bool = typename dependent_bool_type<B, T...>::type;

template <typename Signature>
class Func
{
	static_assert(Bool<false, Signature>{}, "Template arguments needs to be a signature");
};

template <typename ResultType, typename... Args>
class Func<ResultType(Args...)>
{
	using callback = ResultType(Args...);

public:
	Func() = default;

	template <typename F>
	Func(F f)
	    : ptr((callback *)f)
	{
	}

	template <typename... Args>
	void operator()(Args... args)
	{
		(*ptr)(args...);
	}

private:
	callback * ptr;
};

void test3()
{
	Func<void(float)> abc(function);
	abc(123.0f);
}

void test4()
{
	class TestClass
	{
	public:
		TestClass() = default; // Trivial constructor
		TestClass(int)
		{
		}
	};

	// Print boolean values as "false" and "true" instead of "0" and "1"
	std::cout << std::boolalpha;
	std::cout << std::is_floating_point<float>::value << std::endl;
	std::cout << std::is_trivially_constructible<TestClass>::value << std::endl;
	std::cout << std::is_trivially_assignable<int, TestClass>::value << std::endl;
	std::cout << std::is_assignable<TestClass, int>::value << std::endl;

	// Type traits types
	bool                                                 a = true;
	typename std::add_const<bool>::type                  b = true;
	typename std::add_rvalue_reference<const bool>::type c = true;
	std::is_same<int, int>::value;

	el::Connection con1;
	el::Connection con2 = con1;
	con1.SetBlocking(true);
	std::cout << con1.Blocking() << std::endl;
}

void test5()
{
	el::EventQueue<void()> Queue;
	Queue.Connect([]() { std::puts("One"); });
	Queue.Execute();
	Queue.Connect([]() { std::puts("Two"); });
	Queue.Connect([]() { std::puts("Three"); });
	Queue();
}

void test6()
{
	el::Publisher<std::string> Pub;
	el::Connection             Con = Pub.Register("One", []() { std::puts("1"); });
	Pub.Register("Two", []() { std::puts("2"); });
	Pub.Register("Three", []() { std::puts("3"); });
	Pub.Publish("One");
	Pub.Publish("One");
	Pub.Publish("Three");

	Con.Disconnect();
	Pub.Publish("One");

	// Expected output:
	// 1
	// 1
	// 3
}

bool test7()
{
	// Concurrently connect functions to an event to test thread-safety
	auto                   Increase = [](int & ref) { ref++; };
	el::Event<void(int &)> Ev;

	// Run two threads that connect a function to a shared event
	std::thread ThreadA([&]() {
		for (int i = 0; i < 10000; i++)
			Ev.Connect(Increase);
	});
	std::thread ThreadB([&]() {
		for (int i = 0; i < 10000; i++)
			Ev.Connect(Increase);
	});

	ThreadA.join();
	ThreadB.join();

	int a = 0;
	Ev(a);
	printf("Expecting a to have value 20000. Value = %d\n", a);

	return a == 20000;
}

bool test8()
{ // Scoped connection test
	int  i           = 0;
	auto Incrementer = [&i]() { i++; };

	el::Event<void()> Ev;

	Ev(); // i += 0 = 0

	Ev.Connect(Incrementer);

	Ev(); // i += 1 = 1

	{
		el::ScopedConnection Con = Ev.Connect(Incrementer);
		Ev(); // i += 2 = 3
	}

	Ev(); // i += 1 = 4

	return i == 4;
}

bool test9()
{ // Disconnecting
	int  i           = 0;
	auto Incrementer = [&i]() { i++; };

	el::Event<void()> ev;
	el::Connection    con = ev.Connect(Incrementer);

	ev.Disconnect(con);

	ev();

	return i == 0;
}

int main()
{
	bool result = test9();
	if (result == false)
	{
		std::puts("TEST FAILED");
	}

	// Pause
	std::getchar();

	return 0;
}
