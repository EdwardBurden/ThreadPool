#pragma once
//General
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <type_traits>
#include <algorithm>

//Concurrency
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>

#ifdef THREADPOOL_DEBUG
inline std::mutex consoleMutex;
#endif //THREADPOOL_DEBUG

class ThreadPool;
class WorkerThread
{
public:
	WorkerThread(ThreadPool* pool, int index);
	WorkerThread(const WorkerThread&) = delete;
	WorkerThread& operator=(const WorkerThread&) = delete;
	WorkerThread(WorkerThread&&) = delete;
	WorkerThread& operator=(WorkerThread&&) = delete;

	void Run();
	~WorkerThread();
	bool IsActive() const { return m_isActive; }

private:
	std::atomic<bool> m_isActive{ false };
	ThreadPool* m_pool;
	std::thread m_thread;
	int m_index;
};

class ThreadPool
{
public:

	friend class WorkerThread;

	ThreadPool(size_t size);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	// TODO comment about how future maybe cause an error if thradpool has already shutdown.
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
	bool WorkersActive();
	void WaitForAllTasksComplete();

private:
	std::atomic<bool> m_stopFlag{ false };
	std::vector<std::unique_ptr<WorkerThread>> m_workers;
	std::queue<std::function<void()>> m_tasks;
	size_t m_size;
	std::mutex m_queueLock;
	std::condition_variable m_condition;
};


//WorkerThread
inline WorkerThread::WorkerThread(ThreadPool* pool, int index)
{
	m_pool = pool;
	m_index = index;
	m_thread = std::thread(&WorkerThread::Run, this);

#ifdef THREADPOOL_DEBUG
	{
		std::lock_guard lock(consoleMutex);
		printf("%i Worker Alive\n", m_index);
	}
#endif //THREADPOOL_DEBUG
}

inline WorkerThread::~WorkerThread()
{
	if (m_thread.joinable())
		m_thread.join();

#ifdef THREADPOOL_DEBUG
	{
		std::lock_guard lock(consoleMutex);
		printf("%i Worker Dead\n", m_index);
	}
#endif //THREADPOOL_DEBUG
}

inline void WorkerThread::Run()
{
	while (true)
	{
		std::function<void()> currentTask;
		{
			std::unique_lock<std::mutex> lock(m_pool->m_queueLock);
			m_pool->m_condition.wait(lock, [this]
				{
					auto conditionPassed = !m_pool->m_tasks.empty() || m_pool->m_stopFlag;
					return conditionPassed;
				});
			if (m_pool->m_stopFlag)
				return;

			currentTask = std::move(m_pool->m_tasks.front());
			m_pool->m_tasks.pop();
		}
		m_isActive = true;

#ifdef THREADPOOL_DEBUG
		{
			std::lock_guard lock(consoleMutex);
			printf("%i Start\n", m_index);
		}
#endif //THREADPOOL_DEBUG
		currentTask();
#ifdef THREADPOOL_DEBUG
		{
			std::lock_guard lock(consoleMutex);
			printf("%i End\n", m_index);
		}
#endif //THREADPOOL_DEBUG
		m_isActive = false;
		m_pool->m_condition.notify_all();
	}
}

//ThreadPool
inline ThreadPool::ThreadPool(size_t size)
{
	unsigned int max = std::thread::hardware_concurrency();
	if (max == 0)
		max = 1;
	m_size = std::clamp(static_cast<unsigned int>(size), 1u, max);
	m_workers.reserve(m_size);
	for (size_t i = 0; i < m_size; i++)
	{
		m_workers.emplace_back(std::make_unique<WorkerThread>(this, i));
	}
#ifdef THREADPOOL_DEBUG
	{
		std::lock_guard lock(consoleMutex);
		printf("Pool Alive Size:%zu\n", m_size);
	}
#endif //THREADPOOL_DEBUG
}

inline ThreadPool::~ThreadPool()
{
	{
		std::lock_guard lock(m_queueLock);
		m_stopFlag = true;
	}
	m_condition.notify_all();
	m_workers.clear();

#ifdef THREADPOOL_DEBUG
	{
		std::lock_guard lock(consoleMutex);
		printf("Pool Dead\n");
	}
#endif //THREADPOOL_DEBUG
}

inline bool ThreadPool::HasTasks()
{
	std::lock_guard lock(m_queueLock);
	return !m_tasks.empty();
}

inline bool ThreadPool::WorkersActive()
{
	for (auto& worker : m_workers)
	{
		if (worker->IsActive())
			return true;
	}
	return false;
}

inline void ThreadPool::WaitForAllTasksComplete()
{
	std::unique_lock<std::mutex> lock(m_queueLock);
	m_condition.wait(lock, [this]
		{
			return m_tasks.empty() && !WorkersActive();
		});
}