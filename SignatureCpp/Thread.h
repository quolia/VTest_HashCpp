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
		shared_ptr<task_queue> _queue;
		bool _is_started;

	public:

		task_thread()
		{
			_is_started = false;
			_queue.reset(new task_queue());
		}

		~task_thread()
		{
			join();
		}

		bool const is_started()
		{
			return _is_started;
		}

		void start(shared_ptr<task_queue>& free_tasks, shared_ptr<exception_control>& ec)
		{
			if (is_started())
			{
				throw exception("The thread has been already started.");
			}

			_thread.reset(new thread(run, ref(_queue), ref(free_tasks), ref(ec)));
			_is_started = true;
		}

		void join()
		{
			if (is_started())
			{
				add_task(nullptr); // Tell thread to exit while-loop. See [1].
				_thread->join();
				_is_started = false;
			}
		}

		void add_task(const shared_ptr<hash_task>& task)
		{
			_queue->add(task);
		}

		void clear_tasks()
		{
			_queue->clear();
		}

	private:

		static void run(shared_ptr<task_queue>& queue, shared_ptr<task_queue>& free_tasks, shared_ptr<exception_control>& ec)
		{
			try
			{
				while (!ec->is_exception())
				{
					auto task = queue->wait_and_get();
					if (!task) // See [1].
					{
						break;
					}

					task->do_task();
					free_tasks->add(task);
				}
			}
			catch (const exception& e)
			{
				ec->set_exception(e);
			}
		}
	};

	class task_threads
	{
		vector<shared_ptr<task_thread>> _threads;

	public:

		~task_threads()
		{
			join();
		}

		void init(unsigned count)
		{
			join();

			_threads.clear();
			_threads.reserve(count);

			for (unsigned i = 0; i < count; ++i)
			{
				shared_ptr<task_thread> ptr(new task_thread);
				_threads.push_back(ptr);
			}
		}

		void start(shared_ptr<task_queue>& free_tasks, shared_ptr<exception_control>& ec)
		{
			for (auto& thread : _threads)
			{
				thread->start(free_tasks, ec);
			}
		}

		void join()
		{
			for (auto& thread : _threads)
			{
				thread->join();
			}
		}

		void add_task(unsigned thread_number, const shared_ptr<hash_task>& task)
		{
			if (_threads.empty())
			{
				throw exception("None threads have been created yet.");
			}

			_threads[thread_number % _threads.size()]->add_task(task);
		}
	};
}

#endif // ! _VHASHCPP_THREAD_H_