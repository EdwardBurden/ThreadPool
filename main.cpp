//General
#include <iostream>
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


	// Example 1, block main thread and wait for all tasks
	std::future<std::uint64_t> future = pool.EnqueueTask(&ExampleClass::Fibonacci, &c, 40);
	for (int i = 0; i < 20; i++)
	{
		int value = RandomInt(1, 30);
		auto temp = pool.EnqueueTask([&c, i, value]
			{
				auto returnValue = c.Fibonacci(value);
				return returnValue;
			});
	}

	pool.WaitForAllTasksComplete();
	std::cout << "Example 1:" << future.get() << "\n"; // Future guaranteed to be ready

	// Example 2, block main thread waiting for one specific task
	for (int i = 0; i < 20; i++)
	{
		int value = RandomInt(40, 45);
		auto temp = pool.EnqueueTask([&c, i, value]
			{
				auto returnValue = c.Fibonacci(value);
				return returnValue;
			});
	}
	future = pool.EnqueueTask(&ExampleClass::Fibonacci, &c, 10);
	auto number = future.get();
	std::cout << "Example 2:" << number << "\n";
	pool.WaitForAllTasksComplete(); //wait for all others before starting next example

	// Example 3, start tasks and dont block main thread. Waits for all tasks be complete
	for (int i = 0; i < 20; i++)
	{
		int value = RandomInt(40, 45);
		auto temp = pool.EnqueueTask([&c, i, value]
			{
				auto returnValue = c.Fibonacci(value);
				return returnValue;
			});
	}
	future = pool.EnqueueTask(&ExampleClass::Fibonacci, &c, 5);
	while (pool.HasTasks() && pool.WorkersActive())
	{
		//Do something else while tasks are pending
	}
	number = future.get();
	std::cout << "Example 3:" << number << "\n";

	// Example 4, only waiting for the task we care about, but not blocking main thread
	for (int i = 0; i < 20; i++)
	{
		int value = RandomInt(40, 45);
		auto temp = pool.EnqueueTask([&c, i, value]
			{
				auto returnValue = c.Fibonacci(value);
				return returnValue;
			});
	}
	future = pool.EnqueueTask(&ExampleClass::Fibonacci, &c, 5);
	while (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		//Do something else while waiting.
	}
	number = future.get();
	std::cout << "Example 4:" << number << "\n"; // Chance the output will be mangled in debug since other threads are potentially writting to the console.
	// Example 4 

	return 0;
}
