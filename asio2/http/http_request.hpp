/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_REQUEST_HPP__
#define __ASIO2_HTTP_REQUEST_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/http/http_msg.hpp>

namespace asio2
{

	class http_request : public http_msg, public boost::beast::http::request<boost::beast::http::string_body>
	{
		friend class http_session_impl;
		friend class http_connection_impl;
		friend class ws_connection_impl;

	public:
		/**
		 * @construct
		 */
		http_request()
			: http_msg(http::msg_type::string_request)
			, boost::beast::http::request<boost::beast::http::string_body>()
		{
		}
		explicit http_request(const char * target, boost::beast::http::verb method = boost::beast::http::verb::get, unsigned version = 11)
			: http_msg(http::msg_type::string_request)
			, boost::beast::http::request<boost::beast::http::string_body>(method, target, version)
		{
		}
		explicit http_request(boost::beast::http::request<boost::beast::http::string_body > && req)
			: http_msg(http::msg_type::string_request)
			, boost::beast::http::request<boost::beast::http::string_body>(std::move(req))
		{
		}

		/**
		 * @destruct
		 */
		virtual ~http_request()
		{
		}

		/// get functions
	public:
		virtual unsigned                    version()                               override { return boost::beast::http::request<boost::beast::http::string_body>::version();           }
		virtual void                        version(unsigned v)                     override { return boost::beast::http::request<boost::beast::http::string_body>::version(v);          }
		virtual bool                        keep_alive()                            override { return boost::beast::http::request<boost::beast::http::string_body>::keep_alive();        }
		virtual void                        keep_alive(bool v)                      override { return boost::beast::http::request<boost::beast::http::string_body>::keep_alive(v);       }
		virtual bool                        need_eof()                              override { return boost::beast::http::request<boost::beast::http::string_body>::need_eof();          }
		virtual boost::beast::string_view   host()                                  override
		{
			boost::beast::string_view host;
			auto const iter = this->find(boost::beast::http::field::host);
			if (iter != this->end())
			{
				host = iter->value();
				auto pos = host.find(':');
				if (pos != boost::beast::string_view::npos)
					host.remove_suffix(host.size() - pos - 1);
			}
			return host;
		}
		virtual boost::beast::string_view port() override
		{
			boost::beast::string_view host;
			auto const iter = this->find(boost::beast::http::field::host);
			if (iter != this->end())
			{
				host = iter->value();
				auto pos = host.find(':');
				if (pos == boost::beast::string_view::npos)
					host.remove_prefix(host.size());
				else
					host.remove_prefix(pos + 1);
			}
			return host;
		}

		virtual boost::beast::http::verb    method()                                override { return boost::beast::http::request<boost::beast::http::string_body>::method();            }
		virtual void                        method(boost::beast::http::verb v)      override { return boost::beast::http::request<boost::beast::http::string_body>::method(v);           }
		virtual void                        method(const char * s)                  override { return this->method_string(s);                                                            }
		virtual boost::beast::string_view   method_string()                         override { return boost::beast::http::request<boost::beast::http::string_body>::method_string();     }
		virtual void                        method_string(const char * s)           override 
		{
			std::string m(s);
			std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return std::toupper(c); });
			return boost::beast::http::request<boost::beast::http::string_body>::method_string(m.data());
		}
		virtual boost::beast::string_view   target()                                override { return boost::beast::http::request<boost::beast::http::string_body>::target();            }
		virtual void                        target(const char * v)                  override { return boost::beast::http::request<boost::beast::http::string_body>::target(v);           }

	public:
		virtual boost::beast::http::status  result()                                override { return boost::beast::http::status::unknown;                                               }
		virtual unsigned                    result_int()                            override { return 0;                                                                                 }
		virtual void                        result(boost::beast::http::status )     override {                                                                                           }
		virtual void                        result(unsigned )                       override {                                                                                           }
		virtual boost::beast::string_view   reason()                                override { return boost::beast::string_view();                                                       }
		virtual void                        reason(const char * )                   override {                                                                                           }

	public:
		virtual boost::beast::string_view get_header_val(const char * field) override
		{
			boost::beast::string_view val;
			auto const iter = this->find(field);
			if (iter != this->end())
			{
				val = iter->value();
			}
			return val;
		}
		virtual boost::beast::string_view get_header_val(boost::beast::http::field field) override
		{
			boost::beast::string_view val;
			auto const iter = this->find(field);
			if (iter != this->end())
			{
				val = iter->value();
			}
			return val;
		}
		virtual void for_each_header(std::function<void(boost::beast::http::field name, boost::beast::string_view name_string, boost::beast::string_view value)> callback) override
		{
			auto iter = this->begin();
			for (; iter != this->end(); iter++)
			{
				callback(iter->name(), iter->name_string(), iter->value());
			}
		}

	public:
		virtual std::string get_full_string() override
		{
			std::ostringstream ss;
			ss << (*this);
			return ss.str();
		}

		virtual std::string get_body_string() override
		{
			std::ostringstream ss;
			ss << this->body();
			return ss.str();
		}

	protected:
		virtual void _reset() override
		{
			(*this) = {};
		}

		virtual void _send(asio::ip::tcp::socket & s, error_code & ec) override
		{
			boost::beast::http::write(s, *this, ec);
		}
		virtual void _send(boost::beast::websocket::stream<asio::ip::tcp::socket> &, error_code &) override
		{
		}

	};

	//using request_ptr = std::shared_ptr<http_request>;

	namespace http
	{

		std::shared_ptr<http_request> make_request(const char * target, boost::beast::http::verb method, unsigned version = 11, bool keep_alive = false)
		{
			std::shared_ptr<http_request> request_ptr = std::make_shared<http_request>(target, method, version);
			request_ptr->set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			request_ptr->set(boost::beast::http::field::content_type, "text/html");
			request_ptr->keep_alive(keep_alive);
			request_ptr->prepare_payload();
			return request_ptr;
		}
		std::shared_ptr<http_request> make_request(const char * target, const char * method = "GET", unsigned version = 11, bool keep_alive = false)
		{
			std::string smethod(method);
			std::transform(smethod.begin(), smethod.end(), smethod.begin(), [](unsigned char c) { return std::toupper(c); });
			return make_request(target, boost::beast::http::string_to_verb(smethod), version, keep_alive);
		}

	}

}

#endif

#endif // !__ASIO2_HTTP_REQUEST_HPP__
