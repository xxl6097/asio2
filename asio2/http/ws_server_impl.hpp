/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_WS_SERVER_IMPL_HPP__
#define __ASIO2_WS_SERVER_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/http/http_server_impl.hpp>

namespace asio2
{

	template<class _acceptor_impl_t>
	class ws_server_impl : public http_server_impl<_acceptor_impl_t>
	{
	public:
		/**
		 * @construct
		 */
		ws_server_impl(
			std::shared_ptr<url_parser>                    url_parser_ptr,
			std::shared_ptr<listener_mgr>                  listener_mgr_ptr
		)
			: http_server_impl<_acceptor_impl_t>(url_parser_ptr, listener_mgr_ptr)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~ws_server_impl()
		{
		}

	};

}

#endif

#endif // !__ASIO2_WS_SERVER_IMPL_HPP__
