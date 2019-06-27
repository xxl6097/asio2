/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_MSG_HPP__
#define __ASIO2_HTTP_MSG_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <memory>
#include <unordered_map>

#include <asio2/base/selector.hpp>

#include <asio2/http/mime_types.hpp>
#include <asio2/http/http_decl.hpp>

namespace asio2
{
	class http_response;

	class http_msg
	{
		friend class http_session_impl;
		friend class ws_session_impl;
		friend class httpws_session_impl;
		friend class http_connection_impl;
		friend class ws_connection_impl;

		friend http_response execute(const char * url, unsigned version);

	public:
		/**
		 * @construct
		 */
		http_msg(http::msg_type msg_type = http::msg_type::unknown) : m_msg_type(msg_type)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~http_msg()
		{
		}

		/// get functions
	public:
		// -- global properties
		virtual unsigned                   version()                             = 0;
		virtual void                       version(unsigned v)                   = 0;
		virtual bool                       keep_alive()                          = 0;
		virtual void                       keep_alive(bool v)                    = 0;
		virtual bool                       need_eof()                            = 0;
		virtual boost::beast::string_view  host()                                = 0;
		virtual boost::beast::string_view  port()                                = 0;
		virtual boost::beast::string_view  path()
		{
			boost::beast::string_view target = this->target();

			// show the use of http_parser_parse_url function
			// const char * url = "http://localhost:8080/engine/api/user/adduser?json=%7b%22id%22:4990560701320869680,%22name%22:%22admin%22%7d";
			// const char * url = "http://localhost:8080/engine/api/user/adduser?json=[\"id\":4990560701320869680,\"name\":\"admin\"]";
			// const char * url = "http://localhost:8080/engine/api/user/adduser";
			http::parser::http_parser_url u;
			if (!target.empty() && 0 == http::parser::http_parser_parse_url(target.data(), target.size(), 0, &u))
			{
				if (u.field_set & (1 << http::parser::UF_PATH))
				{
					return target.substr(u.field_data[http::parser::UF_PATH].off, u.field_data[http::parser::UF_PATH].len);
				}
			}

			return boost::beast::string_view();
		}
		virtual boost::beast::string_view  query()
		{
			boost::beast::string_view target = this->target();

			http::parser::http_parser_url u;
			if (!target.empty() && 0 == http::parser::http_parser_parse_url(target.data(), target.size(), 0, &u))
			{
				if (u.field_set & (1 << http::parser::UF_QUERY))
				{
					return target.substr(u.field_data[http::parser::UF_QUERY].off, u.field_data[http::parser::UF_QUERY].len);
				}
			}

			return boost::beast::string_view();
		}

		// -- request properties
		virtual boost::beast::http::verb   method()                              = 0;
		virtual void                       method(boost::beast::http::verb v)    = 0;
		virtual void                       method(const char * s) { this->method_string(s); }
		virtual boost::beast::string_view  method_string()                       = 0;
		virtual void                       method_string(const char * s)         = 0;
		virtual boost::beast::string_view  target()                              = 0;
		virtual void                       target(const char * v)                = 0;

		// -- response properties
		virtual boost::beast::http::status result()                              = 0;
		virtual unsigned                   result_int()                          = 0;
		virtual void                       result(boost::beast::http::status v)  = 0;
		virtual void                       result(unsigned v)                    = 0;
		virtual boost::beast::string_view  reason()                              = 0;
		virtual void                       reason(const char * s)                = 0;

	public:
		virtual boost::beast::string_view get_header_val(const char * field) = 0;
		virtual boost::beast::string_view get_header_val(boost::beast::http::field field) = 0;
		virtual void for_each_header(std::function<void(boost::beast::http::field name, boost::beast::string_view name_string, boost::beast::string_view value)> callback) = 0;

	public:
		virtual std::string get_full_string() = 0;
		virtual std::string get_body_string() = 0;

		inline http::msg_type get_msg_type() { return this->m_msg_type; }
		inline void set_msg_type(http::msg_type msg_type = http::msg_type::unknown) { this->m_msg_type = msg_type; }

	protected:
		virtual void _reset() = 0;

		virtual void _send(asio::ip::tcp::socket & s, error_code & ec) = 0;
		virtual void _send(boost::beast::websocket::stream<asio::ip::tcp::socket> & s, error_code & ec) = 0;

	protected:
		http::msg_type m_msg_type = http::msg_type::unknown;
	};

	using http_msg_ptr = std::shared_ptr<http_msg>;

}

#endif

#endif // !__ASIO2_HTTP_MSG_HPP__
