/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ASIO2_TCP_SEND_OP_HPP__
#define __ASIO2_TCP_SEND_OP_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <future>
#include <utility>
#include <string_view>

#include <asio2/base/selector.hpp>
#include <asio2/base/error.hpp>
#include <asio2/base/detail/condition_wrap.hpp>
#include <asio2/base/detail/buffer_wrap.hpp>

namespace asio2::detail
{
	template<class derived_t, bool isSession>
	class tcp_send_op
	{
	protected:
		template<typename T>
		struct has_member_dgram
		{
			typedef char(&yes)[1];
			typedef char(&no)[2];

			// this creates an ambiguous &Derived::dgram_ if T has got member dgram_

			struct Fallback { char dgram_; };
			struct Derived : T, Fallback { };

			template<typename U, U>
			struct Check;

			template<typename U>
			static no test(Check<char Fallback::*, &U::dgram_>*);

			template<typename U>
			static yes test(...);

			static constexpr bool value = sizeof(test<Derived>(0)) == sizeof(yes);
		};

	public:
		/**
		 * @constructor
		 */
		tcp_send_op() : derive(static_cast<derived_t&>(*this)) {}

		/**
		 * @destructor
		 */
		~tcp_send_op() = default;

	protected:
		template<class Data, class Callback>
		inline bool _tcp_send(Data& data, Callback&& callback)
		{
			if constexpr (has_member_dgram<derived_t>::value)
			{
				if (derive.dgram_)
				{
					return derive._tcp_send_head(asio::buffer(data), std::forward<Callback>(callback));
				}
			}
			else
			{
				std::ignore = true;
			}

			return derive._tcp_send_body(asio::buffer(data), std::forward<Callback>(callback));
		}

		template<class BufferSequence, class Callback>
		inline bool _tcp_send_head(BufferSequence&& buffer, Callback&& callback)
		{
			std::unique_ptr<std::uint8_t[]> head;

			std::size_t bytes = 0;
			if (buffer.size() > (std::numeric_limits<std::uint16_t>::max)())
			{
				bytes = 9;
				head = std::make_unique<std::uint8_t[]>(bytes);
				head[0] = static_cast<std::uint8_t>(255);
				std::uint64_t size = buffer.size();
				std::memcpy(&head[1], reinterpret_cast<const void*>(&size), sizeof(std::uint64_t));
			}
			else if (buffer.size() > 253)
			{
				bytes = 3;
				head = std::make_unique<std::uint8_t[]>(bytes);
				head[0] = static_cast<std::uint8_t>(254);
				std::uint16_t size = static_cast<std::uint16_t>(buffer.size());
				std::memcpy(&head[1], reinterpret_cast<const void*>(&size), sizeof(std::uint16_t));
			}
			else
			{
				bytes = 1;
				head = std::make_unique<std::uint8_t[]>(bytes);
				head[0] = static_cast<std::uint8_t>(buffer.size());
			}

			auto head_buffer = asio::buffer(reinterpret_cast<void*>(head.get()), bytes);

#if defined(ASIO2_SEND_CORE_ASYNC)
			asio::async_write(derive.stream(), head_buffer, asio::bind_executor(derive.io().strand(),
				make_allocator(derive.wallocator(),
					[this, p = derive.selfptr(),
					head = std::move(head),
					buffer = std::forward<BufferSequence>(buffer),
					callback = std::forward<Callback>(callback)]
			(const error_code& ec, std::size_t bytes_sent) mutable
			{
				set_last_error(ec);

				if (ec)
				{
					callback(ec, 0);

					// must stop, otherwise re-sending will cause header confusion
					if (bytes_sent > 0)
					{
						derive._do_disconnect(ec);
					}

					derive.next_event();
				}
				else
				{
					derive._tcp_send_body(std::move(buffer), std::move(callback));
				}
			})));
			return true;
#else
			error_code ec;
			std::size_t bytes_sent = asio::write(derive.stream(), head_buffer, ec);
			set_last_error(ec);
			if (ec)
			{
				callback(ec, 0);
				// must stop, otherwise re-sending will cause header confusion
				if (bytes_sent > 0)
					derive._do_disconnect(ec);
				return false;
			}
			return derive._tcp_send_body(std::forward<BufferSequence>(buffer), std::forward<Callback>(callback));
#endif
		}

		template<class BufferSequence, class Callback>
		inline bool _tcp_send_body(BufferSequence&& buffer, Callback&& callback)
		{
#if defined(ASIO2_SEND_CORE_ASYNC)
			asio::async_write(derive.stream(), buffer, asio::bind_executor(derive.io().strand(),
				make_allocator(derive.wallocator(),
					[this, p = derive.selfptr(), callback = std::forward<Callback>(callback)]
			(const error_code& ec, std::size_t bytes_sent) mutable
			{
				set_last_error(ec);

				callback(ec, bytes_sent);

				if (ec)
				{
					// must stop, otherwise re-sending will cause body confusion
					derive._do_disconnect(ec);
				}

				derive.next_event();
			})));
			return true;
#else
			error_code ec;
			std::size_t bytes_sent = asio::write(derive.stream(), buffer, ec);
			set_last_error(ec);
			callback(ec, bytes_sent);
			if (ec)
			{
				// must stop, otherwise re-sending will cause header confusion
				derive._do_disconnect(ec);
			}
			return (!bool(ec));
#endif
		}

	protected:
		derived_t & derive;
	};
}

#endif // !__ASIO2_TCP_SEND_OP_HPP__
