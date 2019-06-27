/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_TRANSMITTER_IMPL_HPP__
#define __ASIO2_TRANSMITTER_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <chrono>
#include <future>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

#include <asio2/util/buffer.hpp>
#include <asio2/util/def.hpp>
#include <asio2/util/helper.hpp>
#include <asio2/util/logger.hpp>
#include <asio2/util/pool.hpp>
#include <asio2/util/rwlock.hpp>
#include <asio2/util/spin_lock.hpp>

#include <asio2/base/selector.hpp>
#include <asio2/base/socket_helper.hpp>
#include <asio2/base/io_context_pool.hpp>
#include <asio2/base/error.hpp>
#include <asio2/base/def.hpp>
#include <asio2/base/listener_mgr.hpp>
#include <asio2/base/url_parser.hpp>

namespace asio2
{

	class transmitter_impl : public std::enable_shared_from_this<transmitter_impl>
	{
		friend class sender_impl;

	public:
		/**
		 * @construct
		 */
		transmitter_impl(
			std::shared_ptr<url_parser>       url_parser_ptr,
			std::shared_ptr<listener_mgr>     listener_mgr_ptr,
			std::shared_ptr<asio::io_context> send_io_context_ptr,
			std::shared_ptr<asio::io_context> recv_io_context_ptr
		)
			: m_url_parser_ptr(url_parser_ptr)
			, m_listener_mgr_ptr(listener_mgr_ptr)
			, m_send_io_context_ptr(send_io_context_ptr)
			, m_recv_io_context_ptr(recv_io_context_ptr)
		{
			if (this->m_send_io_context_ptr)
			{
				this->m_send_strand_ptr = std::make_shared<asio::io_context::strand>(*this->m_send_io_context_ptr);
			}
			if (this->m_recv_io_context_ptr)
			{
				this->m_recv_strand_ptr = std::make_shared<asio::io_context::strand>(*this->m_recv_io_context_ptr);
			}
		}

		/**
		 * @destruct
		 */
		virtual ~transmitter_impl()
		{
		}

		/**
		 * @function : start the transmitter
		 * @param    : async_connect - asynchronous connect to the server or sync
		 * @return   : true  - start successed , false - start failed
		 */
		virtual bool start() = 0;

		/**
		 * @function : stop the transmitter
		 */
		virtual void stop()
		{
			for (auto & pair : this->m_timers)
			{
				pair.second->cancel();
			}
			this->m_timers.clear();
		}

		/**
		 * @function : check whether the transmitter is started
		 */
		virtual bool is_started() = 0;

		/**
		 * @function : check whether the transmitter is stopped
		 */
		virtual bool is_stopped() = 0;

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, unsigned short port, const std::shared_ptr<buffer<uint8_t>> & buf_ptr)
		{
			return this->send(ip, std::to_string(port), buf_ptr);
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, const std::string & port, const std::shared_ptr<buffer<uint8_t>> & buf_ptr) = 0;

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, unsigned short port, const uint8_t * buf, std::size_t len)
		{
			return this->send(ip, std::to_string(port), buf, len);
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, const std::string & port, const uint8_t * buf, std::size_t len)
		{
			return ((!ip.empty() && !port.empty() && buf) ?
				this->send(ip, port, std::make_shared<buffer<uint8_t>>(len, malloc_send_buffer(len), buf, len)) : false);
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, unsigned short port, const char * buf)
		{
			return (buf ? this->send(ip, std::to_string(port), reinterpret_cast<const uint8_t *>(buf), std::strlen(buf)) : false);
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, const std::string & port, const char * buf)
		{
			return (buf ? this->send(ip, port, reinterpret_cast<const uint8_t *>(buf), std::strlen(buf)) : false);
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, unsigned short port, const std::string & s)
		{
			return this->send(ip, std::to_string(port), reinterpret_cast<const uint8_t *>(s.data()), s.size());
		}

		/**
		 * @function : send data
		 */
		virtual bool send(const std::string & ip, const std::string & port, const std::string & s)
		{
			return this->send(ip, port, reinterpret_cast<const uint8_t *>(s.data()), s.size());
		}

	public:
		/**
		 * @function : get the local address
		 */
		virtual std::string get_local_address() = 0;

		/**
		 * @function : get the local port
		 */
		virtual unsigned short get_local_port() = 0;

		/**
		 * @function : get the remote address
		 */
		virtual std::string get_remote_address() = 0;

		/**
		 * @function : get the remote port
		 */
		virtual unsigned short get_remote_port() = 0;

		/**
		 * @function : get std::size_t user data 
		 */
		inline std::size_t get_user_data()
		{
			return this->m_user_data;
		}

		/**
		 * @function : set std::size_t user data
		 */
		inline void set_user_data(std::size_t user_data)
		{
			this->m_user_data = user_data;
		}
		
		/**
		 * @function : get std::shared_ptr user data 
		 */
		inline std::shared_ptr<void> get_user_data_ptr()
		{
			return this->m_user_data_ptr;
		}

		/**
		 * @function : set std::shared_ptr user data
		 */
		inline void set_user_data(std::shared_ptr<void> user_data_ptr)
		{
			this->m_user_data_ptr = user_data_ptr;
		}

	public:
		/**
		 * @function : get last active time 
		 */
		std::chrono::time_point<std::chrono::system_clock> get_last_active_time()
		{
			return this->m_last_active_time;
		}

		/**
		 * @function : reset last active time 
		 */
		void reset_last_active_time()
		{
			this->m_last_active_time = std::chrono::system_clock::now();
		}

		/**
		 * @function : get silence duration of milliseconds
		 */
		std::chrono::milliseconds::rep get_silence_duration()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->m_last_active_time).count();
		}

	public:
		template<class Fun, class... Args>
		bool start_timer(std::size_t timer_id, std::chrono::milliseconds duration, Fun&& fun, Args&&... args)
		{
			if (this->is_started())
			{
				std::function<void()> task = std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...);

				auto this_ptr(this->shared_from_this());
				this->m_recv_strand_ptr->post([this, this_ptr, timer_id, duration, task]()
				{
					if (this->is_started())
					{
						auto iter = this->m_timers.find(timer_id);
						if (iter == this->m_timers.end())
						{
							iter = this->m_timers.emplace(timer_id, std::make_shared<asio::steady_timer>(*this->m_recv_io_context_ptr)).first;
						}
						else
						{
							return;
						}

						auto & timer_ptr = iter->second;

						timer_ptr->expires_after(duration);
						timer_ptr->async_wait(asio::bind_executor(*this->m_recv_strand_ptr,
							std::bind(&transmitter_impl::_handle_timers, this,
								std::placeholders::_1, // error_code
								std::move(task),
								std::move(duration),
								timer_ptr,
								std::move(this_ptr)
							)));
					}
				});

				return true;
			}

			return false;
		}

		bool stop_timer(std::size_t timer_id)
		{
			auto this_ptr(this->shared_from_this());
			this->m_recv_strand_ptr->post([this, this_ptr, timer_id]()
			{
				auto iter = this->m_timers.find(timer_id);
				if (iter != this->m_timers.end())
				{
					auto & timer_ptr = iter->second;
					timer_ptr->cancel();
					this->m_timers.erase(iter);
				}
			});

			return true;
		}

	protected:
		void _handle_timers(const error_code & ec, std::function<void()> task, std::chrono::milliseconds duration,
			std::shared_ptr<asio::steady_timer> timer_ptr, std::shared_ptr<transmitter_impl> this_ptr)
		{
			set_last_error(ec.value());

			task();

			if (!ec)
			{
				if (this->is_started())
				{
					timer_ptr->expires_after(duration);
					timer_ptr->async_wait(asio::bind_executor(*this->m_recv_strand_ptr,
						std::bind(&transmitter_impl::_handle_timers, this,
							std::placeholders::_1, // error_code
							std::move(task),
							std::move(duration),
							timer_ptr,
							std::move(this_ptr)
						)));
				}
				else
				{
					std::ignore = 0;
				}
			}
			else
			{
				// occur error,may be cancel is called
				std::ignore = 0;
			}
		}

	protected:
		/// url parser
		std::shared_ptr<url_parser>                        m_url_parser_ptr;

		/// listener manager
		std::shared_ptr<listener_mgr>                      m_listener_mgr_ptr;

		/// The io_context used to handle the socket event.
		std::shared_ptr<asio::io_context>                  m_send_io_context_ptr;

		/// The io_context used to handle the socket event.
		std::shared_ptr<asio::io_context>                  m_recv_io_context_ptr;

		/// The strand used to handle the socket event.
		std::shared_ptr<asio::io_context::strand>          m_send_strand_ptr;

		/// The strand used to handle the socket event.
		std::shared_ptr<asio::io_context::strand>          m_recv_strand_ptr;

		/// use to check whether the user call stop in the listener
		volatile state                                     m_state = state::stopped;

		/// user data
		std::size_t                                        m_user_data = 0;
		std::shared_ptr<void>                              m_user_data_ptr;

		/// last active time 
		std::chrono::time_point<std::chrono::system_clock> m_last_active_time = std::chrono::system_clock::now();

		/// endpoint cache map
		std::unordered_map<std::size_t, asio::ip::udp::endpoint> m_endpoint_cache;

		// synchronization
		std::mutex                                               m_stopped_mtx;
		std::condition_variable                                  m_stopped_cv;

		std::unordered_map<std::size_t, std::shared_ptr<asio::steady_timer>> m_timers;

	};

	using transer     = transmitter_impl;
	using transer_ptr = std::shared_ptr<transmitter_impl>;
}

#endif // !__ASIO2_TRANSMITTER_IMPL_HPP__
