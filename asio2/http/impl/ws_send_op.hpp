/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 * 
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef ASIO_STANDALONE

#ifndef __ASIO2_WS_SEND_OP_HPP__
#define __ASIO2_WS_SEND_OP_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <memory>
#include <future>
#include <utility>
#include <string_view>

#include <asio2/base/selector.hpp>
#include <asio2/base/error.hpp>

namespace asio2::detail
{
	template<class derived_t, bool isSession>
	class ws_send_op
	{
	public:
		/**
		 * @constructor
		 */
		ws_send_op() : derive(static_cast<derived_t&>(*this)) {}

		/**
		 * @destructor
		 */
		~ws_send_op() = default;

	protected:
		template<class Data, class Callback>
		inline bool _ws_send(Data& data, Callback&& callback)
		{
#if defined(ASIO2_SEND_CORE_ASYNC)
			derive.ws_stream().async_write(asio::buffer(data), asio::bind_executor(derive.io().strand(),
				make_allocator(derive.wallocator(),
					[this, p = derive.selfptr(), callback = std::forward<Callback>(callback)]
			(const error_code& ec, std::size_t bytes_sent) mutable
			{
				set_last_error(ec);

				callback(ec, bytes_sent);

				derive.next_event();
			})));
			return true;
#else
			error_code ec;
			std::size_t bytes_sent = derive.ws_stream().write(asio::buffer(data), ec);
			set_last_error(ec);
			callback(ec, bytes_sent);
			return (!bool(ec));
#endif
		}

	protected:
		derived_t & derive;
	};
}

#endif // !__ASIO2_WS_SEND_OP_HPP__

#endif
