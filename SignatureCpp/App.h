#pragma once
#ifndef  _VHASHCPP_APP_H_
#define _VHASHCPP_APP_H_

#include <iostream>
#include <string>
#include <boost/scoped_array.hpp>
#include "File.h"
#include "CLI.h"
#include "TaskFactory.h"
#include "Queue.h"
#include "Thread.h"

namespace VHASHCPP
{
	using namespace std;

	class application
	{
		bool _is_inited;
		unsigned _chunk_size_bytes;
		unsigned _chunks_in_source_file;
		unsigned _max_free_task_count;
		unsigned long long _max_free_task_memory;

		boost::scoped_array<unsigned char> _hash_file_buff;
		task_queue _free_tasks;

		binary_file _src_file;
		binary_file _hash_file;
		shared_ptr<hash_task> _proto_task;

	public:

		application()
		{
			_is_inited = false;
			_chunk_size_bytes = 0;
			_chunks_in_source_file = 0;
			_max_free_task_count = 0;
			_max_free_task_memory = 0;
		}
		
		long long src_file_size()
		{
			if (!_is_inited)
			{
				throw exception("Application has not been inited yet.");
			}

			return _src_file.size();
		}
		
		void init(int argc, char* argv[], unsigned max_free_task_count, unsigned long long max_free_task_memory)
		{
			_is_inited = false;

			if (max_free_task_count < 1)
			{
				throw exception("Invalid max free tasks count. Must be larger than 0.");
			}

			if (max_free_task_memory < 1)
			{
				throw exception("Invalid max free tasks memory. Must be larger than 0.");
			}

			_max_free_task_count = max_free_task_count;
			_max_free_task_memory = max_free_task_memory;

			command_line cli_params;
			cli_params.init(argc, argv);

			cout << "Source file: " << cli_params.src_file_name() << endl;
			_src_file.open_for_read(cli_params.src_file_name());

			cout << "Source file size, bytes: " << _src_file.size() << endl;
			if (_src_file.size() < 1)
			{
				throw exception("Invalid source file size.");
			}

			cout << "Hash file: " << cli_params.hash_file_name() << endl;
			_hash_file.open_for_write(cli_params.hash_file_name());

			cout << "Source chunk size to hash, MBs: " << cli_params.chunk_size_mb() << endl;

			_chunk_size_bytes = cli_params.chunk_size_mb() * 1024 * 1024;
			cout << "Source chunk size, bytes: " << _chunk_size_bytes << endl;

			_chunks_in_source_file = (unsigned)(_src_file.size() / _chunk_size_bytes);
			if (_src_file.size() % _chunk_size_bytes)
			{
				++_chunks_in_source_file;
			}
			cout << "Chunks in source file: " << _chunks_in_source_file << endl;

			cout << "Hash name: " << cli_params.hash_name() << endl;

			// Base instance from which all other instances will be created. See [2].
			_proto_task = hash_task_factory::new_instance_by_name(cli_params.hash_name());

			unsigned single_hash_size = _proto_task->get_hash_size();
			cout << "Chunk hash size, bytes: " << single_hash_size << endl;
			if (single_hash_size < 1)
			{
				throw exception("Invalid single hash size. Must be larger than 0.");
			}
			cout << "Hash file size, bytes: " << single_hash_size * _chunks_in_source_file << endl;

			allocate_tasks();

			_is_inited = true;
		}

		void run()
		{
			if (!_is_inited)
			{
				throw exception("Application has not been inited yet.");
			}

			unsigned cpu_count = thread::hardware_concurrency();
			cout << "Threads count: " << cpu_count << endl;
			if (cpu_count < 1)
			{
				throw exception("Unable to get CPUs count.");
			}

			exception_control excep_control;
			task_threads threads;
			threads.init(cpu_count);
			threads.start(_free_tasks, excep_control);

			cout << "Threads started" << endl;
			cout << "Working..." << endl;

			unsigned single_hash_size = _proto_task->get_hash_size();

			int read = 1;
			unsigned chunk_number = 0;
			while (read > 0)
			{
				if (excep_control.is_exception())
				{
					break;
				}

				auto task = _free_tasks.wait_and_get();
				read = _src_file.read_bytes(task->get_buffer(), task->get_buffer_size());
				if (read > 0)
				{
					task->init(read, _hash_file_buff.get() + chunk_number * single_hash_size);
					threads.add_task(chunk_number, task);
					++chunk_number;
				}
				else
				{
					_free_tasks.add(task);
				}
			}

			cout << "Reading completed" << endl;

			threads.join();

			excep_control.try_throw(); // Check if any exception occured.

			_hash_file.write_bytes(_hash_file_buff.get(), single_hash_size * _chunks_in_source_file);
		}

	private:

		void allocate_tasks()
		{
			if (!_proto_task.operator->())
			{
				throw exception("Application has not been inited yet.");
			}

			// Create buffer for output file.
			_hash_file_buff.reset(new unsigned char[_proto_task->get_hash_size() * _chunks_in_source_file]);

			// Allocate as much tasks as possible.
			
			// Reserve some memory to leave it to the system.
			boost::scoped_array<unsigned char> reserve_ptr(new unsigned char[50 * 1024 * 1024]);

			_free_tasks.clear();

			unsigned memory_allocated = 0;
			while (memory_allocated < _max_free_task_memory &&
				_free_tasks.size() < _max_free_task_count &&
				_free_tasks.size() < _chunks_in_source_file)
			{
				try
				{
					shared_ptr<hash_task> task_ptr(_proto_task->new_instance()); // See [2].
					task_ptr->create_buffer(_chunk_size_bytes);
					memory_allocated += _chunk_size_bytes;
					_free_tasks.add(task_ptr);
				}
				catch (const bad_alloc& e)
				{
					break; // Break on out-of-memory and free reserved memory.
				}
			}

			cout << "Free tasks count: " << _free_tasks.size() << endl;
		}
	};
}

#endif // ! _VHASHCPP_APP_H_