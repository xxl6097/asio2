/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ASIO2_SEND_COMPONENT_HPP__
#define __ASIO2_SEND_COMPONENT_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <future>
#include <queue>
#include <tuple>
#include <utility>
#include <string_view>

#include <asio2/base/selector.hpp>
#include <asio2/base/iopool.hpp>
#include <asio2/base/error.hpp>

#include <asio2/base/detail/util.hpp>
#include <asio2/base/detail/function_traits.hpp>
#include <asio2/base/detail/buffer_wrap.hpp>

#include <asio2/base/component/data_persistence_cp.hpp>

namespace asio2::detail
{
	template<class derived_t, bool isSession>
	class send_cp : public data_persistence_cp<derived_t>
	{
		template <class>               friend class data_persistence_cp;
		template <class>               friend class event_queue_cp;

	public:
		/**
		 * @constructor
		 */
		send_cp(io_t&) : derive(static_cast<derived_t&>(*this)) {}

		/**
		 * @destructor
		 */
		~send_cp() = default;

	public:
		/**
		 * @function : Asynchronous send data,supporting multi data formats,see asio::buffer(...) in /asio/buffer.hpp
		 * You can call this function on the communication thread and anywhere,it's multi thread safed.
		 * use like this : std::string m; send(std::move(m)); can reducing memory allocation.
		 * PodType * : send("abc");
		 * PodType (&data)[N] : double m[10]; send(m);
		 * std::array<PodType, N> : std::array<int,10> m; send(m);
		 * std::vector<PodType, Allocator> : std::vector<float> m; send(m);
		 * std::basic_string<Elem, Traits, Allocator> : std::string m; send(m);
		 * We do not provide synchronous send function,because the synchronous send code is very simple,
		 * if you want use synchronous send data,you can do it like this (example):
		 * asio::write(session_ptr->stream(), asio::buffer(std::string("abc")));
		 */
		template<class T>
		inline bool send(T&& data)
		{
			// We must ensure that there is only one operation to send data
			// at the same time,otherwise may be cause crash.
			try
			{
				if (!this->derive.is_started())
					asio::detail::throw_error(asio::error::not_connected);

				this->derive.push_event([this,
					data = this->derive._data_persistence(std::forward<T>(data))]() mutable
				{
					return this->derive._do_send(data, [](const error_code&, std::size_t) {});
				});
				return true;
			}
			catch (system_error & e) { set_last_error(e); }
			catch (std::exception &) { set_last_error(asio::error::eof); }
			return false;
		}

		/**
		 * @function : Asynchronous send data
		 * You can call this function on the communication thread and anywhere,it's multi thread safed.
		 * PodType * : send("abc");
		 * We do not provide synchronous send function,because the synchronous send code is very simple,
		 * if you want use synchronous send data,you can do it like this (example):
		 * asio::write(session_ptr->stream(), asio::buffer(std::string("abc")));
		 */
		template<class CharT, class Traits = std::char_traits<CharT>>
		inline typename std::enable_if_t<
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, wchar_t> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char16_t> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char32_t>, bool> send(CharT * s)
		{
			return this->send(s, s ? Traits::length(s) : 0);
		}

		/**
		 * @function : Asynchronous send data
		 * You can call this function on the communication thread and anywhere,it's multi thread safed.
		 * PodType (&data)[N] : double m[10]; send(m,5);
		 * We do not provide synchronous send function,because the synchronous send code is very simple,
		 * if you want use synchronous send data,you can do it like this (example):
		 * asio::write(session_ptr->stream(), asio::buffer(std::string("abc")));
		 */
		template<class CharT, class SizeT>
		inline typename std::enable_if_t<std::is_integral_v<std::remove_cv_t<std::remove_reference_t<SizeT>>>, bool>
			send(CharT * s, SizeT count)
		{
			// We must ensure that there is only one operation to send data
			// at the same time,otherwise may be cause crash.
			try
			{
				if (!this->derive.is_started())
					asio::detail::throw_error(asio::error::not_connected);

				if (!s)
					asio::detail::throw_error(asio::error::invalid_argument);

				this->derive.push_event([this, data = this->derive._data_persistence(s, count)]() mutable
				{
					return this->derive._do_send(data, [](const error_code&, std::size_t) {});
				});
				return true;
			}
			catch (system_error & e) { set_last_error(e); }
			catch (std::exception &) { set_last_error(asio::error::eof); }
			return false;
		}

		/**
		 * @function : Asynchronous send data,supporting multi data formats,see asio::buffer(...) in /asio/buffer.hpp
		 * use like this : std::string m; send(std::move(m)); can reducing memory allocation.
		 * the pair.first save the send result error_code,the pair.second save the sent_bytes.
		 * note : Do not call this function in any listener callback function like this:
		 * auto future = send(msg,asio::use_future); future.get(); it will cause deadlock,the future.get() will
		 * never return.
		 * PodType * : send("abc");
		 * PodType (&data)[N] : double m[10]; send(m);
		 * std::array<PodType, N> : std::array<int,10> m; send(m);
		 * std::vector<PodType, Allocator> : std::vector<float> m; send(m);
		 * std::basic_string<Elem, Traits, Allocator> : std::string m; send(m);
		 */
		template<class T>
		inline std::future<std::pair<error_code, std::size_t>> send(T&& data, asio::use_future_t<> flag)
		{
			// why use copyable_wrapper? beacuse std::promise is moveable-only, but
			// std::function is copyable-only
			std::ignore = flag;
			copyable_wrapper<std::promise<std::pair<error_code, std::size_t>>> promise;
			std::future<std::pair<error_code, std::size_t>> future = promise().get_future();
			try
			{
				if (!this->derive.is_started())
					asio::detail::throw_error(asio::error::not_connected);

				this->derive.push_event([this, data = this->derive._data_persistence(std::forward<T>(data)),
					promise = std::move(promise)]() mutable
				{
					return this->derive._do_send(data, [&promise](const error_code& ec, std::size_t bytes_sent)
					{
						promise().set_value(std::pair<error_code, std::size_t>(ec, bytes_sent));
					});
				});
			}
			catch (system_error & e)
			{
				set_last_error(e);
				promise().set_value(std::pair<error_code, std::size_t>(e.code(), 0));
			}
			catch (std::exception &)
			{
				set_last_error(asio::error::eof);
				promise().set_value(std::pair<error_code, std::size_t>(asio::error::eof, 0));
			}
			return future;
		}

		/**
		 * @function : Asynchronous send data
		 * the pair.first save the send result error_code,the pair.second save the sent_bytes.
		 * note : Do not call this function in any listener callback function like this:
		 * auto future = send(msg,asio::use_future); future.get(); it will cause deadlock,the future.get() will
		 * never return.
		 * PodType * : send("abc");
		 */
		template<class CharT, class Traits = std::char_traits<CharT>>
		inline typename std::enable_if_t<
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, wchar_t> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char16_t> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char32_t>,
			std::future<std::pair<error_code, std::size_t>>>
			send(CharT * s, asio::use_future_t<> flag)
		{
			return this->send(s, s ? Traits::length(s) : 0, std::move(flag));
		}

		/**
		 * @function : Asynchronous send data
		 * the pair.first save the send result error_code,the pair.second save the sent_bytes.
		 * note : Do not call this function in any listener callback function like this:
		 * auto future = send(msg,asio::use_future); future.get(); it will cause deadlock,the future.get() will
		 * never return.
		 * PodType (&data)[N] : double m[10]; send(m,5);
		 */
		template<class CharT, class SizeT>
		inline typename std::enable_if_t<std::is_integral_v<std::remove_cv_t<std::remove_reference_t<SizeT>>>,
			std::future<std::pair<error_code, std::size_t>>>
			send(CharT * s, SizeT count, asio::use_future_t<> flag)
		{
			std::ignore = flag;
			copyable_wrapper<std::promise<std::pair<error_code, std::size_t>>> promise;
			std::future<std::pair<error_code, std::size_t>> future = promise().get_future();
			try
			{
				if (!this->derive.is_started())
					asio::detail::throw_error(asio::error::not_connected);

				if (!s)
					asio::detail::throw_error(asio::error::invalid_argument);

				this->derive.push_event([this, data = this->derive._data_persistence(s, count),
					promise = std::move(promise)]() mutable
				{
					return this->derive._do_send(data, [&promise](const error_code& ec, std::size_t bytes_sent)
					{
						promise().set_value(std::pair<error_code, std::size_t>(ec, bytes_sent));
					});
				});
			}
			catch (system_error & e)
			{
				set_last_error(e);
				promise().set_value(std::pair<error_code, std::size_t>(e.code(), 0));
			}
			catch (std::exception &)
			{
				set_last_error(asio::error::eof);
				promise().set_value(std::pair<error_code, std::size_t>(asio::error::eof, 0));
			}
			return future;
		}

		/**
		 * @function : Asynchronous send data,supporting multi data formats,see asio::buffer(...) in /asio/buffer.hpp
		 * You can call this function on the communication thread and anywhere,it's multi thread safed.
		 * use like this : std::string m; send(std::move(m)); can reducing memory allocation.
		 * PodType * : send("abc");
		 * PodType (&data)[N] : double m[10]; send(m);
		 * std::array<PodType, N> : std::array<int,10> m; send(m);
		 * std::vector<PodType, Allocator> : std::vector<float> m; send(m);
		 * std::basic_string<Elem, Traits, Allocator> : std::string m; send(m);
		 * We do not provide synchronous send function,because the synchronous send code is very simple,
		 * if you want use synchronous send data,you can do it like this (example):
		 * asio::write(session_ptr->stream(), asio::buffer(std::string("abc")));
		 * Callback signature : void() or void(std::size_t bytes_sent)
		 */
		template<class T, class Callback>
		inline typename std::enable_if_t<is_callable_v<Callback>, bool> send(T&& data, Callback&& fn)
		{
			// We must ensure that there is only one operation to send data
			// at the same time,otherwise may be cause crash.
			try
			{
				if (!this->derive.is_started())
					asio::detail::throw_error(asio::error::not_connected);

				this->derive.push_event([this, data = this->derive._data_persistence(std::forward<T>(data)),
					fn = std::forward<Callback>(fn)]() mutable
				{
					return this->derive._do_send(data, [&fn](const error_code&, std::size_t bytes_sent)
					{
						callback_helper::call(fn, bytes_sent);
					});
				});
				return true;
			}
			catch (system_error & e) { set_last_error(e); }
			catch (std::exception &) { set_last_error(asio::error::eof); }
			return false;
		}

		/**
		 * @function : Asynchronous send data
		 * You can call this function on the communication thread and anywhere,it's multi thread safed.
		 * PodType * : send("abc");
		 * We do not provide synchronous send function,because the synchronous send code is very simple,
		 * if you want use synchronous send data,you can do it like this (example):
		 * asio::write(session_ptr->stream(), asio::buffer(std::string("abc")));
		 * Callback signature : void() or void(std::size_t bytes_sent)
		 */
		template<class Callback, class CharT, class Traits = std::char_traits<CharT>>
		inline typename std::enable_if_t<is_callable_v<Callback> && (
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, wchar_t> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char16_t> ||
			std::is_same_v<std::remove_cv_t<std::remove_reference_t<CharT>>, char32_t>), bool> send(CharT * s, Callback&& fn)
		{
			return this->send(s, s ? Traits::length(s) : 0, std::forward<Callback>(fn));
		}

		/**
		 * @function : Asynchronous send data
		 * You can call this function on the communication thread and anywhere,it's multi thread safed.
		 * PodType (&data)[N] : double m[10]; send(m,5);
		 * We do not provide synchronous send function,because the synchronous send code is very simple,
		 * if you want use synchronous send data,you can do it like this (example):
		 * asio::write(session_ptr->stream(), asio::buffer(std::string("abc")));
		 * Callback signature : void() or void(std::size_t bytes_sent)
		 */
		template<class Callback, class CharT, class SizeT>
		inline typename std::enable_if_t<is_callable_v<Callback> &&
			std::is_integral_v<std::remove_cv_t<std::remove_reference_t<SizeT>>>, bool>
			send(CharT * s, SizeT count, Callback&& fn)
		{
			// We must ensure that there is only one operation to send data
			// at the same time,otherwise may be cause crash.
			try
			{
				if (!this->derive.is_started())
					asio::detail::throw_error(asio::error::not_connected);

				if (!s)
					asio::detail::throw_error(asio::error::invalid_argument);

				this->derive.push_event([this, data = this->derive._data_persistence(s, count),
					fn = std::forward<Callback>(fn)]() mutable
				{
					return this->derive._do_send(data, [&fn](const error_code&, std::size_t bytes_sent)
					{
						callback_helper::call(fn, bytes_sent);
					});
				});
				return true;
			}
			catch (system_error & e) { set_last_error(e); }
			catch (std::exception &) { set_last_error(asio::error::eof); }
			return false;
		}

	protected:
		derived_t                     & derive;
	};
}

#endif // !__ASIO2_SEND_COMPONENT_HPP__
