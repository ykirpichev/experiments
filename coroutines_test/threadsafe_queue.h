/*
 * threadsafe_queue.h
 *
 *  Created on: Aug 22, 2013
 *      Author: ykirpichev
 */

#ifndef THREADSAFE_QUEUE_H_
#define THREADSAFE_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename Data>
class threadsafe_queue
{
private:
    std::queue<Data> the_queue;
    mutable std::mutex the_mutex;
    typedef std::unique_lock<std::mutex> UniqueLock;
    std::condition_variable the_condition_variable;

public:
    void push(Data const& data)
    {
    	UniqueLock lock(the_mutex);
        the_queue.push(data);
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
    	UniqueLock lock(the_mutex);
        return the_queue.empty();
    }

    bool try_pop(Data& popped_value)
    {
    	UniqueLock lock(the_mutex);
        if(the_queue.empty())
        {
            return false;
        }

        popped_value=the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
    	UniqueLock lock(the_mutex);
        while(the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }

        popped_value=the_queue.front();
        the_queue.pop();
    }

};

#endif /* THREADSAFE_QUEUE_H_ */
