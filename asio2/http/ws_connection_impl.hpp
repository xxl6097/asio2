/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */

#ifndef __ASIO2_HTTP_WEBSOCKET_CONNECTION_IMPL_HPP__
#define __ASIO2_HTTP_WEBSOCKET_CONNECTION_IMPL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(ASIO2_USE_HTTP)

#include <asio2/base/connection_impl.hpp>

#include <asio2/http/http_request.hpp>
#include <asio2/http/http_response.hpp>
#include <asio2/http/http_file_response.hpp>
#include <asio2/http/ws_msg.hpp>

namespace asio2
{

	class ws_connection_impl : public connection_impl
	{
	public:
		/**
		 * @construct
		 */
		explicit ws_connection_impl(
			std::shared_ptr<url_parser>       url_parser_ptr,
			std::shared_ptr<listener_mgr>     listener_mgr_ptr,
			std::shared_ptr<asio::io_context> send_io_context_ptr,
			std::shared_ptr<asio::io_context> recv_io_context_ptr
		)
			: connection_impl(
				url_parser_ptr,
				listener_mgr_ptr,
				send_io_context_ptr,
				recv_io_context_ptr
			)
			, m_socket(*recv_io_context_ptr)
		{
		}

		/**
		 * @destruct
		 */
		virtual ~ws_connection_impl()
		{
		}

		virtual bool start(bool async_connect = false) override
		{
			this->m_state = state::starting;

			// reset the variable to default status
			this->m_fire_close_is_called.clear(std::memory_order_release);

			try
			{
				// Look up the domain name
				asio::ip::tcp::resolver resolver(*this->m_recv_io_context_ptr);
				auto const results = resolver.resolve(this->m_url_parser_ptr->get_host(), this->m_url_parser_ptr->get_port());

				if (async_connect)
				{
					// Make the connection on the IP address we get from a lookup
					asio::async_connect(this->get_socket(),
						results.begin(), results.end(),
						this->m_recv_strand_ptr->wrap(std::bind(
							&ws_connection_impl::_handle_connect, this,
							std::placeholders::_1,
							shared_from_this()
						)));

					return (this->get_socket().is_open());
				}
				else
				{
					error_code ec;
					asio::connect(this->get_socket(), results.begin(), results.end(), ec);

					_handle_connect(ec, shared_from_this());

					// if error code is not 0,then connect failed,return false
					return (!ec && is_started());
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}

			return false;
		}

		virtual void stop() override
		{
			if (this->m_state >= state::starting)
			{
				auto prev_state = this->m_state;
				this->m_state = state::stopping;

				try
				{
					auto self(shared_from_this());

					// first wait for the send event finished
					this->m_send_strand_ptr->post([this, self, prev_state]()
					{
						// asio don't allow operate the same socket in multi thread,if you close socket in one thread and another thread is 
						// calling socket's async_... function,it will crash.so we must care for operate the socket.when need close the
						// socket ,we use the strand to post a event,make sure the socket's close operation is in the same thread.
						this->m_recv_strand_ptr->post([this, self, prev_state]()
						{
							this->m_stopped_cv.notify_all();

							if (prev_state == state::running)
								_fire_close(get_last_error());

							if (this->m_socket.is_open())
							{
								// Close the WebSocket connection
								error_code ec;
								this->m_socket.close(boost::beast::websocket::close_code::normal, ec);
								set_last_error(ec.value());
							}

							this->m_state = state::stopped;
						});
					});
				}
				catch (std::exception &) {}
			}

			connection_impl::stop();
		}

		/**
		 * @function : check whether the connection is started
		 */
		virtual bool is_started() override
		{
			return ((this->m_state >= state::started) && this->m_socket.is_open());
		}

		/**
		 * @function : check whether the connection is stopped
		 */
		virtual bool is_stopped() override
		{
			return ((this->m_state == state::stopped) && !this->m_socket.is_open());
		}

		/**
		 * @function : send data
		 * note : cannot be executed at the same time in multithreading when "async == false"
		 */
		virtual bool send(const std::shared_ptr<buffer<uint8_t>> & buf_ptr) override
		{
			return this->send(std::make_shared<ws_msg>(buf_ptr->data(), buf_ptr->size()));
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
					this->m_send_strand_ptr->post(std::bind(&ws_connection_impl::_post_send, this,
						shared_from_this(),
						http_msg_ptr
					));
					return true;
				}
				catch (std::exception &) {}
			}
			else if (!this->get_socket().is_open())
			{
				set_last_error((int)errcode::socket_not_ready);
			}
			else
			{
				set_last_error((int)errcode::invalid_parameter);
			}
			return false;
		}

	public:
		/**
		 * @function : get the socket
		 */
		inline asio::ip::tcp::socket::lowest_layer_type & get_socket()
		{
			return this->m_socket.lowest_layer();
		}
		/**
		 * @function : get the stream
		 */
		inline boost::beast::websocket::stream<asio::ip::tcp::socket> & get_stream()
		{
			return this->m_socket;
		}

		/**
		 * @function : get the local address
		 */
		virtual std::string get_local_address() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().local_endpoint().address().to_string();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return std::string();
		}

		/**
		 * @function : get the local port
		 */
		virtual unsigned short get_local_port() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().local_endpoint().port();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return 0;
		}

		/**
		 * @function : get the remote address
		 */
		virtual std::string get_remote_address() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().remote_endpoint().address().to_string();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return std::string();
		}

		/**
		 * @function : get the remote port
		 */
		virtual unsigned short get_remote_port() override
		{
			try
			{
				if (this->get_socket().is_open())
				{
					return this->get_socket().remote_endpoint().port();
				}
			}
			catch (system_error & e)
			{
				set_last_error(e.code().value());
			}
			return 0;
		}

	protected:
		virtual void _handle_connect(const error_code & ec, std::shared_ptr<connection_impl> this_ptr)
		{
			set_last_error(ec.value());

			if (!ec)
			{
				// set port reuse
				this->get_socket().set_option(asio::ip::tcp::acceptor::reuse_address(true));

				// set keeplive
				set_keepalive_vals(this->get_socket());

				// setsockopt SO_SNDBUF from url params
				if (this->m_url_parser_ptr->get_so_sndbuf_size() > 0)
				{
					asio::socket_base::send_buffer_size option(this->m_url_parser_ptr->get_so_sndbuf_size());
					this->get_socket().set_option(option);
				}

				// setsockopt SO_RCVBUF from url params
				if (this->m_url_parser_ptr->get_so_rcvbuf_size() > 0)
				{
					asio::socket_base::receive_buffer_size option(this->m_url_parser_ptr->get_so_rcvbuf_size());
					this->get_socket().set_option(option);
				}

				// Perform the websocket handshake
				this->m_socket.handshake(this->m_url_parser_ptr->get_host(), "/", const_cast<error_code &>(ec));

				this->_handle_handshake(ec, this_ptr);
			}
			else
			{
				this->_fire_connect(ec.value());
			}
		}

		virtual void _handle_handshake(const error_code & ec, std::shared_ptr<connection_impl> & this_ptr)
		{
			set_last_error(ec.value());

			if (!ec)
				this->m_state = state::started;

			this->m_last_active_time = std::chrono::system_clock::now();
			this->m_connect_time     = std::chrono::system_clock::now();

			this->_fire_connect(ec.value());

			// Connect succeeded.
			if (!ec && this->m_state == state::started)
			{
				// to avlid the user call stop in another thread,then it may be m_socket.async_read_some and m_socket.close be called at the same time
				this->m_recv_strand_ptr->post([this, this_ptr]()
				{
					// Connect succeeded. set the keeplive values
					//set_keepalive_vals(this->get_socket());

					// Connect succeeded. post recv request.
					this->_post_recv(std::move(this_ptr), std::make_shared<ws_msg>());
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

				ws_msg * ws_msg_ptr = static_cast<ws_msg *>(http_msg_ptr.get());
				// Read a message into our buffer
				this->m_socket.async_read(*ws_msg_ptr,
					asio::bind_executor(
						*(this->m_recv_strand_ptr),
						std::bind(
							&ws_connection_impl::_handle_recv, this,
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

				if (use_count == http_msg_ptr.use_count())
				{
					this->_post_recv(std::move(this_ptr), std::move(http_msg_ptr));
				}
				else
				{
					this->_post_recv(std::move(this_ptr), std::make_shared<ws_msg>());
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
				http_msg_ptr->_send(this->m_socket, ec);
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
		virtual void _fire_connect(int error)
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

		virtual void _fire_close(int error)
		{
			if (!this->m_fire_close_is_called.test_and_set(std::memory_order_acquire))
			{
				static_cast<http_client_listener_mgr *>(this->m_listener_mgr_ptr.get())->notify_close(error);
			}
		}

	protected:
		boost::beast::websocket::stream<asio::ip::tcp::socket> m_socket;

		/// use to avoid call _fire_close twice
		std::atomic_flag m_fire_close_is_called = ATOMIC_FLAG_INIT;

	};

}

#endif

#endif // !__ASIO2_HTTP_WEBSOCKET_CONNECTION_IMPL_HPP__
