/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_WEBSOCKET_MSG_HPP__
#define __ASIO2_HTTP_WEBSOCKET_MSG_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/http/http_msg.hpp>

namespace asio2
{

	class ws_msg : public http_msg, public boost::beast::multi_buffer
	{
		friend class http_session_impl;
		friend class ws_session_impl;
		friend class httpws_session_impl;
		friend class http_connection_impl;
		friend class ws_connection_impl;

	public:
		/**
		 * @construct
		 */
		ws_msg()
			: http_msg(http::msg_type::ws_msg)
			, boost::beast::multi_buffer()
		{
		}
		explicit ws_msg(const char * msg)
			: http_msg(http::msg_type::ws_msg)
			, boost::beast::multi_buffer()
		{
			std::size_t len = std::strlen(msg);
			this->commit(asio::buffer_copy(this->prepare(len), asio::buffer(msg, len)));
			//boost::beast::multi_buffer b;
			//boost::beast::ostream(b) << "Hello, world!\n";
		}
		explicit ws_msg(const uint8_t * msg, std::size_t len)
			: http_msg(http::msg_type::ws_msg)
			, boost::beast::multi_buffer()
		{
			this->commit(asio::buffer_copy(this->prepare(len), asio::buffer(msg, len)));
		}
		explicit ws_msg(const std::string & msg)
			: http_msg(http::msg_type::ws_msg)
			, boost::beast::multi_buffer()
		{
			this->commit(asio::buffer_copy(this->prepare(msg.size()), asio::buffer(msg)));
		}

		/**
		 * @destruct
		 */
		virtual ~ws_msg()
		{
		}

		/// get functions
	private:
		virtual unsigned                    version()                               override { return 0;                                                                                 }
		virtual void                        version(unsigned )                      override {                                                                                           }
		virtual bool                        keep_alive()                            override { return false;                                                                             }
		virtual void                        keep_alive(bool )                       override {                                                                                           }
		virtual bool                        need_eof()                              override { return false;                                                                             }
		virtual boost::beast::string_view   host()                                  override
		{
			return boost::beast::string_view();
		}
		virtual boost::beast::string_view port() override
		{
			return boost::beast::string_view();
		}

		virtual boost::beast::http::verb    method()                                override { return boost::beast::http::verb::unknown;                                                 }
		virtual void                        method(boost::beast::http::verb )       override {                                                                                           }
		virtual boost::beast::string_view   method_string()                         override { return boost::beast::string_view();                                                       }
		virtual void                        method_string(const char * )            override {                                                                                           }
		virtual boost::beast::string_view   target()                                override { return boost::beast::string_view();                                                       }
		virtual void                        target(const char * )                   override {                                                                                           }

		virtual boost::beast::http::status  result()                                override { return boost::beast::http::status::unknown;                                               }
		virtual unsigned                    result_int()                            override { return 0;                                                                                 }
		virtual void                        result(boost::beast::http::status )     override {                                                                                           }
		virtual void                        result(unsigned )                       override {                                                                                           }
		virtual boost::beast::string_view   reason()                                override { return boost::beast::string_view();                                                       }
		virtual void                        reason(const char * )                   override {                                                                                           }

	private:
		virtual boost::beast::string_view get_header_val(const char * ) override
		{
			return boost::beast::string_view();
		}
		virtual boost::beast::string_view get_header_val(boost::beast::http::field ) override
		{
			return boost::beast::string_view();
		}
		virtual void for_each_header(std::function<void(boost::beast::http::field name, boost::beast::string_view name_string, boost::beast::string_view value)> ) override
		{
		}

	public:
		virtual std::string get_full_string() override
		{
			std::ostringstream ss;
			ss << boost::beast::buffers(this->data());
			return ss.str();
		}

		virtual std::string get_body_string() override
		{
			std::ostringstream ss;
			ss << boost::beast::buffers(this->data());
			return ss.str();
		}

	protected:
		virtual void _reset() override
		{
			(*this) = {};
		}

		virtual void _send(asio::ip::tcp::socket &, error_code &) override
		{
		}
		virtual void _send(boost::beast::websocket::stream<asio::ip::tcp::socket> & s, error_code & ec) override
		{
			s.write(this->data(), ec);
		}

	};

	namespace ws
	{

		std::shared_ptr<ws_msg> make_response(const char * msg)
		{
			std::shared_ptr<ws_msg> response_ptr = std::make_shared<ws_msg>(msg);
			return response_ptr;
		}

		std::shared_ptr<ws_msg> make_response(const std::string & msg)
		{
			std::shared_ptr<ws_msg> response_ptr = std::make_shared<ws_msg>(msg);
			return response_ptr;
		}

	}

}

#endif

#endif // !__ASIO2_HTTP_WEBSOCKET_MSG_HPP__
