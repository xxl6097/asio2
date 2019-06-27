/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_UTIL_HPP__
#define __ASIO2_HTTP_UTIL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/http/http_request.hpp>
#include <asio2/http/http_response.hpp>
#include <asio2/http/http_file_response.hpp>
#include <asio2/http/ws_msg.hpp>

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable:4996)
#endif

namespace asio2
{
	namespace http
	{

		bool url_decode(std::string & url)
		{
			std::size_t index = 0, size = url.size();
			for (std::size_t i = 0; i < size; ++i)
			{
				if (url[i] == '%')
				{
					if (i + 3 <= size)
					{
						int value = 0;
						std::istringstream is(url.substr(i + 1, 2));
						if (is >> std::hex >> value)
						{
							url[index++] = static_cast<char>(value);
							i += 2;
						}
						else
							return false;
					}
					else
						return false;
				}
				else if (url[i] == '+')
					url[index++] = ' ';
				else
					url[index++] = url[i];
			}
			url.resize(index);
			return true;
		}

		template<typename _message>
		std::string get_full_string(_message & message)
		{
			std::ostringstream oss;
			oss << message;
			return oss.str();
		}

		template<typename _message>
		std::string get_body_string(_message & message)
		{
			std::ostringstream oss;
			oss << message.body();
			return oss.str();
		}

		http_response execute(const char * url, boost::beast::http::verb method, unsigned version, const std::function<void(http_request & req)> & request_callback = nullptr)
		{
			http_response response;
			response.set_msg_type();
			response.result(boost::beast::http::status::unknown);

			parser::http_parser_url u;
			if (0 != parser::http_parser_parse_url(url, std::strlen(url), 0, &u))
			{
				set_last_error((int)errcode::invalid_parameter);
				return response;
			}

			/* <scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<fragment> */

			std::string schema, userinfo, host, port("80"), path, query, fragment;
			std::string target;

			if (u.field_set & (1 << parser::UF_SCHEMA))
			{
				schema.resize(u.field_data[parser::UF_SCHEMA].len, '\0');
				strncpy((char *)schema.data(), url + u.field_data[parser::UF_SCHEMA].off, u.field_data[parser::UF_SCHEMA].len);
			}
			if (u.field_set & (1 << parser::UF_HOST))
			{
				host.resize(u.field_data[parser::UF_HOST].len, '\0');
				strncpy((char *)host.data(), url + u.field_data[parser::UF_HOST].off, u.field_data[parser::UF_HOST].len);
			}
			if (u.field_set & (1 << parser::UF_USERINFO))
			{
				userinfo.resize(u.field_data[parser::UF_USERINFO].len, '\0');
				strncpy((char *)userinfo.data(), url + u.field_data[parser::UF_USERINFO].off, u.field_data[parser::UF_USERINFO].len);
				host.insert(0, "@");
				host.insert(0, userinfo);
			}
			if (u.field_set & (1 << parser::UF_PORT))
			{
				port = std::to_string(u.port);
			}
			if (u.field_set & (1 << parser::UF_PATH))
			{
				path.resize(u.field_data[parser::UF_PATH].len, '\0');
				strncpy((char *)path.data(), url + u.field_data[parser::UF_PATH].off, u.field_data[parser::UF_PATH].len);
				target += path;
			}
			if (u.field_set & (1 << parser::UF_QUERY))
			{
				query.resize(u.field_data[parser::UF_QUERY].len, '\0');
				strncpy((char *)query.data(), url + u.field_data[parser::UF_QUERY].off, u.field_data[parser::UF_QUERY].len);
				target += '?';
				target += query;
			}
			if (u.field_set & (1 << parser::UF_FRAGMENT))
			{
				fragment.resize(u.field_data[parser::UF_FRAGMENT].len, '\0');
				strncpy((char *)fragment.data(), url + u.field_data[parser::UF_FRAGMENT].off, u.field_data[parser::UF_FRAGMENT].len);
				target += '#';
				target += fragment;
			}

			if (target.empty()) target = "/";

			try
			{
				// The io_context is required for all I/O
				asio::io_context ioc;

				// These objects perform our I/O
				asio::ip::tcp::resolver resolver{ ioc };
				asio::ip::tcp::socket socket{ ioc };

				// Look up the domain name
				auto const results = resolver.resolve(host, port);

				// Make the connection on the IP address we get from a lookup
				asio::connect(socket, results.begin(), results.end());

				// Set up an HTTP GET request message
				http_request req{ target.data(), method, version };
				req.set(boost::beast::http::field::host, host);
				req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

				if (request_callback) request_callback(req);

				// Send the HTTP request to the remote host
				boost::beast::http::write(socket, req);

				// This buffer is used for reading and must be persisted
				//boost::beast::flat_buffer buffer;
				boost::beast::multi_buffer buffer;

				// Declare a container to hold the response
				//boost::beast::http::response<boost::beast::http::dynamic_body> res;

				// Receive the HTTP response
				boost::beast::http::read(socket, buffer, response);
				//boost::beast::http ::async_read(socket, buffer, response, [](const error_code & err, std::size_t bytes_recvd)
				//{
				//	set_last_error(err.value());
				//	std::ignore = bytes_recvd;
				//	// Reading completed, assign the read the result to ec
				//	// If the code does not execute into here, the ec value is the default value timed_out.
				//});

				//// timedout run
				//ioc.run_one_for(std::chrono::milliseconds(5000));

				response.set_msg_type(msg_type::string_response);

				// Gracefully close the socket
				error_code ec;
				socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
				socket.close(ec);
			}
			catch (system_error & e)
			{
				response.body() = e.what();
				response.prepare_payload();
				set_last_error(e.code().value());
			}

			return response;
		}

		http_response execute(const char * url, const char * method = "get", unsigned version = 11, const std::function<void(http_request & req)> & request_callback = nullptr)
		{
			std::string m(method);
			std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return std::toupper(c); });
			return execute(url, boost::beast::http::string_to_verb(m.data()), version, request_callback);
		}

		http_response execute(const char * url, const std::function<void(http_request & req)> & request_callback)
		{
			return execute(url, boost::beast::http::verb::get, 11, request_callback);
		}

	}
}

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif

#endif // !__ASIO2_HTTP_UTIL_HPP__
