# asio2
A open source cross-platform c++ library for network programming based on asio,support for tcp,udp,http,rpc,ssl and so on.

<a href="https://996.icu"><img src="https://img.shields.io/badge/link-996.icu-red.svg" alt="996.icu" /></a>
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)

* Support TCP,UDP,HTTP,WEBSOCKET,RPC,ICMP,SERIAL_PORT;
* Support reliable UDP (based on KCP), support SSL, support loading SSL certificates from memory strings;
* TCP supports data unpacking (automatically unpacking data according to specified delimiters to ensure that the data received by users is a complete data package), and implements the datagram mode of TCP (similar to WEBSOCKET);
* Support windows, linux, 32 bits, 64 bits;
* Dependence on asio (boost::asio or asio standalone). If HTTP functions are required, can only use boost::asio, dependence on C++ 17.
* The demo directory contains a large number of sample projects (projects based on VS2017 creation), and a variety of use methods refer to the sample code.

## TCP:
##### server:
```c++
asio2::tcp_server server;
server.bind_recv([&server](std::shared_ptr<asio2::tcp_session> & session_ptr, std::string_view s)
{
	session_ptr->no_delay(true);

	printf("recv : %u %.*s\n", (unsigned)s.size(), (int)s.size(), s.data());
	session_ptr->send(s); // Asynchronous sending (all sending operations are asynchronous and thread-safe)
	// When sending, a callback function is specified, which is called when the sending is completed.
	// bytes_sent indicates the number of bytes actually sent. Whether there is an error in sending
	// can be obtained by using the asio2::get_last_error() function.
	// session_ptr->send(s, [](std::size_t bytes_sent) {}); 
}).bind_connect([&server](auto & session_ptr)
{
	printf("client enter : %s %u %s %u\n",
		session_ptr->remote_address().c_str(), session_ptr->remote_port(),
		session_ptr->local_address().c_str(), session_ptr->local_port());
	// The session_ptr session can be used to start a timer, which is executed in the data send and recv
	// thread of the session_ptr session. This timer is useful for judging the connection status or other
	// requirements (especially in UDP connectionless protocol, Sometimes it is necessary to use a timer
	// in data processing to delay certain operations, and the timer also needs to be triggered safely in
	// the same thread as data processing.)
	//session_ptr->start_timer(1, std::chrono::seconds(1), []() {});
}).bind_disconnect([&server](auto & session_ptr)
{
	printf("client leave : %s %u %s\n",
		session_ptr->remote_address().c_str(),
		session_ptr->remote_port(), asio2::last_error_msg().c_str());
});
server.start("0.0.0.0", "8080");
//server.start("0.0.0.0", "8080", '\n'); // Automatic unpacking by \n (arbitrary characters can be specified)
//server.start("0.0.0.0", "8080", "\r\n"); // Automatic unpacking by \r\n (arbitrary string can be specified)
//server.start("0.0.0.0", "8080", match_role('#')); // Automatic unpacking according to the rules specified by match_role (see demo code for match_role) (for user-defined protocol unpacking)
//server.start("0.0.0.0", "8080", asio::transfer_exactly(100)); // Receive a fixed 100 bytes at a time
//server.start("0.0.0.0", "8080", asio2::use_dgram); // TCP in datagram mode, no matter how long the data is sent, the whole package data of the corresponding length must be received by both sides.
```
##### client:
```c++
asio2::tcp_client client;
client.bind_connect([&](asio::error_code ec)
{
	if (asio2::get_last_error())
		printf("connect failure : %d %s\n", asio2::last_error_val(), asio2::last_error_msg().c_str());
	else
		printf("connect success : %s %u\n", client.local_address().c_str(), client.local_port());

	client.send("<abcdefghijklmnopqrstovuxyz0123456789>");
}).bind_disconnect([](asio::error_code ec)
{
	printf("disconnect : %d %s\n", asio2::last_error_val(), asio2::last_error_msg().c_str());
}).bind_recv([&](std::string_view sv)
{
	printf("recv : %u %.*s\n", (unsigned)sv.size(), (int)sv.size(), sv.data());

	client.send(sv);
})
	//.bind_recv(on_recv) // Binding global functions
	//.bind_recv(std::bind(&listener::on_recv, &lis, std::placeholders::_1)) // Binding member functions (see demo code for details)
	//.bind_recv(&listener::on_recv, lis) // Bind member functions by reference to lis object (see demo code for details)
	//.bind_recv(&listener::on_recv, &lis) // Bind member functions by pointers to lis object (see demo code for details)
	;
client.async_start("0.0.0.0", "8080"); // Asynchronous connection to server
//client.start("0.0.0.0", "8080"); // Synchronized connection to server
//client.async_start("0.0.0.0", "8080", '\n');
//client.async_start("0.0.0.0", "8080", "\r\n");
//client.async_start("0.0.0.0", "8080", match_role);
//client.async_start("0.0.0.0", "8080", asio::transfer_exactly(100));
//client.start("0.0.0.0", "8080", asio2::use_dgram);
```

## UDP:
##### server:
```c++
asio2::udp_server server;
// ... Binding listener (see demo code)
server.start("0.0.0.0", "8080"); // general UDP
//server.start("0.0.0.0", "8080", asio2::use_kcp); // Reliable UDP
```
##### client:
```c++
asio2::udp_client client;
// ... Binding listener (see demo code)
client.start("0.0.0.0", "8080");
//client.async_start("0.0.0.0", "8080", asio2::use_kcp); // Reliable UDP
```

## RPC:
##### server:
```c++
asio2::rpc_server server;
// ... Binding listener (see demo code)
A a; // For the definition of A, see the demo code
server.bind("add", add); // Binding RPC global functions
server.bind("mul", &A::mul, a); // Binding RPC member functions
server.bind("cat", [&](const std::string& a, const std::string& b) { return a + b; }); // Binding lambda
server.bind("get_user", &A::get_user, a); // Binding member functions (by reference)
server.bind("del_user", &A::del_user, &a); // Binding member functions (by pointer)
//server.start("0.0.0.0", "8080", asio2::use_dgram); // Using TCP datagram mode as the underlying support of RPC communication, the use_dgram parameter must be used when starting the server.
server.start("0.0.0.0", "8080"); // Using websocket as the underlying support of RPC communication(You need to go to the end code of the rcp_server.hpp file and choose to use websocket)
```
##### client:
```c++
asio2::rpc_client client;
// ... Binding listener (see demo code)
//client.start("0.0.0.0", "8080", asio2::use_dgram);
client.start("0.0.0.0", "8080");
asio::error_code ec;
// Synchronized invoke RPC functions
int sum = client.call<int>(ec, std::chrono::seconds(3), "add", 11, 2);
printf("sum : %d err : %d %s\n", sum, ec.value(), ec.message().c_str());
// Asynchronous invocation of RPC function, the first parameter is the callback function, when the call is completed or timeout, the callback function automatically called, if timeout or other errors,
// error codes are stored in ec, where async_call does not specify the result value type, the second parameter of the lambda expression must specify the type.
client.async_call([](asio::error_code ec, int v)
{
	printf("sum : %d err : %d %s\n", v, ec.value(), ec.message().c_str());
}, "add", 10, 20);
// Here async_call specifies the result value type, the second parameter of the lambda expression can be auto type.
client.async_call<int>([](asio::error_code ec, auto v)
{
	printf("sum : %d err : %d %s\n", v, ec.value(), ec.message().c_str());
}, "add", 12, 21);
// Result value is user-defined data type (see demo code for the definition of user type)
user u = client.call<user>(ec, "get_user");
printf("%s %d ", u.name.c_str(), u.age);
for (auto &[k, v] : u.purview)
{
	printf("%d %s ", k, v.c_str());
}
printf("\n");

u.name = "hanmeimei";
u.age = ((int)time(nullptr)) % 100;
u.purview = { {10,"get"},{20,"set"} };
// If the result value of the RPC function is void, then the user callback function has only one parameter.
client.async_call([](asio::error_code ec)
{
}, "del_user", std::move(u));

```

## HTTP:
##### server:
```c++
asio2::http_server server;
server.bind_recv([&](std::shared_ptr<asio2::http_session> & session_ptr, http::request<http::string_body>& req)
{
	// Attempt to send a file to the client when receiving an HTTP request
	{
		// Request path must be absolute and not contain "..".
		if (req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != beast::string_view::npos)
		{
			session_ptr->send(http::make_response(http::status::bad_request, "Illegal request-target"));
			session_ptr->stop(); // Disconnect the connection directly at this point
			return;
		}

		// Build the path to the requested file
		std::string path(req.target().data(), req.target().size());
		path.insert(0, std::filesystem::current_path().string());
		if (req.target().back() == '/')
			path.append("index.html");

		// Attempt to open the file
		beast::error_code ec;
		http::file_body::value_type body;
		body.open(path.c_str(), beast::file_mode::scan, ec);

		// Handle the case where the file doesn't exist
		if (ec == beast::errc::no_such_file_or_directory)
		{
			session_ptr->send(http::make_response(http::status::not_found,
				std::string_view{ req.target().data(), req.target().size() }));
			return;
		}

		// Cache the size since we need it after the move
		auto const size = body.size();

		// Respond to GET request
		http::response<http::file_body> res{
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, req.version()) };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, http::extension_to_mimetype(path));
		res.content_length(size);
		res.keep_alive(req.keep_alive()); 
		res.chunked(true);
		// Specify a callback function when sending
		//session_ptr->send(std::move(res));
		session_ptr->send(std::move(res), [&res](std::size_t bytes_sent)
		{
			auto opened = res.body().is_open(); std::ignore = opened;
			auto err = asio2::get_last_error(); std::ignore = err;
		});
		//session_ptr->send(std::move(res), asio::use_future);
		return;
	}

	std::cout << req << std::endl;
	if (true)
	{
		// use make_response to generate an HTTP response object. The status code 200 indicates that the operation is successful. Suceess is the body part of the HTTP message.
		auto rep = http::make_response(200, "suceess");
		session_ptr->send(rep, []()
		{
			auto err = asio2::get_last_error(); std::ignore = err;
		});
	}
	else
	{
		// You can also send an HTTP standard response string directly
		std::string_view rep =
			"HTTP/1.1 404 Not Found\r\n"\
			"Server: Boost.Beast/181\r\n"\
			"Content-Length: 7\r\n"\
			"\r\n"\
			"failure";
		// test send string sequence, the string will automatically parsed into a standard http request
		session_ptr->send(rep, [](std::size_t bytes_sent)
		{
			auto err = asio2::get_last_error(); std::ignore = err;
		});
	}
});
server.start(host, port);

```
##### client:
```c++
asio2::error_code ec;
auto req1 = http::make_request("http://www.baidu.com/get_user?name=a"); // Generate an HTTP request object through a URL string
auto req2 = http::make_request("GET / HTTP/1.1\r\nHost: 127.0.0.1:8443\r\n\r\n"); // Generating an HTTP request object through HTTP protocol string
req2.set(http::field::timeout, 5000); // Set a timeout for request
auto rep1 = asio2::http_client::execute("http://www.baidu.com/get_user?name=a", ec); // Request a web address directly through the URL string, save the result into rep1, if there are errors, error code is saved in ec
auto rep2 = asio2::http_client::execute("127.0.0.1", "8080", req2); // Send an HTTP request through the IP port and the req2 request object
std::cout << rep2 << std::endl; // Display HTTP request results
std::stringstream ss;
ss << rep2;
std::string result = ss.str(); // In this way, HTTP request results are converted to string
```

##### Refer to demo code for other HTTP usage and WEBSOCKET usage.

## ICMP:
```c++
class ping_test // Simulate the use of ping component in a class object (all other components such as TCP/UDP/HTTP can be used in class objects as well)
{
	asio2::ping ping;
public:
	ping_test() : ping(10) // The constructor passes in 10 times to indicate that it ends after only ping 10 times. -1 indicate infinity.
	{
		ping.timeout(std::chrono::seconds(3)); // Setting the ping timeout
		ping.interval(std::chrono::seconds(1)); // Set the ping interval
		ping.body("0123456789abcdefghijklmnopqrstovuxyz");
		ping.bind_recv(&ping_test::on_recv, this) // Binding member functions of the current class as listeners
			.bind_start(std::bind(&ping_test::on_start, this, std::placeholders::_1)) // It's also a bound member function
			.bind_stop([this](asio::error_code ec) { this->on_stop(ec); }); // Binding lambda
	}
	void on_recv(asio2::icmp_rep& rep)
	{
		if (rep.lag.count() == -1) // If the value of the lag equals -1, the time-out is indicated.
			std::cout << "request timed out" << std::endl;
		else
			std::cout << rep.total_length() - rep.header_length()
			<< " bytes from " << rep.source_address()
			<< ": icmp_seq=" << rep.sequence_number()
			<< ", ttl=" << rep.time_to_live()
			<< ", time=" << std::chrono::duration_cast<std::chrono::milliseconds>(rep.lag).count() << "ms"
			<< std::endl;
	}
	void on_start(asio::error_code ec)
	{
		printf("start : %d %s\n", asio2::last_error_val(), asio2::last_error_msg().c_str());
	}
	void on_stop(asio::error_code ec)
	{
		printf("stop : %d %s\n", asio2::last_error_val(), asio2::last_error_msg().c_str());
	}
	void run()
	{
		if (!ping.start("127.0.0.1"))
			//if (!ping.start("123.45.67.89"))
			//if (!ping.start("stackoverflow.com"))
			printf("start failure : %s\n", asio2::last_error_msg().c_str());
		while (std::getchar() != '\n');
		ping.stop();
		// Statistical information can be output after ping, including packet loss rate, average delay time, etc.
		printf("loss rate : %.0lf%% average time : %lldms\n", ping.plp(),
			std::chrono::duration_cast<std::chrono::milliseconds>(ping.avg_lag()).count());
	}
};
```

## serial port:
##### See the sample code serial port section

## others:
##### timer
```c++
// Timer is provided in the framework, which is very simple to use, as follows:
asio2::timer timer;
// Parameter 1 represents the timer ID, parameter 2 represents the timer interval, and parameter 3 is the timer callback function.
timer.start_timer(1, std::chrono::seconds(1), [&]()
{
	printf("timer 1\n");
	if (true) // Close the timer when certain conditions are met, or anywhere else.
		timer.stop_timer(1);
});
```
##### There are some other auxiliary futures, please feel them in the source code or in the use.
