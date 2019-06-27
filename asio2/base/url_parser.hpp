/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 * 
 */


#ifndef __ASIO2_URL_PARSER_HPP__
#define __ASIO2_URL_PARSER_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cctype>
#include <cstring>

#include <stdexcept>

#include <string>
#include <memory>
#include <algorithm>
#include <unordered_map>

#include <asio2/base/def.hpp>
#include <asio2/util/helper.hpp>
#include <asio2/base/error.hpp>

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable:4996)
#endif

namespace asio2 
{

	/**
	 * more details see server.hpp
	 */
	class url_parser
	{
	public:
		url_parser(const std::string & url) : m_url(url)
		{
			if (!_parse())
			{
				set_last_error((int)errcode::url_string_invalid);

				_reset();
			}
			else
			{
				_set_so_sndbuf_size_from_url();
				_set_so_rcvbuf_size_from_url();
				_set_send_buffer_size_from_url();
				_set_recv_buffer_size_from_url();
				_set_silence_timeout_from_url();
				_set_max_packet_size();
				_set_packet_header_flag();
				_set_auto_reconnect();
				_set_endpoint_cache();
			}
		}
		virtual ~url_parser()
		{
		}

		std::string get_url()        { return this->m_url     ; }
		std::string get_protocol()   { return this->m_protocol; }
		std::string get_host()       { return this->m_host      ; }
		std::string get_port()       { return this->m_port    ; }
		std::string get_model()      { return this->m_model   ; }

		std::string get_param_value(std::string name)
		{
			auto iter = this->m_params.find(name);
			return ((iter != this->m_params.end()) ? iter->second : std::string());
		}

		template<typename _handler>
		void for_each_param(_handler handler)
		{
			for (auto & pair : this->m_params)
			{
				handler(pair.first, pair.second);
			}
		}

		inline int           get_so_sndbuf_size()     { return this->m_so_sndbuf_size;     }
		inline int           get_so_rcvbuf_size()     { return this->m_so_rcvbuf_size;     }
		inline std::size_t   get_send_buffer_size()   { return this->m_send_buffer_size;   }
		inline std::size_t   get_recv_buffer_size()   { return this->m_recv_buffer_size;   }
		inline long          get_silence_timeout()    { return this->m_silence_timeout;    }
		inline uint8_t       get_packet_header_flag() { return this->m_packet_header_flag; }
		inline uint32_t      get_max_packet_size()    { return this->m_max_packet_size;    }
		inline long          get_auto_reconnect()     { return this->m_auto_reconnect;     }
		inline bool          get_endpoint_cache()     { return this->m_endpoint_cache;     }

		inline url_parser &  set_send_buffer_size(std::size_t send_buffer_size) { this->m_send_buffer_size = send_buffer_size; return (*this); }
		inline url_parser &  set_recv_buffer_size(std::size_t recv_buffer_size) { this->m_recv_buffer_size = recv_buffer_size; return (*this); }
		inline url_parser &  set_silence_timeout (long        silence_timeout ) { this->m_silence_timeout  = silence_timeout;  return (*this); }
		inline url_parser &  set_auto_reconnect  (long        auto_reconnect  ) { this->m_auto_reconnect   = auto_reconnect;   return (*this); }
		inline url_parser &  set_endpoint_cache  (bool        endpoint_cache  ) { this->m_endpoint_cache   = endpoint_cache;   return (*this); }

	protected:
		/**
		 * @function : setsockopt SO_SNDBUF
		 */
		void _set_so_sndbuf_size_from_url()
		{
			auto val = get_param_value("so_sndbuf");
			if (!val.empty())
			{
				int size = std::atoi(val.data());
				if (val.find_last_of('k') != std::string::npos)
					size *= 1024;
				else if (val.find_last_of('m') != std::string::npos)
					size *= 1024 * 1024;
				this->m_so_sndbuf_size = size;
			}
		}

		/**
		 * @function : setsockopt SO_RCVBUF
		 */
		void _set_so_rcvbuf_size_from_url()
		{
			auto val = get_param_value("so_rcvbuf");
			if (!val.empty())
			{
				int size = std::atoi(val.data());
				if (val.find_last_of('k') != std::string::npos)
					size *= 1024;
				else if (val.find_last_of('m') != std::string::npos)
					size *= 1024 * 1024;
				this->m_so_rcvbuf_size = size;
			}
		}

		/**
		 * @function : send_buffer_size for the socket::send function
		 */
		void _set_send_buffer_size_from_url()
		{
			auto val = get_param_value("send_buffer_size");
			if (!val.empty())
			{
				std::size_t size = static_cast<std::size_t>(std::strtoull(val.data(), nullptr, 10));
				if (val.find_last_of('k') != std::string::npos)
					size *= 1024;
				else if (val.find_last_of('m') != std::string::npos)
					size *= 1024 * 1024;
				if (size < 64)
					size = 1024;
				this->m_send_buffer_size = size;
			}
		}

		/**
		 * @function : recv_buffer_size for the socket::recv function
		 */
		void _set_recv_buffer_size_from_url()
		{
			auto val = get_param_value("recv_buffer_size");
			if (!val.empty())
			{
				std::size_t size = static_cast<std::size_t>(std::strtoull(val.data(), nullptr, 10));
				if (val.find_last_of('k') != std::string::npos)
					size *= 1024;
				else if (val.find_last_of('m') != std::string::npos)
					size *= 1024 * 1024;
				if (size < 64)
					size = 1024;
				this->m_recv_buffer_size = size;
			}
		}

		void _set_silence_timeout_from_url()
		{
			auto val = get_param_value("silence_timeout");
			if (!val.empty())
			{
				long size = static_cast<long>(std::atoi(val.data()));
				if (val.find_last_of('m') != std::string::npos)
					size *= 60;
				else if (val.find_last_of('h') != std::string::npos)
					size *= 60 * 60;
				this->m_silence_timeout = size;
			}
			else
			{
				if /**/ (this->m_protocol == "tcp")
					this->m_silence_timeout = ASIO2_DEFAULT_TCP_SILENCE_TIMEOUT;
				else if (this->m_protocol == "tcps")
					this->m_silence_timeout = ASIO2_DEFAULT_TCP_SILENCE_TIMEOUT;
				else if (this->m_protocol == "udp")
					this->m_silence_timeout = ASIO2_DEFAULT_UDP_SILENCE_TIMEOUT;
				else if (this->m_protocol == "http")
					this->m_silence_timeout = ASIO2_DEFAULT_HTTP_SILENCE_TIMEOUT;
				else if (this->m_protocol == "ws")
					this->m_silence_timeout = ASIO2_DEFAULT_HTTP_SILENCE_TIMEOUT;
				else if (this->m_protocol == "httpws")
					this->m_silence_timeout = ASIO2_DEFAULT_HTTP_SILENCE_TIMEOUT;
				else if (this->m_protocol == "https")
					this->m_silence_timeout = ASIO2_DEFAULT_HTTP_SILENCE_TIMEOUT;
			}
		}

		void _set_max_packet_size()
		{
			// set max_packet_size from the url
			auto val = get_param_value("max_packet_size");
			if (!val.empty())
				this->m_max_packet_size = static_cast<uint32_t>(std::atoi(val.data()));
			if (this->m_max_packet_size < 8 || this->m_max_packet_size > ASIO2_MAX_PACKET_SIZE)
				this->m_max_packet_size = ASIO2_MAX_PACKET_SIZE;
		}

		void _set_packet_header_flag()
		{
			// set packet_header_flag from the url
			auto val = get_param_value("packet_header_flag");
			if (!val.empty())
				this->m_packet_header_flag = static_cast<uint8_t>(std::atoi(val.data()));
			if (this->m_packet_header_flag == 0 || this->m_packet_header_flag > ASIO2_MAX_HEADER_FLAG)
				this->m_packet_header_flag = ASIO2_DEFAULT_HEADER_FLAG;
		}

		void _set_auto_reconnect()
		{
			auto val = get_param_value("auto_reconnect");
			if (!val.empty())
			{
				if /**/ (val == "true")
					this->m_auto_reconnect = 0;
				else if (val == "false")
					this->m_auto_reconnect = -1;
				else
				{
					auto interval = std::strtol(val.data(), nullptr, 10);
					if /**/ (val.find("s") != std::string::npos)
						interval *= 1000;
					else if (val.find("m") != std::string::npos)
						interval *= 1000 * 60;
					else if (val.find("h") != std::string::npos)
						interval *= 1000 * 60 * 60;
					this->m_auto_reconnect = interval;
				}
			}
		}

		void _set_endpoint_cache()
		{
			auto val = get_param_value("endpoint_cache");
			if (!val.empty())
			{
				if /**/ (val == "true" || val == "1")
					this->m_endpoint_cache = true;
				else if (val == "false" || val == "0")
					this->m_endpoint_cache = false;
			}
		}

	protected:
		bool _parse()
		{
			if (this->m_url.empty())
			{
				ASIO2_ASSERT(false);
				return false;
			}

			// erase the invalid character : space \t \r \n and so on
			trim_all(this->m_url);

			// tolower
			std::transform(this->m_url.begin(), this->m_url.end(), this->m_url.begin(), [](unsigned char c) { return std::tolower(c); });

			// parse the protocol
			std::size_t pos_protocol_end = this->m_url.find("://", 0);
			if (pos_protocol_end == 0 || pos_protocol_end == std::string::npos)
			{
				ASIO2_ASSERT(false);
				return false;
			}
			this->m_protocol = this->m_url.substr(0, pos_protocol_end - 0);

			// parse the host
			std::size_t pos_host_begin = pos_protocol_end + std::strlen("://");
			std::size_t pos_host_end   = this->m_url.find(':', pos_host_begin);
			if (pos_host_end == std::string::npos)
			{
				ASIO2_ASSERT(false);
				return false;
			}
			this->m_host = this->m_url.substr(pos_host_begin, pos_host_end - pos_host_begin);
			if (this->m_host.empty() || this->m_host == "*")
				this->m_host = "0.0.0.0";
			else if (this->m_host == "localhost")
				this->m_host = "127.0.0.1";

			// parse the port
			std::size_t pos_port_begin = pos_host_end + std::strlen(":");
			std::size_t pos_port_end = this->m_url.find('/', pos_port_begin);
			this->m_port = this->m_url.substr(pos_port_begin, (pos_port_end == std::string::npos) ? std::string::npos : pos_port_end - pos_port_begin);
			if (this->m_port.empty() || this->m_port == "*")
			{
				if /**/ (this->m_protocol == "http")
					this->m_port = "80";
				else if (this->m_protocol == "https")
					this->m_port = "443";
				else
					this->m_port = "0";
			}

			if (pos_port_end != std::string::npos)
			{
				// parse the model
				std::size_t pos_model_begin = pos_port_end + std::strlen("/");
				std::size_t pos_model_end = this->m_url.find('?', pos_model_begin);
				if (pos_model_end != std::string::npos)
					this->m_model = this->m_url.substr(pos_model_begin, pos_model_end - pos_model_begin);
				else
				{
					if (this->m_url.find('=', pos_model_begin) == std::string::npos)
						this->m_model = this->m_url.substr(pos_model_begin);
					else
						pos_model_end = pos_model_begin - 1;
				}

				if (pos_model_end != std::string::npos)
				{
					// parse the params
					std::size_t pos_params_begin = pos_model_end + std::strlen("?");
					if (this->m_url.length() > pos_params_begin)
					{
						std::string params = this->m_url.substr(pos_params_begin);
						if (params[params.length() - 1] != '&')
							params += '&';

						std::size_t pos_sep = 0, pos_head = 0;
						while ((pos_sep = params.find('&', pos_sep)) != std::string::npos)
						{
							if (pos_sep > pos_head)
							{
								std::string param = params.substr(pos_head, pos_sep - pos_head);

								std::size_t pos_equal = param.find('=', 0);
								if (pos_equal > 0 && pos_equal + 1 < param.length())
								{
									std::string name = param.substr(0, pos_equal);
									pos_equal++;
									std::string value = param.substr(pos_equal);
									this->m_params.emplace(name, value);
								}
							}

							pos_sep++;
							pos_head = pos_sep;
						}
					}
				}
			}

			return true;
		}

		void _reset()
		{
			this->m_url.clear();

			this->m_protocol.clear();
			this->m_host.clear();
			this->m_port.clear();
			this->m_model.clear();

			this->m_params.clear();
		}

	protected:
		std::string m_url;

		std::string m_protocol;
		std::string m_host;
		std::string m_port;
		std::string m_model;

		std::unordered_map<std::string, std::string> m_params;

		int                  m_so_sndbuf_size     = 0;
		int                  m_so_rcvbuf_size     = 0;
		volatile std::size_t m_send_buffer_size   = ASIO2_DEFAULT_SEND_BUFFER_SIZE;
		volatile std::size_t m_recv_buffer_size   = ASIO2_DEFAULT_RECV_BUFFER_SIZE;
		volatile long        m_silence_timeout    = 0;
		uint8_t              m_packet_header_flag = ASIO2_DEFAULT_HEADER_FLAG;
		uint32_t             m_max_packet_size    = ASIO2_MAX_PACKET_SIZE;
		volatile long        m_auto_reconnect     = -1;
		volatile bool        m_endpoint_cache     = true;
	};

}

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // !__ASIO2_URL_PARSER_HPP__
