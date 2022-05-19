/*****************************************************************//**
 * @file   ThreadPool.hpp
 * @brief  线程池工具类
 * 支持C++20的std::invoke函数，支持C++17的std::invoke_result函数
 * 增加wait函数与join函数
 * 参考：https://github.com/progschj/ThreadPool
 * @author 林嘉豪
 * @date   2022/05/19

Copyright (c) 2012 Jakob Progsch, Václav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
 *********************************************************************/
#pragma once
#include <thread>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include <stdexcept>

namespace ThreadPool {
	class ThreadPool {
	public:
		ThreadPool(size_t);
		template<class F, class... Args>
		auto enqueue(F&& f, Args&&... args)
			->std::future<typename std::invoke_result<F, Args...>::type>;
		~ThreadPool();
		void wait();
		void join();
	private:
		std::vector<std::thread> workers;
		std::queue<std::function<void()>> tasks;
		std::mutex queue_mutex;
		std::condition_variable condition;
		std::atomic<bool> stop;
		std::vector<unsigned char> status;
	};

	// the constructor just launches some amount of workers
	inline ThreadPool::ThreadPool(size_t threads) : stop(false), status(threads, 0)
	{
		for (size_t i = 0; i < threads; ++i)
		{
			workers.emplace_back(
				[this, i]
				{
					for (;;)
					{
						std::function<void()> task;

						{
							std::unique_lock<std::mutex> lock(this->queue_mutex);
							this->condition.wait(lock,
								[this] {return this->stop || !this->tasks.empty(); });
							if (this->stop && this->tasks.empty())
								return;
							task = std::move(this->tasks.front());
							this->tasks.pop();
							//std::cout << "Thread " << i << " is working" << std::endl;
							//std::cout << "The task size is " << this->tasks.size() << std::endl;
						}
						this->status[i] = true;
						task();
						this->status[i] = false;
					}
				}
				);
		}
	}

	// add new work item to the pool
	template <class F, class... Args>
	auto ThreadPool::enqueue(F&& f, Args&&... args)
		-> std::future<typename std::invoke_result<F, Args...>::type>
	{
		using return_type = typename std::invoke_result<F, Args...>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(queue_mutex);

			// don't allow enqueueing after stopping the pool
			if (stop)
				throw std::runtime_error("enqueue on stopped ThreadPool");

			tasks.emplace([task]() { (*task)(); });
		}
		condition.notify_one();
		return res;
	}

	// the destructor joins all threads
	inline ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop = true;
		}
		condition.notify_all();
		for (auto& worker : workers)
		{
			if (worker.joinable()) {
				worker.join();
			}
		}
	}

	inline void ThreadPool::wait()
	{
		bool isRuning = true;
		while (isRuning) {
			isRuning = false;
			for (auto& status : this->status) {
				if (status) {
					isRuning = true;
					break;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	inline void ThreadPool::join()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop = true;
		}
		condition.notify_all();
		for (auto& worker : workers)
		{
			if (worker.joinable()) {
				worker.join();
			}
		}
	}
}