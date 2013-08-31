/*
 * asyncreadstream.h
 *
 *  Created on: Aug 22, 2013
 *      Author: ykirpichev
 */

#ifndef ASYNCREADSTREAM_H_
#define ASYNCREADSTREAM_H_

#include <memory>
#include <vector>
#include <functional>
#include <boost/asio.hpp>
#include <boost/coroutine/all.hpp>

#include "worker.h"

using namespace std::placeholders;

class AsyncReadStream
{
	typedef std::shared_ptr<std::vector<char> > SharedData;

	boost::asio::io_service& io_service;
	WorkingThread<void(SharedData), SharedData, void(SharedData, int)> worker;

	typedef std::function<void(boost::system::error_code, std::size_t)> async_result;

public:
    AsyncReadStream(boost::asio::io_service& io_service) : io_service(io_service) {
    	worker.start_worker();
    }

    ~AsyncReadStream() {
    	worker.stop_worker();
    	worker.wait_worker();
    }

    void async_read(SharedData buffer, async_result callback) {

    	worker.post_work(buffer,
    			std::bind(&AsyncReadStream::sync_read, this, _1),
    			std::bind(&AsyncReadStream::post_result, this, _1, _2, callback));
    }

private:
    void sync_read(SharedData buffer) {
    	std::cout << "calling sync_read from separate thread" << std::this_thread::get_id()<< std::endl;
    }

    void resume_coro(async_result callback) {
    	std::cout << "resume current coro in thread" << std::this_thread::get_id()<< std::endl;
    	callback(boost::system::error_code(), 10);
    }

    void post_result(SharedData buffer, int result, async_result callback) {
    	std::cout << "calling post_result from separate thread" << std::this_thread::get_id()<< std::endl;
    	io_service.post(std::bind(&AsyncReadStream::resume_coro, this, callback));
    }
};

#endif /* ASYNCREADSTREAM_H_ */
