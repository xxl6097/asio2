// compile application on linux system can use below command :
// g++ -std=c++11 -lpthread -lrt -ldl main.cpp -o main.exe -I /usr/local/include -I ../../asio2 -L /usr/local/lib -l boost_system -Wall

#include <clocale>
#include <climits>
#include <csignal>
#include <ctime>
#include <locale>
#include <limits>
#include <thread>
#include <chrono>
#include <iostream>

// http_parser see : 
// http://blog.csdn.net/jiange_zh/article/details/50639178

#if defined(_MSC_VER)
#	pragma warning(disable:4996)
#endif

#include <strstream>
#include <asio2/asio2.hpp>

volatile bool run_flag = true;

#if defined(ASIO2_USE_HTTP)
class main_app : public asio2::http_server_listener
{
public:
	/// construct  
	main_app() : http_server("httpws://*:8080")
	{
		http_server.bind_listener(this);

		if (!http_server.start())
			std::printf("start http server failed : %d - %s.\n", asio2::get_last_error(), asio2::get_last_error_desc().data());
		else
			std::printf("start http server successed : %s - %u\n", http_server.get_listen_address().data(), http_server.get_listen_port());
	}

public:
	virtual void on_send(asio2::session_ptr & session_ptr, asio2::http_msg_ptr & http_msg_ptr, int error) override
	{
	}
	virtual void on_recv(asio2::session_ptr & session_ptr, asio2::http_msg_ptr & http_msg_ptr) override
	{
		auto keep_alive = http_msg_ptr->keep_alive();
		auto host = http_msg_ptr->host();
		auto port = http_msg_ptr->port();
		auto method = http_msg_ptr->method_string();
		auto target = http_msg_ptr->target();
		auto version = http_msg_ptr->version();
		auto full_string = http_msg_ptr->get_full_string();
		auto body_string = http_msg_ptr->get_body_string();
		if (body_string.empty() && http_msg_ptr->method() == asio2::http::verb::get)
			body_string = http_msg_ptr->target().to_string();

		
		session_ptr->start_timer(1, std::chrono::milliseconds(5000), [session_ptr]()
		{
			//session_ptr->stop_timer(1);
			printf("%u enter timer 1\n", session_ptr->get_remote_port());
		});

		if (http_msg_ptr->get_msg_type() == asio2::http::msg_type::ws_msg)
			session_ptr->send(asio2::ws::make_response(body_string.data()));
		else
			session_ptr->send(asio2::http::make_response(asio2::http::status::not_found, body_string.data(), http_msg_ptr->version(), http_msg_ptr->keep_alive()));
		char * s = 0;
	}
	virtual void on_close(asio2::session_ptr & session_ptr, int error) override
	{
		printf("http session leave %s %u %d %s\n", session_ptr->get_remote_address().data(), session_ptr->get_remote_port(), error, asio2::get_error_desc(error).data());
	}
	virtual void on_listen() override
	{
	}
	virtual void on_accept(asio2::session_ptr & session_ptr) override
	{
		printf("http session enter %s %u\n", session_ptr->get_remote_address().data(), session_ptr->get_remote_port());
	}
	virtual void on_shutdown(int error) override
	{
	}

protected:
	asio2::server http_server;

};
#endif

int main(int argc, char *argv[])
{
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS_)
	// Detected memory leaks on windows system,linux has't these function
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// the number is the memory leak line num of the vs output window content.
	//_CrtSetBreakAlloc(1640); 
#endif

	std::signal(SIGINT, [](int signal) { run_flag = false; });

#if defined(ASIO2_USE_HTTP)

	auto res = asio2::http::execute("http://bbs.csdn.net", [](asio2::http_request & req)
	{
		req.set(boost::beast::http::field::accept_charset, "gbk");
	});
	auto full = res.get_full_string();
	auto body = res.get_body_string();

	while (run_flag)
	{
		main_app app;

		//GET / HTTP/1.1
		//Host: 127.0.0.1:8443
		//Connection: keep-alive
		//Cache-Control: max-age=0
		//User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36
		//Upgrade-Insecure-Requests: 1
		//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
		//Accept-Encoding: gzip, deflate, br
		//Accept-Language: zh-CN,zh;q=0.8

		//GET /DataSvr/api/tag/ModTag?id=4506898531877019&name=tag004&peoplename=WangWu2&rfid=rfid001&department=depart001 HTTP/1.1
		//Host: localhost:8443
		//Connection: keep-alive
		//User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36
		//Upgrade-Insecure-Requests: 1
		//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
		//Accept-Encoding: gzip, deflate, br
		//Accept-Language: zh-CN,zh;q=0.8

		//GET /DataSvr/api/anchor/AddAnchor?json=%7b%22id%22:4990560701320869680,%22name%22:%22anchor222%22,%22ip%22:%22192.168.0.101%22%7d HTTP/1.1
		//Host: localhost:8443
		//Connection: keep-alive
		//User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36
		//Upgrade-Insecure-Requests: 1
		//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
		//Accept-Encoding: gzip, deflate, br
		//Accept-Language: zh-CN,zh;q=0.8

		//HTTP/1.1 302 Found
		//Server: openresty
		//Date: Fri, 10 Nov 2017 03:11:50 GMT
		//Content-Type: text/html; charset=utf-8
		//Transfer-Encoding: chunked
		//Connection: keep-alive
		//Keep-Alive: timeout=20
		//Location: http://bbs.csdn.net/home
		//Cache-Control: no-cache
		//
		//5a
		//<html><body>You are being <a href="http://bbs.csdn.net/home">redirected</a>.</body></html>
		//0

		// show the use of http_parser_parse_url function
		//const char * url = "http://localhost:8080/engine/api/user/adduser?json=%7b%22id%22:4990560701320869680,%22name%22:%22admin%22%7d";
		//const char * url = "http://localhost:8080/engine/api/user/adduser?json=[\"id\":4990560701320869680,\"name\":\"admin\"]";
		//const char * url = "http://localhost:8080/engine/api/user/adduser";
		//asio2::http::parser::http_parser_url u;
		//if (0 == asio2::http::parser::http_parser_parse_url(url, strlen(url), 0, &u))
		//{
		//	std::string s;
		//	uint16_t port = 0;
		//	if (u.field_set & (1 << asio2::http::UF_PORT))
		//		port = u.port;
		//	else
		//		port = 80;

		//	if (u.field_set & (1 << asio2::http::UF_SCHEMA))
		//	{
		//		s.resize(u.field_data[asio2::http::UF_SCHEMA].len, '\0');
		//		strncpy((char *)s.data(), url + u.field_data[asio2::http::UF_SCHEMA].off, u.field_data[asio2::http::UF_SCHEMA].len);
		//	}

		//	if (u.field_set & (1 << asio2::http::UF_HOST))
		//	{
		//		s.resize(u.field_data[asio2::http::UF_HOST].len, '\0');
		//		strncpy((char *)s.data(), url + u.field_data[asio2::http::UF_HOST].off, u.field_data[asio2::http::UF_HOST].len);
		//	}

		//	if (u.field_set & (1 << asio2::http::UF_PATH))
		//	{
		//		s.resize(u.field_data[asio2::http::UF_PATH].len, '\0');
		//		strncpy((char *)s.data(), url + u.field_data[asio2::http::UF_PATH].off, u.field_data[asio2::http::UF_PATH].len);
		//	}

		//	if (u.field_set & (1 << asio2::http::UF_QUERY))
		//	{
		//		s.resize(u.field_data[asio2::http::UF_QUERY].len, '\0');
		//		strncpy((char *)s.data(), url + u.field_data[asio2::http::UF_QUERY].off, u.field_data[asio2::http::UF_QUERY].len);
		//	}
		//}

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
#endif

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS_)
	//system("pause");
#endif

	return 0;
};
