/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 * refenced from : https://github.com/progschj/ThreadPool
 * see c++ 17 version : https://github.com/jhasse/ThreadPool
 * 
 * note :
 * 
 * 1 : when declare an global thread_pool object in dll,when enter the constructor to create std::thread,it will blocking forever.
 * 2 : when declare an global thread_pool object in dll and when dll is released,the code will run into thread_pool
 *     destructor,then call notify_all in the destructor, but the notify_all calling will blocking forever.
 * 
 * one resolve method is add a start and stop function,and move the notify_all into the stop inner,and tell user call the 
 * start and stop function manual.
 *  
 * but in order to keep the interface simple,we don't add stop function,you can use "new" "delete" way to avoid above problems,you 
 * can delete thread_pool pointer object before exit.
 * 
 * std::thread cause deadlock in DLLMain : 
 * The constructor for the std::thread cannot return until the new thread starts executing the thread procedure. When a new thread 
 * is created, before the thread procedure is invoked, the entry point of each loaded DLL is invoked for DLL_THREAD_ATTACH. To do 
 * this, the new thread must acquire the loader lock. Unfortunately, your existing thread already holds the loader lock.
 * 
 */

#ifndef __ASIO2_THREAD_POOL_HPP__
#define __ASIO2_THREAD_POOL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdlib>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace asio2
{

	/**
	 * the thread pool interface , this pool is multi thread safed,if you want use the special thread 
	 * index to put a task into the thread pool,you should use the thread_pool<true> template class
	 */
	template<bool _specify_thread = false> class thread_pool {};

	template<>
	class thread_pool<false>
	{
	public:

		/**
		 * @construct
		 */
		thread_pool(std::size_t thread_count = std::thread::hardware_concurrency())
		{
			if (thread_count < 1)
				thread_count = 1;

			this->_workers.reserve(thread_count);
			for (std::size_t i = 0; i < thread_count; ++i)
			{
				// emplace_back can use the parameters to construct the std::thread object automictly
				// use lambda function as the thread proc function,lambda can has no parameters list
				this->_workers.emplace_back([this]
				{
					for (;;)
					{
						std::function<void()> task;

						{
							std::unique_lock<std::mutex> lock(this->_mtx);
							this->_cv.wait(lock, [this] { return (this->_is_stop || !this->_tasks.empty()); });

							if (this->_is_stop && this->_tasks.empty())
								return;

							task = std::move(this->_tasks.front());
							this->_tasks.pop();
						}

						task();
					}
				});
			}
		}

		/**
		 * @destruct
		 */
		virtual ~thread_pool()
		{
			{
				std::unique_lock<std::mutex> lock(this->_mtx);
				this->_is_stop = true;
			}

			this->_cv.notify_all();

			for (auto & worker : this->_workers)
			{
				if (worker.joinable())
					worker.join();
			}
		}

		/**
		 * @function : put a task into the thread pool,the task can be 
		 * global function,static function,lambda function,member function
		 * @return : std::future<...>
		 */
		template<class Fun, class... Args>
		auto put(Fun&& fun, Args&&... args) -> std::future<typename std::result_of<Fun(Args...)>::type>
		{
			using return_type = typename std::result_of<Fun(Args...)>::type;

			auto task = std::make_shared< std::packaged_task<return_type()> >(
				std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...));

			std::future<return_type> future = task->get_future();

			{
				std::unique_lock<std::mutex> lock(this->_mtx);

				// don't allow put after stopping the pool
				if (this->_is_stop)
					throw std::runtime_error("put a task into thread pool but the pool is stopped");

				this->_tasks.emplace([task]() { (*task)(); });
			}

			this->_cv.notify_one();

			return future;
		}

		/**
		 * @function : get thread count of the thread pool
		 */
		inline std::size_t get_pool_size()
		{
			return this->_workers.size();
		}

		/**
		 * @function : get remain task size
		 */
		inline std::size_t get_task_size()
		{
			return this->_tasks.size();
		}

	private:
		/// no copy construct function
		thread_pool(const thread_pool&) = delete;

		/// no operator equal function
		thread_pool& operator=(const thread_pool&) = delete;

	protected:

		// need to keep track of threads so we can join them
		std::vector<std::thread> _workers;

		// the task queue
		std::queue<std::function<void()>> _tasks;

		// synchronization
		std::mutex _mtx;
		std::condition_variable _cv;

		// flag indicate the pool is stoped
		volatile bool _is_stop = false;

	};


	template<>
	class thread_pool<true>
	{
	public:

		/**
		 * @construct
		 */
		thread_pool(std::size_t thread_count = std::thread::hardware_concurrency())
		{
			if (thread_count < 1)
				thread_count = 1;

			this->_workers.reserve(thread_count);
			this->_mtx.reserve(thread_count);
			this->_cv.reserve(thread_count);

			for (std::size_t i = 0; i < thread_count; ++i)
			{
				this->_mtx.emplace_back(new std::mutex());
				this->_cv.emplace_back(new std::condition_variable());
				this->_tasks.emplace_back(new std::queue<std::function<void()>>());
			}

			for (std::size_t i = 0; i < thread_count; ++i)
			{
				// emplace_back can use the parameters to construct the std::thread object automictly
				// use lambda function as the thread proc function,lambda can has no parameters list
				this->_workers.emplace_back([this, i]
				{
					auto & mtx = *(this->_mtx[i]);
					auto & cv  = *(this->_cv[i]);
					auto & que = *(this->_tasks[i]);

					for (;;)
					{
						std::function<void()> task;

						{
							std::unique_lock<std::mutex> lock(mtx);
							cv.wait(lock, [this, &que] { return (this->_is_stop || !que.empty()); });

							if (this->_is_stop && que.empty())
								return;

							task = std::move(que.front());
							que.pop();
						}

						task();
					}
				});
			}
		}

		/**
		 * @destruct
		 */
		virtual ~thread_pool()
		{
			{
				for (auto & mtx : this->_mtx)
					mtx->lock();
				this->_is_stop = true;
				for (auto & mtx : this->_mtx)
					mtx->unlock();
			}

			for (auto & cv : this->_cv)
				cv->notify_all();

			for (auto & worker : this->_workers)
			{
				if (worker.joinable())
					worker.join();
			}
		}

		/**
		 * @function : put a task into the thread pool,the task can be
		 * global function,static function,lambda function,member function
		 * @param : thread_index - if thread_index is large than pool size,the thread_index will be equal to thread_index % pool_size
		 * @return : std::future<...>
		 */
		template<class Fun, class... Args>
		auto put(std::size_t thread_index, Fun&& fun, Args&&... args) -> std::future<typename std::result_of<Fun(Args...)>::type>
		{
			thread_index = thread_index % this->_workers.size();

			using return_type = typename std::result_of<Fun(Args...)>::type;

			auto task = std::make_shared< std::packaged_task<return_type()> >(
				std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...));

			std::future<return_type> future = task->get_future();

			{
				std::unique_lock<std::mutex> lock(*(this->_mtx[thread_index]));

				// don't allow put after stopping the pool
				if (this->_is_stop)
					throw std::runtime_error("put a task into thread pool but the pool is stopped");

				this->_tasks[thread_index]->emplace([task]() { (*task)(); });
			}

			this->_cv[thread_index]->notify_one();

			return future;
		}

		/**
		 * @function : get thread count of the thread pool
		 */
		inline std::size_t get_pool_size()
		{
			return this->_workers.size();
		}

		/**
		 * @function : get remain task size
		 */
		inline std::size_t get_task_size(std::size_t thread_index = -1)
		{
			std::size_t size = 0;
			if (thread_index >= this->_workers.size())
			{			
				for (auto & que : this->_tasks)
					size += que->size();
			}
			else
			{
				size = this->_tasks[thread_index]->size();
			}
			return size;
		}

	private:
		/// no copy construct function
		thread_pool(const thread_pool&) = delete;

		/// no operator equal function
		thread_pool& operator=(const thread_pool&) = delete;

	protected:

		// need to keep track of threads so we can join them
		std::vector<std::thread> _workers;

		// the task queue
		std::vector<std::unique_ptr<std::queue<std::function<void()>>>> _tasks;

		// synchronization
		std::vector<std::unique_ptr<std::mutex>> _mtx;
		std::vector<std::unique_ptr<std::condition_variable>> _cv;

		// flag indicate the pool is stoped
		volatile bool _is_stop = false;

	};


}

#endif // !__ASIO2_THREAD_POOL_HPP__
