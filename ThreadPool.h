#pragma once
//general
#include <iostream>
#include <vector>
#include <forward_list>
#include <functional>
#include <memory>
#include <queue>
#include <type_traits>
#include <algorithm>

//concurrency
#include <thread>
#include <mutex>
#include <atomic>

#include <condition_variable>
#include <future>
std::mutex consoleMutex;

class ThreadPool;
class WorkerThread
{
public:
	WorkerThread();
	WorkerThread(ThreadPool* pool, int index);

	WorkerThread(const WorkerThread&) = delete;
	WorkerThread& operator=(const WorkerThread&) = delete;

	WorkerThread(WorkerThread&&);
	WorkerThread& operator=(WorkerThread&&);
	void Run();
	void Stop();
	~WorkerThread();
	bool IsActive() const { return m_isActive; }

private:
	ThreadPool* m_pool;
	std::thread m_thread;
	int m_index;
	std::atomic<bool> m_isActive;
};

class ThreadPool
{
public:
	ThreadPool(size_t size);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	template<typename F, typename... Args>
	auto EnqueueTask(F&& f, Args&&... args)
	{
		using returnType = std::invoke_result_t<F, Args...>;
		std::packaged_task<returnType()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
		std::shared_ptr<std::packaged_task<returnType()>> pointer = std::make_shared<std::packaged_task<returnType()>>(std::move(task));
		std::future<returnType> future = pointer->get_future();
		{
			std::unique_lock<std::mutex> lock(m_queueLock);
			m_tasks.emplace([pointer]()
				{
					(*pointer)();
				});
		}
		m_condition.notify_one();
		return future;
	}
	~ThreadPool();
	bool HasTasks();
	bool ThreadsActive();
	bool StopFlagActive() const { return m_stopFlag; }
	std::condition_variable& GetRunCondition() { return m_condition; }
	std::mutex& GetQueueLock() { return m_queueLock; }
	std::queue<std::function<void()>>& GetTasks() { return m_tasks; }

private:
	std::vector<std::unique_ptr<WorkerThread>> m_workers;
	std::queue<std::function<void()>> m_tasks;
	size_t m_size;
	std::mutex m_queueLock;
	std::condition_variable m_condition;
	bool m_stopFlag;
};

//////WorkerThread

WorkerThread::WorkerThread()
{
	m_pool = nullptr;
}

WorkerThread::WorkerThread(ThreadPool* pool, int index)
{
	m_pool = pool;
	m_index = index;
	m_thread = std::thread(&WorkerThread::Run, this);
	m_isActive = false;
}

WorkerThread::WorkerThread(WorkerThread&& other)
{
	m_thread = std::move(other.m_thread);
	m_pool = other.m_pool;
	m_index = other.m_index;
}

WorkerThread& WorkerThread::operator=(WorkerThread&& other)
{
	if (this == &other)
		return *this;

	m_thread = std::move(other.m_thread);
	m_pool = other.m_pool;
	m_index = other.m_index;
	return *this;
}


WorkerThread::~WorkerThread()
{
	if (m_thread.joinable())
		m_thread.join();
}

void WorkerThread::Run()
{
	while (true)
	{
		std::function<void()> currentTask;
		{
			std::unique_lock<std::mutex> lock(m_pool->GetQueueLock());
			m_pool->GetRunCondition().wait(lock, [this]
				{
					auto condtionPassed = !m_pool->GetTasks().empty() || m_pool->StopFlagActive();
					return condtionPassed;
				});
			if (m_pool->StopFlagActive())
			{
				return;
			}

			currentTask = std::move(m_pool->GetTasks().front());
			m_pool->GetTasks().pop();
		}
		m_isActive = true; 
		{
			std::lock_guard lock(consoleMutex);
			printf("%i Start", m_index);
			printf("\n");
		}
		currentTask(); 
		{
			std::lock_guard lock(consoleMutex);
			printf("%i End", m_index);
			printf("\n");
		}
		m_isActive = false;
	}
}

void WorkerThread::Stop()
{
	if (m_thread.joinable())
		m_thread.join();
}

//////ThreadPool

ThreadPool::ThreadPool(size_t size)
{
	unsigned int max = std::thread::hardware_concurrency();
	size = std::clamp((int)size, 1, (int)max);
	m_size = size;
	m_stopFlag = false;
	m_workers.reserve(size);
	for (size_t i = 0; i < size; i++)
	{
		m_workers.emplace_back(std::make_unique<WorkerThread>(this, i));
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(m_queueLock);
		m_stopFlag = true;
	}
	m_condition.notify_all();
	m_workers.clear();
}

bool ThreadPool::HasTasks()
{
	std::lock_guard lock(m_queueLock);
	return m_tasks.size() > 0;
}

bool ThreadPool::ThreadsActive()
{
	bool flag = false;
	for (auto& worker : m_workers)
	{
		if (worker->IsActive())
		{
			return true;
		}
	}
	return flag;
}