//general
#include <iostream>
#include <vector>
#include <forward_list>
#include <functional>


//concurrency
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <future>

//
#include "ThreadPool.h"



void print(const char* string)
{
	std::lock_guard lock(consoleMutex);
///	printf("Thread_%i ", std::this_thread::get_id());
	printf(string);
	printf("\n");
}

std::uint64_t HeavyWork(std::uint64_t iterations)
{
	//print("HeavyWork Start");
	volatile std::uint64_t value = 0;

	for (std::uint64_t i = 0; i < iterations; ++i)
	{
		value += (i * 2654435761ULL) ^ (value >> 3);
		value ^= (value << 7);
	}
	//print("HeavyWork End");
	return value;
}

class  ExampleClass
{
public:
	ExampleClass();
	~ExampleClass();

	std::uint64_t Fibonacci(std::uint64_t n)
	{
		if (n <= 1)
			return n;
		return Fibonacci(n - 1) + Fibonacci(n - 2);

	}
private:

};

ExampleClass::ExampleClass()
{
}

ExampleClass::~ExampleClass()
{
}


#include <random>

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
	//std::future<std::uint64_t> future = pool.EnqueueTask(&ExampleClass::Fibonacci, &c, 5);

	/*for (size_t i = 0; i < 100; i++)
	{
		std::future<std::uint64_t> future = pool.EnqueueTask([] {return HeavyWork(10000); });
	}*/
	for (size_t i = 0; i < 100; i++)
	{
		int value = RandomInt(40, 50);
		auto future = pool.EnqueueTask([&c, i, value]
			{
				//print("Fibonacci Start");
				auto returnValue =  c.Fibonacci(value);
				//print("Fibonacci End");
				return returnValue;
			});
	}

	while (pool.HasTasks() || pool.ThreadsActive())
	{
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	//auto waiting = future.get();
	//std::cout << waiting;
	return 0;
}
