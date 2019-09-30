#pragma once
#ifndef  _VHASHCPP_THREAD_H_
#define _VHASHCPP_THREAD_H_

#include <thread>
#include "Queue.h"
#include "Task.h"
#include "ExceptionControl.h"

namespace VHASHCPP
{
	using namespace std;

	class task_thread
	{
		shared_ptr<thread> _thread;
		task_queue _queue;

	public:

		void start(task_queue& free_tasks, exception_control& ec)
		{
			_thread.reset(new thread(run, ref(_queue), ref(free_tasks), ref(ec)));
		}

		void join()
		{
			add_task(nullptr); // Tell thread to exit while-loop. See [1].
			_thread->join();
		}

		void add_task(const shared_ptr<hash_task> task)
		{
			_queue.add(task);
		}

	private:

		static void run(task_queue& queue, task_queue& free_tasks, exception_control& ec)
		{
			try
			{
				while (!ec.is_exception())
				{
					auto task = queue.wait_and_get();
					if (!task) // See [1].
					{
						break;
					}

					task->do_task();
					free_tasks.add(task);
				}
			}
			catch (const exception& e)
			{
				ec.set_exception(e);
			}
		}
	};
}

#endif // ! _VHASHCPP_THREAD_H_