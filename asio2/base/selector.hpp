/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_SELECTOR_HPP__
#define __ASIO2_SELECTOR_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <asio2/config.hpp>

#ifdef ASIO2_USE_ASIOSTANDALONE
	#include <asio/asio.hpp>
	#if defined(ASIO2_USE_SSL)
		#include <asio/ssl.hpp>
	#endif
#else
	#if defined(ASIO2_USE_HTTP)
		#include <boost/beast.hpp>
	#endif
	#include <boost/asio.hpp>
	#if defined(ASIO2_USE_SSL)
		#include <boost/asio/ssl.hpp>
	#endif
#endif // ASIO2_USE_ASIOSTANDALONE


namespace asio2
{

#ifdef ASIO2_USE_ASIOSTANDALONE

	namespace asio = asio;
	using error_code = asio::error_code;
	using system_error = asio::system_error;

#else

	namespace asio = boost::asio;
	using error_code = boost::system::error_code;
	using system_error = boost::system::system_error;

#endif // ASIO2_USE_ASIOSTANDALONE


}

#endif // !__ASIO2_SELECTOR_HPP__
