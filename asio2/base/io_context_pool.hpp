/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_IO_CONTEXT_POOL_HPP__
#define __ASIO2_IO_CONTEXT_POOL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <vector>
#include <memory>
#include <thread>
#include <mutex>

#include <asio2/base/selector.hpp>
#include <asio2/base/def.hpp>

namespace asio2
{

	/**
	 * the io_context_pool interface
	 * note : must ensure all events completion handler of the same socket run in the same thread within 
	 *        io_context::run,the event completion handler is mean to async_read async_send and so on...
	 */
	class io_context_pool
	{
	public:
		typedef asio::executor_work_guard<asio::io_context::executor_type> io_context_work;

		/**
		 * @construct
		 * @param    : pool_size - the new pool size,default is equal to the cpu kernel count
		 */
		explicit io_context_pool(std::size_t pool_size = std::thread::hardware_concurrency() * 2)
		{
			if (pool_size == 0)
			{
				pool_size = std::thread::hardware_concurrency() * 2;
			}

			for (std::size_t i = 0; i < pool_size; ++i)
			{
				this->m_io_contexts.emplace_back(std::make_shared<asio::io_context>(1));
			}
		}

		/**
		 * @destruct
		 */
		virtual ~io_context_pool()
		{
		}

		/**
		 * @function : run all io_context objects in the pool.
		 */
		void run()
		{
			std::lock_guard<std::mutex> guard(this->mutex_);

			ASIO2_ASSERT(this->m_works.size() == 0 && this->m_threads.size() == 0);
			// Create a pool of threads to run all of the io_contexts. 
			for (auto & io_context_ptr : this->m_io_contexts)
			{
				// Give all the io_contexts work to do so that their run() functions will not 
				// exit until they are explicitly stopped. 
				this->m_works.emplace_back(std::make_shared<io_context_work>(io_context_ptr->get_executor()));

				// start work thread
				this->m_threads.emplace_back(
					// when bind a override function,should use static_cast to convert the function to correct function version
					std::bind(static_cast<std::size_t(asio::io_context::*)()>(
						&asio::io_context::run), io_context_ptr));
			}

			stop_ = false;
		}

		/**
		 * @function : stop all io_context objects in the pool
		 */
		void stop()
		{
			{
				std::lock_guard<std::mutex> guard(this->mutex_);

				if (stop_)
					return;

				if (this->m_works.empty() && this->m_threads.empty())
					return;

				if (this->running_in_iopool_threads())
					return;

				stop_ = true;
			}

			this->wait_iothreads();

			{
				std::lock_guard<std::mutex> guard(this->mutex_);

				// call work reset,and then the io_context working thread will be exited.
				for (auto & work_ptr : this->m_works)
				{
					work_ptr->reset();
				}
				this->m_works.clear();
				// Wait for all threads to exit. 
				for (auto & thread : this->m_threads)
				{
					ASIO2_ASSERT(thread.get_id() != std::this_thread::get_id());
					if (thread.joinable())
						thread.join();
				}
				this->m_threads.clear();
				// Reset the io_context in preparation for a subsequent run() invocation.
				for (auto & io_context_ptr : this->m_io_contexts)
				{
					io_context_ptr->restart();
				}
			}
		}

		/**
		 * @function : get an io_context to use
		 */
		std::shared_ptr<asio::io_context> get_io_context_ptr()
		{
			// Use a round-robin scheme to choose the next io_context to use. 
			return this->m_io_contexts[(this->m_next_io_context++) % this->m_io_contexts.size()];
		}

		/**
		 * @function : Determine whether current code is running in the iopool threads.
		 */
		inline bool running_in_iopool_threads()
		{
			std::thread::id curr_tid = std::this_thread::get_id();
			for (auto & thread : this->m_threads)
			{
				if (curr_tid == thread.get_id())
					return true;
			}
			return false;
		}

		/**
		 * Use to ensure that all nested asio::post(...) events are fully invoked.
		 */
		void wait_iothreads()
		{
			// Must first open the following files to change private to public,
			// otherwise, the outstanding_work_ variable will not be accessible.

			// row 130 asio/detail/scheduler.hpp 
			// row 209 asio/detail/win_iocp_io_context.hpp 
			// row 596 asio/io_context.hpp 

			for (;;)
			{
				long ms = 0;
				bool idle = true;
				for (auto & io_context_ptr : this->m_io_contexts)
				{
					if (io_context_ptr->impl_.outstanding_work_ > 1)
					{
						idle = false;
						ms += io_context_ptr->impl_.outstanding_work_ - 1;
					}
				}
				if (idle) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(
					(std::max<long>)((std::min<long>)(ms, 10), 1)));
			}
		}
	protected:
		/// threads to run all of the io_contexts
		std::vector<std::thread> m_threads;

		/// The pool of io_contexts. 
		std::vector<std::shared_ptr<asio::io_context>> m_io_contexts;

		/// The work that keeps the io_contexts running. 
		std::vector<std::shared_ptr<io_context_work>> m_works;

		/// The next io_context to use for a connection. 
		std::size_t m_next_io_context = 0;

		/// 
		std::mutex  mutex_;

		/// 
		volatile bool stop_ = true;

	};

}

#endif // !__ASIO2_IO_CONTEXT_POOL_HPP__
