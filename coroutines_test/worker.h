/*
 * worker.h
 *
 *  Created on: Aug 22, 2013
 *      Author: ykirpichev
 */

#ifndef WORKER_H_
#define WORKER_H_


#include <thread>
#include <functional>

#include "threadsafe_queue.h"

template<typename worker, typename worker_data, typename worker_cb>
class WorkingThread
{
	typedef enum
	{
		WORK,
		END_WORK,
	} workType_t;

	std::thread t;
	struct work {
		std::function<worker> work_cb;
		std::function<worker_cb> ready_cb;
		worker_data data;
		workType_t type;
		work(workType_t t = END_WORK,
				worker_data data = NULL,
				std::function<worker> wcb = NULL,
				std::function<worker_cb> rcb = NULL):
					work_cb(wcb), ready_cb(rcb), data(data), type(t) {
		}
	};

	threadsafe_queue<work> wq;

public:
	WorkingThread() {

	}
	virtual ~WorkingThread() {
	}

	void post_work(worker_data data,
			std::function<worker> wcb,
			std::function<worker_cb> rcb)
	{
		wq.push(work(WORK, data, wcb, rcb));
	}

	void stop_worker()
	{
		// tell thread to break
		wq.push(work(END_WORK));
	}

	void wait_worker()
	{
	    if (t.joinable())
	    {
	        t.join();
	    }
	}

	void worker_main_loop()
	{
		work entry;

		while (true)
		{
			try
			{
				wq.wait_and_pop(entry);


				if (entry.type == END_WORK)
				{
					// cout << "workThread: END_WORK chunk was detected" << endl;
					break;
				}

				entry.work_cb(entry.data);

				entry.ready_cb(entry.data, 0);
				entry.data.reset();
			}
			catch (std::exception &e)
			{
				std::cout << "exception in working queue: " << e.what() << std::endl;
				entry.ready_cb(entry.data, -1);
				entry.data.reset();
				// todo: may be do not exit the loop here
				break;
			}
		}
	}

	void start_worker()
	{
		try {
			t = std::thread(std::bind(&WorkingThread::worker_main_loop, this));
		} catch (std::exception &e) {
			std::cout << "failed to create thread: " << e.what() << std::endl;
			throw;
		}
	}
};


#endif /* WORKER_H_ */
