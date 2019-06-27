/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_DECL_HPP__
#define __ASIO2_HTTP_DECL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <cstdint>
#include <cstddef>

#include <asio2/base/selector.hpp>
#include <asio2/http/http_parser.h>

namespace asio2
{
	namespace http
	{
		enum class msg_type : int8_t
		{
			unknown,
			string_request,
			string_response,
			file_response,
			ws_msg,
		};

		using status = boost::beast::http::status;
		using field  = boost::beast::http::field;
		using verb   = boost::beast::http::verb;
		using error  = boost::beast::http::error;

	}
}

#endif

#endif // !__ASIO2_HTTP_DECL_HPP__
