/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_FILE_RESPONSE_HPP__
#define __ASIO2_HTTP_FILE_RESPONSE_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/http/http_msg.hpp>

namespace asio2
{

	class http_file_response : public http_msg, public boost::beast::http::response<boost::beast::http::file_body>
	{
		friend class http_session_impl;
		friend class http_connection_impl;
		friend class ws_connection_impl;

	public:
		/**
		 * @construct
		 */
		http_file_response()
			: http_msg(http::msg_type::file_response)
			, boost::beast::http::response<boost::beast::http::file_body>()
		{
		}
		explicit http_file_response(boost::beast::http::status result, unsigned version = 11)
			: http_msg(http::msg_type::file_response)
			, boost::beast::http::response<boost::beast::http::file_body>(result, version)
		{
		}
		explicit http_file_response(boost::beast::http::response<boost::beast::http::file_body > && rep)
			: http_msg(http::msg_type::file_response)
			, boost::beast::http::response<boost::beast::http::file_body>(std::move(rep))
		{
		}

		/**
		 * @destruct
		 */
		virtual ~http_file_response()
		{
		}

		/// get functions
	public:
		virtual unsigned                    version()                               override { return boost::beast::http::response<boost::beast::http::file_body>::version();             }
		virtual void                        version(unsigned v)                     override { return boost::beast::http::response<boost::beast::http::file_body>::version(v);            }
		virtual bool                        keep_alive()                            override { return boost::beast::http::response<boost::beast::http::file_body>::keep_alive();          }
		virtual void                        keep_alive(bool v)                      override { return boost::beast::http::response<boost::beast::http::file_body>::keep_alive(v);         }
		virtual bool                        need_eof()                              override { return boost::beast::http::response<boost::beast::http::file_body>::need_eof();            }
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

	private:
		virtual boost::beast::http::verb    method()                                override { return boost::beast::http::verb::unknown;                                                  }
		virtual void                        method(boost::beast::http::verb )       override {                                                                                            }
		virtual boost::beast::string_view   method_string()                         override { return boost::beast::string_view();                                                        }
		virtual void                        method_string(const char * )            override {                                                                                            }
		virtual boost::beast::string_view   target()                                override { return boost::beast::string_view();                                                        }
		virtual void                        target(const char * )                   override {                                                                                            }

	public:
		virtual boost::beast::http::status  result()                                override { return boost::beast::http::response<boost::beast::http::file_body>::result();              }
		virtual unsigned                    result_int()                            override { return boost::beast::http::response<boost::beast::http::file_body>::result_int();          }
		virtual void                        result(boost::beast::http::status v)    override { return boost::beast::http::response<boost::beast::http::file_body>::result(v);             }
		virtual void                        result(unsigned v)                      override { return boost::beast::http::response<boost::beast::http::file_body>::result(v);             }
		virtual boost::beast::string_view   reason()                                override { return boost::beast::http::response<boost::beast::http::file_body>::reason();              }
		virtual void                        reason(const char * s)                  override { return boost::beast::http::response<boost::beast::http::file_body>::reason(s);             }

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
			//std::ostringstream ss;
			//ss << (*this);
			//return ss.str();
			return std::string();
		}

		virtual std::string get_body_string() override
		{
			//std::ostringstream ss;
			//ss << this->body();
			//return ss.str();
			return std::string();
		}

	protected:
		virtual void _reset() override
		{
			//(*this) = {};
		}

		virtual void _send(asio::ip::tcp::socket & s, error_code & ec) override
		{
			// We need the serializer here because the serializer requires
			// a non-const file_body, and the message oriented version of
			// http::write only works with const messages.
			boost::beast::http::serializer<false, boost::beast::http::file_body, boost::beast::http::fields> sr{ *this };
			boost::beast::http::write(s, sr, ec);
		}
		virtual void _send(boost::beast::websocket::stream<asio::ip::tcp::socket> &, error_code &) override
		{
		}

	};

}

#endif

#endif // !__ASIO2_HTTP_FILE_RESPONSE_HPP__
