// compile application on linux system can use below command :
// g++ -std=c++11 -lpthread -lrt -ldl all_server.cpp -o main.exe -I ../../ -I /usr/local/include -L /usr/local/lib -lboost_filesystem -lboost_system -lboost_locale -lssl -lcrypto

#include <clocale>
#include <climits>
#include <csignal>
#include <ctime>
#include <locale>
#include <limits>
#include <thread>
#include <chrono>
#include <iostream>

#if defined(_MSC_VER)
#	pragma warning(disable:4996)
#endif

#include <asio2/asio2.hpp>

class user_udp_server_listener : public asio2::udp_server_listener
{
public:
	virtual void on_send(asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr, int error) override
	{
	}
	virtual void on_recv(asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr) override
	{
		session_ptr->send(buf_ptr);
	}
	virtual void on_close(asio2::session_ptr & session_ptr, int error) override
	{
	}
	virtual void on_listen() override
	{
	}
	virtual void on_accept(asio2::session_ptr & session_ptr) override
	{
	}
	virtual void on_shutdown(int error) override
	{
	}
};

class user_tcp_client_listener : public asio2::tcp_client_listener_t<uint8_t>
{
public:
	virtual void on_send(asio2::buffer_ptr & buf_ptr, int error) override
	{
	}
	virtual void on_recv(asio2::buffer_ptr & buf_ptr) override
	{
	}
	virtual void on_close(int error) override
	{
	}
	virtual void on_connect(int error) override
	{
	}
};

volatile bool run_flag = true;

class main_frame
{
public:

	void on_recv(asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr, int num)
	{
		session_ptr->send(buf_ptr);
	}
};


std::size_t g_recvd_total_packet_count = 0;
std::size_t g_recvd_total_bytes = 0;
std::size_t g_session_count = 0;


// head 1 byte <
// len  1 byte content len,not include head and tail,just include the content len
// ...  content
// tail 1 byte >

std::size_t pack_parser(asio2::buffer_ptr & buf_ptr)
{
	if (buf_ptr->size() < 3)
		return asio2::need_more;

	uint8_t * data = buf_ptr->data();
	if (data[0] == '<')
	{
		std::size_t total_len = data[1] + 3;
		if (buf_ptr->size() < total_len)
			return asio2::need_more;
		if (data[total_len - 1] == '>')
			return total_len;
	}

	return asio2::bad_data;
}

int main(int argc, char *argv[])
{
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS_)
	// Detected memory leaks on windows system,linux has't these function
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// the number is the memory leak line num of the vs output window content.
	//_CrtSetBreakAlloc(1640); 
#endif

	std::signal(SIGINT, [](int signal) { run_flag = false; });

	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	//main_frame _main_frame;
	user_tcp_client_listener _user_tcp_client_listener;

	auto res = asio2::http::execute("http://bbs.csdn.net", [](asio2::http_request & req) 
	{
		req.set(boost::beast::http::field::accept_charset, "gbk");
	});
	auto full = res.get_full_string();
	auto body = res.get_body_string();

	// in some situations, the asio2 stop function will block forever,so use a loop with start and stop to check the problem. 
	while (run_flag)
	{
		time_t aclock;
		time(&aclock);                 /* Get time in seconds */
		struct tm * newtime = localtime(&aclock);  /* Convert time to struct */
		char tmpbuf[128] = { 0 };
		strftime(tmpbuf, 128, "%Y-%m-%d %H:%M:%S\n", newtime);
		printf("%s", tmpbuf);

		int i = 0;
		const int client_count = 10;

		const char * url = "http://localhost:8443/DataSvr/api/anchor/AddAnchor?json=%7b%22id%22:4990560701320869680,%22name%22:%22anchor222%22,%22ip%22:%22192.168.0.101%22%7d";
		asio2::http::parser::http_parser_url u;
		asio2::http::parser::http_parser_parse_url(url, std::strlen(url), 0, &u);

		asio2::server http_server("http://*:8443");
		http_server.bind_recv([&http_server](asio2::session_ptr & session_ptr, asio2::http_msg_ptr & http_msg_ptr)
		{
		});
		if(!http_server.start())
			std::printf("start http server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		else
			std::printf("start http server successed : %s - %u\n", http_server.get_listen_address().data(), http_server.get_listen_port());

		//asio2::server tcps_server("tcps://*:9443");
		//tcps_server.bind_recv([](asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr)
		//{
		//});
		//if (!tcps_server.start())
		//	std::printf("start tcps server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		//else
		//	std::printf("start tcps server successed : %s - %u\n", tcps_server.get_listen_address().data(), tcps_server.get_listen_port());

		//asio2::client tcps_client("tcps://127.0.0.1:9443");
		//tcps_client.bind_recv([] (asio2::buffer_ptr & buf_ptr)
		//{
		//});
		//if(!tcps_client.start())
		//	std::printf("start tcps client failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		//else
		//	std::printf("start tcps client successed : %s - %u\n", tcps_client.get_remote_address().data(), tcps_client.get_remote_port());

		//asio2::server tcps_auto_server("tcps://*:9445/auto");
		//tcps_auto_server.bind_recv([](asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr)
		//{
		//});
		//if (!tcps_auto_server.start())
		//	std::printf("start tcps auto server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		//else
		//	std::printf("start tcps auto server successed : %s - %u\n", tcps_auto_server.get_listen_address().data(), tcps_auto_server.get_listen_port());

		//asio2::client tcps_auto_client("tcps://127.0.0.1:9445/auto");
		//tcps_auto_client.bind_recv([] (asio2::buffer_ptr & buf_ptr)
		//{
		//});
		//if(!tcps_auto_client.start())
		//	std::printf("start tcps auto client failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		//else
		//	std::printf("start tcps auto client successed : %s - %u\n", tcps_auto_client.get_remote_address().data(), tcps_auto_client.get_remote_port());

		//asio2::server tcps_pack_server("tcps://*:9447/pack");
		//tcps_pack_server.bind_recv([](asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr)
		//{
		//}).set_pack_parser(std::bind(pack_parser, std::placeholders::_1));
		//if (!tcps_pack_server.start())
		//	std::printf("start tcps pack server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		//else
		//	std::printf("start tcps pack server successed : %s - %u\n", tcps_pack_server.get_listen_address().data(), tcps_pack_server.get_listen_port());

		//asio2::client tcps_pack_client("tcps://127.0.0.1:9447/pack");
		//tcps_pack_client.bind_recv([] (asio2::buffer_ptr & buf_ptr)
		//{
		//}).set_pack_parser(std::bind(pack_parser, std::placeholders::_1));
		//if(!tcps_pack_client.start())
		//	std::printf("start tcps pack client failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		//else
		//	std::printf("start tcps pack client successed : %s - %u\n", tcps_pack_client.get_remote_address().data(), tcps_pack_client.get_remote_port());

		//-----------------------------------------------------------------------------------------
		std::shared_ptr<asio2::server> tcp_auto_server = std::make_shared<asio2::server>(" tcp://*:8088/auto?send_buffer_size=1024k & recv_buffer_size=1024K & pool_buffer_size=1024 & io_context_pool_size=3 ");
		tcp_auto_server->bind_recv([&tcp_auto_server](asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr)
		{
			session_ptr->send(buf_ptr); 
		}).bind_accept([](asio2::session_ptr & session_ptr)
		{
		}).bind_close([](asio2::session_ptr & session_ptr, int error)
		{
		});
		if(!tcp_auto_server->start())
			std::printf("start tcp server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		else
			std::printf("start tcp server successed : %s - %u\n", tcp_auto_server->get_listen_address().data(), tcp_auto_server->get_listen_port());

		std::shared_ptr<asio2::client> tcp_auto_client_ptr[client_count];
		for (i = 0; i < client_count; i++)
		{
			tcp_auto_client_ptr[i] = std::make_shared<asio2::client>("tcp://localhost:8088/auto");
			tcp_auto_client_ptr[i]->bind_recv([](asio2::buffer_ptr & buf_ptr)
			{
			});
			if (!tcp_auto_client_ptr[i]->start(false))
				std::printf("connect to tcp server failed : %d - %s. %d\n", asio2::get_last_error(), asio2::get_last_error_desc().data(), i);
			//else
			//	std::printf("connect to tcp server successed : %s - %u\n", tcp_auto_client[i].get_remote_address().data(), tcp_auto_client[i].get_remote_port());
		}


		//-----------------------------------------------------------------------------------------
		asio2::server tcp_pack_server(" tcp://*:8099/pack?send_buffer_size=1024k & recv_buffer_size=1024K & pool_buffer_size=1024 & io_context_pool_size=3");
		tcp_pack_server.bind_recv([&tcp_pack_server](asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr)
		{
			char * p = (char*)buf_ptr->data();
			std::string s;
			s.resize(buf_ptr->size());
			std::memcpy((void*)s.data(), (const void *)p, buf_ptr->size());
			//std::printf("tcp_pack_server recv : %s\n", s.data());

			int send_len = std::rand() % ((int)buf_ptr->size() / 2);
			int already_send_len = 0;
			while (true)
			{
				session_ptr->send((const uint8_t *)p + already_send_len, (std::size_t)send_len);
				already_send_len += send_len;

				if ((std::size_t)already_send_len >= buf_ptr->size())
					break;

				send_len = std::rand() % ((int)buf_ptr->size() / 2);
				if (send_len + already_send_len > (int)buf_ptr->size())
					send_len = (int)buf_ptr->size() - already_send_len;
			}
			
		}).set_pack_parser(std::bind(pack_parser, std::placeholders::_1));
		if (!tcp_pack_server.start())
			std::printf("start tcp server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		else
			std::printf("start tcp server successed : %s - %u\n", tcp_pack_server.get_listen_address().data(), tcp_pack_server.get_listen_port());

		std::shared_ptr<asio2::client> tcp_pack_client[client_count];
		for (i = 0; i < client_count; i++)
		{
			tcp_pack_client[i] = std::make_shared<asio2::client>("tcp://localhost:8099/pack");
			tcp_pack_client[i]->bind_recv([](asio2::buffer_ptr & buf_ptr)
			{
				char * p = (char*)buf_ptr->data();
				std::string s;
				s.resize(buf_ptr->size());
				std::memcpy((void*)s.data(), (const void *)p, buf_ptr->size());
				//std::printf("tcp_pack_client recv : %s\n", s.data());

			}).bind_send([&tcp_pack_client](asio2::buffer_ptr & buf_ptr, int error)
			{
				//char * p = (char*)buf_ptr->data();
				//std::string s;
				//s.resize(len);
				//std::memcpy((void*)s.data(), (const void*)buf_ptr->data(), len);
				//std::printf("tcp client send : %s\n", s.data());
			}).set_pack_parser(std::bind(pack_parser, std::placeholders::_1));
			if (!tcp_pack_client[i]->start(false))
				std::printf("connect to tcp server failed : %d - %s. %d\n", asio2::get_last_error(), asio2::get_last_error_desc().data(), i);
			//else
			//	std::printf("connect to tcp server successed : %s - %u\n", tcp_pack_client[i].get_remote_address().data(), tcp_pack_client[i].get_remote_port());
		}

		asio2::spin_lock lock;
		//-----------------------------------------------------------------------------------------
		asio2::server udp_server("udp://*:9530/?send_buffer_size=256m&recv_buffer_size=256m&pool_buffer_size=1024");
		//udp_server.bind_listener(std::make_shared<user_udp_server_listener>());
		//udp_server.bind_recv(std::bind(&main_frame::on_recv,&_main_frame,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, 10));
		udp_server.bind_recv([&udp_server,&lock](asio2::session_ptr & session_ptr, asio2::buffer_ptr & buf_ptr)
		{
			//char * p = (char*)buf_ptr->data();
			//*(p + len - 1) = 0;
			//std::printf("udp server recv : %d %s\n", len, p);
			std::lock_guard<asio2::spin_lock> g(lock);
			{
				g_recvd_total_packet_count++;
				g_recvd_total_bytes += buf_ptr->size();

				static std::clock_t s_start = std::clock();
				static std::clock_t c_start = std::clock();

				std::clock_t c_end = std::clock();
				if (c_end - c_start > 3000)
				{
					std::printf("%-5lld %6dms %7lldpackets %9lldbytes < %-5lldpackets/sec %5.2fMB bytes/sec >\n",  
						g_session_count,
						c_end - s_start, 
						g_recvd_total_packet_count, 
						g_recvd_total_bytes, 
						g_recvd_total_packet_count * 1000 / (c_end - s_start),
						(double)g_recvd_total_bytes * (double)1000 / (double)(c_end - s_start) / (double)1024 / (double)1024
						);

					c_start = std::clock();
				}
			}

			session_ptr->send(buf_ptr);
			
			//session_ptr->send("0");
		}).bind_accept([](asio2::session_ptr & session_ptr)
		{
			g_session_count++;
			//static int i = 1;
			//std::printf("udp session enter %2d: %s %d\n", i++, session_ptr->get_remote_address().data(), session_ptr->get_remote_port());
		}).bind_close([](asio2::session_ptr & session_ptr, int error)
		{
			g_session_count--;
		});
		if(!udp_server.start())
			std::printf("start udp server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		else
			std::printf("start udp server successed : %s - %u\n", udp_server.get_listen_address().data(), udp_server.get_listen_port());

		std::shared_ptr<asio2::client> udp_client[client_count];
		for (i = 0; i < client_count; i++)
		{
			udp_client[i] = std::make_shared<asio2::client>("udp://localhost:9530");
			udp_client[i]->bind_recv([&udp_client](asio2::buffer_ptr & buf_ptr)
			{
				//char * p = (char*)buf_ptr->data();
				//*(p + len - 1) = 0;
				//std::printf("udp client recv : %s\n", p);
			}).bind_send([&udp_client](asio2::buffer_ptr & buf_ptr, int error)
			{
				//char * p = (char*)buf_ptr->data();
				//*(p + len - 1) = 0;
				//std::printf("udp client send :%lld %s\n", len, p);
			});;
			if (!udp_client[i]->start(false))
			{
				if (asio2::get_last_error() != 24)
					std::printf("connect to udp server failed : %d - %s.\n %d", asio2::get_last_error(), asio2::get_last_error_desc().data(), i);
			}
			//else
			//	std::printf("connect to udp server successed : %s - %u\n", udp_client[i].get_remote_address().data(), udp_client[i].get_remote_port());
		}


		//-----------------------------------------------------------------------------------------
		std::thread([&]()
		{
			while (run_flag)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}).join();

		std::printf(">> ctrl + c is pressed,prepare exit...\n");
	}

	std::printf(">> leave main \n");

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS_)
	//system("pause");
#endif

	return 0;
};
