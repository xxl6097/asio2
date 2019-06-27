/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_CONNECTION_IMPL_HPP__
#define __ASIO2_HTTP_CONNECTION_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/tcp/tcp_connection_impl.hpp>

#include <asio2/http/http_request.hpp>
#include <asio2/http/http_response.hpp>
#include <asio2/http/http_file_response.hpp>
#include <asio2/http/ws_msg.hpp>
#include <asio2/http/http_util.hpp>

namespace asio2
{

	class http_connection_impl : public tcp_connection_impl
	{
	public:
		/**
		 * @construct
		 */
		explicit http_connection_impl(
			std::shared_ptr<url_parser>       url_parser_ptr,
			std::shared_ptr<listener_mgr>     listener_mgr_ptr,
			std::shared_ptr<asio::io_context> send_io_context_ptr,
			std::shared_ptr<asio::io_context> recv_io_context_ptr
		)
			: tcp_connection_impl(
				url_parser_ptr,
				listener_mgr_ptr,
				send_io_context_ptr,
				recv_io_context_ptr
			)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~http_connection_impl()
		{
		}

		virtual bool start(bool async_connect = false) override
		{
			return tcp_connection_impl::start(async_connect);
		}

		virtual void stop() override
		{
			tcp_connection_impl::stop();
		}

		/**
		 * @function : send data
		 * note : cannot be executed at the same time in multithreading when "async == false"
		 */
		virtual bool send(const std::shared_ptr<buffer<uint8_t>> & buf_ptr) override
		{
			error_code ec;
			boost::beast::http::request_parser<boost::beast::http::string_body> parser;
			parser.put(asio::buffer(buf_ptr->data(), buf_ptr->size()), ec);
			if (!ec)
			{
				auto http_msg_ptr = std::make_shared<http_request>(std::move(parser.get()));
				return this->send(http_msg_ptr);
			}
			set_last_error(ec.value());
			return false;
		}
		/**
		 * @function : send data
		 * just used for http protocol
		 */
		virtual bool send(const std::shared_ptr<http_msg> & http_msg_ptr) override
		{
			// We must ensure that there is only one operation to send data at the same time,otherwise may be cause crash.
			if (this->is_started() && http_msg_ptr)
			{
				try
				{
					// must use strand.post to send data.why we should do it like this ? see udp_session._post_send.
					this->m_send_strand_ptr->post(std::bind(&http_connection_impl::_post_send, this,
						this->shared_from_this(),
						http_msg_ptr
					));
					return true;
				}
				catch (std::exception &) {}
			}
			else if (!this->m_socket.is_open())
			{
				set_last_error((int)errcode::socket_not_ready);
			}
			else
			{
				set_last_error((int)errcode::invalid_parameter);
			}
			return false;
		}

	protected:
		virtual void _handle_connect(const error_code & ec, std::shared_ptr<connection_impl> this_ptr) override
		{
			set_last_error(ec.value());

			if (!ec)
				this->m_state = state::started;

			this->m_last_active_time = std::chrono::system_clock::now();
			this->m_connect_time = std::chrono::system_clock::now();

			_fire_connect(ec.value());

			// Connect succeeded.
			if (!ec && this->m_state == state::started)
			{
				// to avlid the user call stop in another thread,then it may be m_socket.async_read_some and m_socket.close be called at the same time
				this->m_recv_strand_ptr->post([this, this_ptr]()
				{
					// Connect succeeded. post recv request.
					this->_post_recv(std::move(this_ptr), std::make_shared<http_response>());
				});

				this->m_state = state::running;
			}
		}

		virtual void _post_recv(std::shared_ptr<connection_impl> this_ptr, std::shared_ptr<http_msg> http_msg_ptr)
		{
			if (is_started())
			{
				// Make the request empty before reading,
				// otherwise the operation behavior is undefined.
				http_msg_ptr->_reset();

				// Receive the HTTP response
				http_response * response = static_cast<http_response *>(http_msg_ptr.get());
				boost::beast::http::async_read(this->m_socket, this->m_buffer, *response,
					asio::bind_executor(
						*(this->m_recv_strand_ptr),
						std::bind(
							&http_connection_impl::_handle_recv, this,
							std::placeholders::_1,
							std::placeholders::_2,
							std::move(this_ptr),
							std::move(http_msg_ptr)
						)));
			}
		}

		virtual void _handle_recv(const error_code & ec, std::size_t bytes_recvd, std::shared_ptr<connection_impl> this_ptr, std::shared_ptr<http_msg> http_msg_ptr)
		{
			std::ignore = bytes_recvd;

			if (!ec)
			{
				// every times recv data,we update the last active time.
				this->reset_last_active_time();

				auto use_count = http_msg_ptr.use_count();

				this->_fire_recv(http_msg_ptr);

				if (http_msg_ptr->need_eof())
				{
					this->stop();
				}
				else
				{
					if (use_count == http_msg_ptr.use_count())
					{
						this->_post_recv(std::move(this_ptr), std::move(http_msg_ptr));
					}
					else
					{
						this->_post_recv(std::move(this_ptr), std::make_shared<http_response>());
					}
				}
			}
			else
			{
				set_last_error(ec.value());

				// close this session
				this->stop();
			}

			// No new asynchronous operations are started. This means that all shared_ptr
			// references to the connection object will disappear and the object will be
			// destroyed automatically after this handler returns. The connection class's
			// destructor closes the socket.
		}

		virtual void _post_send(std::shared_ptr<connection_impl> this_ptr, std::shared_ptr<http_msg> http_msg_ptr)
		{
			if (this->is_started())
			{
				error_code ec;
				// Send the HTTP request to the remote host
				//if (this->m_websocket_ptr)
				//{
				//	http_msg_ptr->_send(*this->m_websocket_ptr, ec);
				//}
				//else
				{
					http_msg_ptr->_send(this->m_socket, ec);
				}
				set_last_error(ec.value());
				this->_fire_send(http_msg_ptr, ec.value());
				if (ec)
				{
					ASIO2_DUMP_EXCEPTION_LOG_IMPL;
					this->stop();
				}
				
			}
			else
			{
				set_last_error((int)errcode::socket_not_ready);
				this->_fire_send(http_msg_ptr, get_last_error());
			}
		}

		/// must override all listener functions,and cast the m_listener_mgr_ptr to http_server_listener_mgr,
		/// otherwise it will crash when these listener was called.
	protected:
		virtual void _fire_connect(int error) override
		{
			static_cast<http_client_listener_mgr *>(this->m_listener_mgr_ptr.get())->notify_connect(error);
		}

		virtual void _fire_recv(std::shared_ptr<http_msg> & http_msg_ptr)
		{
			static_cast<http_client_listener_mgr *>(this->m_listener_mgr_ptr.get())->notify_recv(http_msg_ptr);
		}

		virtual void _fire_send(std::shared_ptr<http_msg> & http_msg_ptr, int error)
		{
			static_cast<http_client_listener_mgr *>(this->m_listener_mgr_ptr.get())->notify_send(http_msg_ptr, error);
		}

		virtual void _fire_close(int error) override
		{
			if (!this->m_fire_close_is_called.test_and_set(std::memory_order_acquire))
			{
				static_cast<http_client_listener_mgr *>(this->m_listener_mgr_ptr.get())->notify_close(error);
			}
		}

	protected:
		//boost::beast::flat_buffer m_buffer{ 8192 }; // (Must persist between reads)
		boost::beast::multi_buffer m_buffer;

	};

}

#endif

#endif // !__ASIO2_HTTP_CONNECTION_IMPL_HPP__
