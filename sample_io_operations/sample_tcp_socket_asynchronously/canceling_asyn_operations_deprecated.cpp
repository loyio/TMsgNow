/*
MIT License

Copyright (c) 2022 Loyio Hex

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 */

#include <boost/predef.h>

#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501

#if _WIN32_WINNT <= 0x0502
#define BOOST_ASIO_DISABLE_IOCP
#define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <boost/asio.hpp>
#include <iostream>
#include <thread>

using namespace boost;

int main(){
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 3333;

	try{
		asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address), port_num);
		asio::io_service ios;

		std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(ios, ep.protocol()));

		sock->async_connect(ep, [sock] (const boost::system::error_code& ec){
				if(ec.value() != 0){
					if(ec == asio::error::operation_aborted) {
						std::cout << "Operation cancelled!";
					}else{
						std::cout << "Error occured! Error code = " << ec.value() << ". Message: " << ec.message();
					}
					return;
				}
		});

		std::thread worker_thread([&ios](){
				try{
					ios.run();
				}
				catch(system::system_error &e){
					std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
				}
		});

		// Emulating delay
		std::this_thread::sleep_for(std::chrono::seconds(2));

		sock->cancel();

		worker_thread.join();
	}
	catch(system::system_error &e){
		std::cout << "Error occured! Error code = " << e.code()  << ". Message: " << e.what();
		return e.code().value();
	}

	return 0;
}
