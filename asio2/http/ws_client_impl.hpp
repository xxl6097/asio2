/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_WS_CLIENT_IMPL_HPP__
#define __ASIO2_WS_CLIENT_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/http/http_client_impl.hpp>

namespace asio2
{

	template<class _connection_impl_t>
	class ws_client_impl : public http_client_impl<_connection_impl_t>
	{
	public:
		/**
		 * @construct
		 */
		explicit ws_client_impl(
			std::shared_ptr<url_parser>   url_parser_ptr,
			std::shared_ptr<listener_mgr> listener_mgr_ptr
		)
			: http_client_impl<_connection_impl_t>(url_parser_ptr, listener_mgr_ptr)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~ws_client_impl()
		{
		}

	};

}

#endif

#endif // !__ASIO2_WS_CLIENT_IMPL_HPP__
