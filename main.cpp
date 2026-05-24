//General
#include <iostream>
#include <functional>
#include <random>

//ThreadPool
#include "ThreadPool.h"

class ExampleClass
{
public:
	inline std::uint64_t Fibonacci(std::uint64_t n)
	{
		if (n <= 1)
			return n;
		return Fibonacci(n - 1) + Fibonacci(n - 2);

	}
private:
};

int RandomInt(int min, int max)
{
	static std::random_device rd;
	static std::mt19937 rng(rd());
	std::uniform_int_distribution<int> dist(min, max);
	return dist(rng);
}

int main()
{
	ExampleClass c;
	ThreadPool pool(4);
	std::future<std::uint64_t> future = pool.EnqueueTask(&ExampleClass::Fibonacci, &c, 40);
	for (size_t i = 0; i < 5; i++)
	{
		int value = RandomInt(40, 50);
		auto temp = pool.EnqueueTask([&c, i, value]
			{
				auto returnValue = c.Fibonacci(value);
				return returnValue;
			});
	}

	pool.WaitForAllTasksComplete();
	std::cout << "Fibonacci Should be 102,334,155:" << future.get() << "\n";
	std::this_thread::sleep_for(std::chrono::seconds(5));

	for (size_t i = 0; i < 5; i++)
	{
		int value = RandomInt(40, 50);
		auto future = pool.EnqueueTask([&c, i, value]
			{
				auto returnValue = c.Fibonacci(value);
				return returnValue;
			});
	}

	pool.WaitForAllTasksComplete();
	return 0;
}