
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio.hpp>
#include <boost/coroutine/all.hpp>
#include <iostream>
#include <memory>


#include "asyncreadstream.h"
using boost::asio::ip::tcp;
using boost::asio::posix::stream_descriptor;
using namespace std::placeholders;
using boost::asio::ip::tcp;

typedef boost::coroutines::coroutine< int(int) > coro_t;

//class file_session : public std::enable_shared_from_this<file_session>{
//public:
//	explicit file_session(stream_descriptor descriptor) :
//		descriptor_(std::move(descriptor)), strand_(descriptor_.get_io_service()){
//
//	}
//
//
//	void read() {
//		auto self(shared_from_this());
//		std::cout << __PRETTY_FUNCTION__ << std::endl;
//		boost::asio::spawn(strand_,
//				[this, self](boost::asio::yield_context yield)
//				{
//					try
//					{
//						char data[128];
//						for (;;)
//						{
//							AsyncReadStream stream(strand_.get_io_service());
//							std::vector<char> data(10);
//
//							stream.async_read(std::make_shared<std::vector<char>>(std::move(data)), yield);
//
//							std::cout << "after stream.async_read " << std::endl;
//
//							std::size_t n = descriptor_.async_read_some(boost::asio::buffer(data), yield);
//							std::cout << "data" << std::endl;
//							boost::asio::async_write(descriptor_, boost::asio::buffer(data, n), yield);
//						}
//					}
//					catch (std::exception& e)
//					{
//						std::cerr << "exception has happened" << e.what() << std::endl;
//						descriptor_.close();
//					}
//				});
//	}
//
//private:
//	stream_descriptor descriptor_;
//	boost::asio::io_service::strand strand_;
//};

//class session: public std::enable_shared_from_this<session> {
//public:
//	explicit session(tcp::socket socket) :
//			socket_(std::move(socket)), timer_(socket_.get_io_service()), strand_(
//					socket_.get_io_service()) {
//	}
//
//	void go() {
//		auto self(shared_from_this());
//		boost::asio::spawn(strand_,
//				[this, self](boost::asio::yield_context yield)
//				{
//					try
//					{
//						char data[128];
//						for (;;)
//						{
//							timer_.expires_from_now(std::chrono::seconds(10));
//							std::size_t n = socket_.async_read_some(boost::asio::buffer(data), yield);
//							boost::asio::async_write(socket_, boost::asio::buffer(data, n), yield);
//						}
//					}
//					catch (std::exception& e)
//					{
//						socket_.close();
//						timer_.cancel();
//					}
//				});
//
//		boost::asio::spawn(strand_,
//				[this, self](boost::asio::yield_context yield)
//				{
//					while (socket_.is_open())
//					{
//						boost::system::error_code ignored_ec;
//						timer_.async_wait(yield[ignored_ec]);
//						if (timer_.expires_from_now() <= std::chrono::seconds(0))
//						socket_.close();
//					}
//				});
//	}
//
//private:
//	tcp::socket socket_;
//	boost::asio::steady_timer timer_;
//	boost::asio::io_service::strand strand_;
//};

class coro_context {
	// what to keep here?
	// async API should work with coro context by self
	// ???
};
//// Result of latest async_read_some operation.
//const boost::system::error_code& error,
//
//// Number of bytes transferred so far.
//std::size_t bytes_transferred
class AsyncReader {
	typedef boost::coroutines::coroutine< void(boost::system::error_code, std::size_t)> coro_t;
    coro_t coro_;
    //coro_t::caller_type& ca_;
    boost::asio::io_service& io_service;
public:
    AsyncReader(boost::asio::io_service& io_service) : io_service(io_service) {
    }

    void start()
    {
        // create and start a coroutine
        // handle_read_() is used as coroutine-function
        coro_ = coro_t(std::bind(&AsyncReader::handle_read_, this, _1));
        std::cout << "AsyncReader::start finished " << std::endl;
    }

    void handle_read_(coro_t::caller_type& yield)
    {
		AsyncReadStream stream(io_service);
		std::vector<char> data(10);
	    boost::asio::io_service::work work(io_service);

		auto coro_resumer = std::bind(&coro_t::operator(), &coro_, _1, _2);

		stream.async_read(std::make_shared<std::vector<char>>(std::move(data)), coro_resumer);
    	yield();

		stream.async_read(std::make_shared<std::vector<char>>(std::move(data)), coro_resumer);
		yield();

		boost::system::error_code ec;
		std::size_t length;

		boost::tie(ec, length) = yield.get();
		std::cout << "after stream.async_read ec = " << ec << " : length = " << length << std::endl;

		tcp::socket s(io_service);
		tcp::resolver resolver(io_service);
		boost::asio::async_connect(s, resolver.resolve({"ya.ru", "80"}),
				[&](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator iterator)
				{
					coro_resumer(ec, 0);
				});
		yield();

		boost::asio::async_write(s, boost::asio::buffer("GET /index.html\r\n"),
				coro_resumer);
		yield();

		char reply[1024] = {0};
		boost::asio::async_read(s, boost::asio::buffer(reply, sizeof(reply)), coro_resumer);
		yield();

		boost::tie(ec, length) = yield.get();
		std::cout << "Reply is: " << std::endl;
		std::cout.write(reply, length);
		std::cout << std::endl;
    }
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: echo_server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

//    boost::asio::spawn(io_service,
//        [&](boost::asio::yield_context yield)
//        {
//          tcp::acceptor acceptor(io_service,
//            tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
//
//          for (;;)
//          {
//            boost::system::error_code ec;
//            tcp::socket socket(io_service);
//            acceptor.async_accept(socket, yield[ec]);
//            if (!ec) std::make_shared<session>(std::move(socket))->go();
//          }
//        });
//	stream_descriptor desc(io_service);
//	int posix_handle = fileno(::fopen("test.txt", "r"));
////	desc.assign(posix_handle);
//	auto file_ses = std::shared_ptr<file_session>(new file_session(std::move(desc)));
//	file_ses->read();

    AsyncReader reader(io_service);
    io_service.post(std::bind(&AsyncReader::start, &reader));

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

