/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_TIMER_HPP__
#define __ASIO2_TIMER_HPP__

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
#include <stack>

namespace asio2
{

	/**
	 * the timer interface 
	 * _timer_in_own_thread : whether each timer run in its own thread,
	 * if true,one thread per timer,if flase,one thread all timers
	 */
	template<bool _timer_in_own_thread = false, class _duration = std::chrono::milliseconds> class timer {};

	//template<>
	//class timer<false>
	//{
	//public:
	//	/**
	//	 * @construct
	//	 */
	//	timer()
	//	{
	//		_worker = std::thread([this]() { this->_run(); });
	//	}

	//	/**
	//	 * @destruct
	//	 */
	//	virtual ~timer()
	//	{
	//		{
	//			std::unique_lock<std::mutex> lock(this->_mtx);
	//			this->_is_stop = true;
	//		}

	//		this->_cv.notify_all();

	//		if (_worker.joinable())
	//			_worker.join();
	//	}

	//	/**
	//	 * @function : put a task into the thread pool,the task can be 
	//	 * global function,static function,lambda function,member function
	//	 * @return : std::future<...>
	//	 */
	//	template<class Rep, class Period, class Fun, class... Args>
	//	std::size_t start(const std::chrono::duration<Rep, Period> & interval, std::size_t repeat, bool immediate, Fun&& fun, Args&&... args)
	//	{
	//		using return_type = typename std::result_of<Fun(Args...)>::type;

	//		auto task = std::make_shared< std::packaged_task<return_type()> >(
	//			std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...));

	//		std::future<return_type> future = task->get_future();

	//		{
	//			std::unique_lock<std::mutex> lock(this->_mtx);

	//			// don't allow put after stopping the pool
	//			if (this->_is_stop)
	//				throw std::runtime_error("put a task into thread pool but the pool is stopped");

	//			this->_tasks.emplace([task]() { (*task)(); });
	//		}

	//		this->_cv.notify_one();

	//		return future;
	//	}

	//protected:
	//	void _run()
	//	{

	//	}

	//private:
	//	/// no copy construct function
	//	timer(const timer&) = delete;

	//	/// no operator equal function
	//	timer& operator=(const timer&) = delete;

	//protected:

	//	// worker thread 
	//	std::thread _worker;

	//	std::stack<std::size_t> _free_ids;

	//	// the task queue
	//	std::queue<std::function<void()>> _tasks;

	//	// synchronization
	//	std::mutex _mtx;
	//	std::condition_variable _cv;

	//	// flag indicate the pool is stoped
	//	volatile bool _is_stop = false;

	//};

	template<class _duration>
	class timer<true, _duration>
	{
	protected:
		class per_timer_data
		{
		public:
			per_timer_data(const _duration & interval, std::size_t repeat, bool immediate, const std::function<void()> & cb)
				: interval_(interval), repeat_(repeat), immediate_(immediate), cb_(cb)
			{
				this->future_ = this->promise_.get_future();
				this->thread_ = std::thread([this]()
				{
					while (true)
					{
						try
						{
							if (this->immediate_)
							{
								this->do_work();
								this->immediate_ = false;
							}
							if (this->idle_)
							{
								this->future_.wait();

								if (this->future_.get() == 1)
									break;
								if (this->future_.get() == 2)
								{
									this->promise_.swap(std::promise<int8_t>{});
									this->future_ = this->promise_.get_future();
								}
							}
							auto status = this->future_.wait_for(this->interval_);
							if (status == std::future_status::timeout)
							{
								this->do_work();
							}
							else if (status == std::future_status::ready)
							{
								if (this->future_.get() == 1)
									break;
								if (this->future_.get() == 2)
								{
									this->promise_.swap(std::promise<int8_t>{});
									this->future_ = this->promise_.get_future();
								}
							}
						}
						catch (std::exception &) {}
					}
				});
			}
			~per_timer_data()
			{
				this->promise_.set_value(1);
				if (this->thread_.joinable()) this->thread_.join();
			}

			void reset(const _duration & interval, std::size_t repeat, bool immediate, const std::function<void()> & cb)
			{
				this->interval_ = interval; this->repeat_ = repeat; this->immediate_ = immediate; this->cb_ = cb;
			}

			void do_work()
			{
				(this->cb_)();
				if (repeat_ != 0)
				{
					repeat_--;
					if (repeat_ == 0)
						this->idle_ = true;
				}
			}

			void cancel()
			{
				this->idle_ = true;
				this->promise_.set_value(2);
			}

			_duration interval_{ 0 };
			std::size_t repeat_{ 0 };
			bool immediate_{ false };
			std::promise<int8_t> promise_{};
			std::future<int8_t> future_{};
			bool idle_{ false };
			std::thread thread_{};
			std::function<void()> cb_{};
		};

	public:
		/**
		 * @construct
		 */
		timer()
		{
		}

		/**
		 * @destruct
		 */
		virtual ~timer()
		{
			std::unique_lock<std::mutex> lock(this->_mtx);
			this->_is_stop = true;

			while (!this->_timers.empty())
			{
				delete this->_timers.top();
				this->_timers.pop();
			}
		}

		/**
		 * @function : put a task into the thread pool,the task can be 
		 * global function,static function,lambda function,member function
		 */
		template<class Fun, class... Args>
		std::size_t start(const _duration & interval, std::size_t repeat, bool immediate, Fun&& fun, Args&&... args)
		{
			std::lock_guard<std::mutex> guard(this->_mtx);

			if (this->_is_stop)
				throw std::runtime_error("put a task into thread pool but the pool is stopped");

			if (!this->_timers.empty())
			{
				auto & timer_data = this->_timers.top();
				if (timer_data->idle_)
				{
					this->_timers.pop();
					timer_data->reset(interval, repeat, immediate, std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...));
					this->_timers.push(timer_data);
					return std::size_t(timer_data);
				}
			}

			per_timer_data* timer_data = new per_timer_data(interval, repeat, immediate, std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...));
			this->_timers.emplace(timer_data);

			return std::size_t(timer_data);
		}

		void stop(std::size_t time_id)
		{
			(per_timer_data*(time_id))->cancel();
		}

	protected:
		struct _cmp
		{
			bool operator()(per_timer_data * l, per_timer_data * r)
			{
				if (l->idle_) return true;
				return true;
			};
		};

	private:
		/// no copy construct function
		timer(const timer&) = delete;

		/// no operator equal function
		timer& operator=(const timer&) = delete;

	protected:
		/// timers queue
		std::priority_queue<per_timer_data*, std::vector<per_timer_data*>, _cmp> _timers;

		/// synchronization
		std::mutex _mtx;

		/// flag indicate the pool is stoped
		volatile bool _is_stop = false;

	};

}

#endif // !__ASIO2_TIMER_HPP__
