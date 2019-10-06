#pragma once
#ifndef  _VHASHCPP_APP_H_
#define _VHASHCPP_APP_H_

#include <iostream>
#include <string>
#include "File.h"
#include "CLI.h"
#include "TaskFactory.h"
#include "Queue.h"
#include "Thread.h"
#include "Types.h"

namespace VHASHCPP
{
	using namespace std;

	class application
	{
		bool _is_inited;

		shared_ptr<byte_array> _hash_file_buff;
		shared_ptr<task_queue> _free_tasks;
		shared_ptr<binary_file> _src_file;
		shared_ptr<binary_file> _hash_file;
		shared_ptr<hash_task> _proto_task;
		shared_ptr<task_threads> _threads;
		shared_ptr<exception_control> _exception_control;

	public:

		application()
		{
			_is_inited = false;
		}
		
		long long src_file_size()
		{
			if (!_is_inited)
			{
				throw exception("Application has not been inited yet.");
			}

			if (!_src_file)
			{
				throw exception("Source file has not been inited yet.");
			}

			return _src_file->size();
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

			_free_tasks.reset(new task_queue());
			_src_file.reset(new binary_file());
			_hash_file.reset(new binary_file());
			_threads.reset(new task_threads());
			_exception_control.reset(new exception_control());

			command_line cli_params;
			cli_params.init(argc, argv);

			cout << "Source file: " << cli_params.src_file_name() << endl;
			_src_file->open_for_read(cli_params.src_file_name());

			cout << "Source file size, bytes: " << _src_file->size() << endl;
			if (_src_file->size() < 1)
			{
				throw exception("Invalid source file size.");
			}

			cout << "Hash file: " << cli_params.hash_file_name() << endl;
			_hash_file->open_for_write(cli_params.hash_file_name());

			cout << "Source chunk size to hash, MBs: " << cli_params.chunk_size_mb() << endl;

			unsigned chunk_size_bytes = cli_params.chunk_size_mb() * 1024 * 1024;
			cout << "Source chunk size, bytes: " << chunk_size_bytes << endl;

			unsigned chunks_in_source_file = (unsigned)(_src_file->size() / chunk_size_bytes);
			if (_src_file->size() % chunk_size_bytes)
			{
				++chunks_in_source_file;
			}
			cout << "Chunks in source file: " << chunks_in_source_file << endl;

			cout << "Hash name: " << cli_params.hash_name() << endl;

			// Base instance from which all other instances will be created. See [2].
			_proto_task.reset(hash_task_factory::new_instance_by_name(cli_params.hash_name()));

			unsigned single_hash_size = _proto_task->get_hash_size();
			cout << "Chunk hash size, bytes: " << single_hash_size << endl;
			if (single_hash_size < 1)
			{
				throw exception("Invalid single hash size. Must be larger than 0.");
			}
			cout << "Hash file size, bytes: " << single_hash_size * chunks_in_source_file << endl;

			unsigned cpu_count = thread::hardware_concurrency();
			cout << "Threads count: " << cpu_count << endl;
			if (cpu_count < 1)
			{
				throw exception("Unable to get CPUs count.");
			}
			_threads->init(cpu_count);

			allocate_tasks(chunks_in_source_file, chunk_size_bytes, max_free_task_count, max_free_task_memory);

			_is_inited = true;
		}

		void run()
		{
			if (!_is_inited)
			{
				throw exception("Application has not been inited yet.");
			}

			_threads->start(_free_tasks, _exception_control);

			cout << "Threads started" << endl;
			cout << "Working..." << endl;

			unsigned single_hash_size = _proto_task->get_hash_size();

			int read = 1;
			unsigned chunk_number = 0;
			while (read > 0 && !_exception_control->is_exception())
			{
				auto task = _free_tasks->wait_and_get();
				read = _src_file->read_bytes(task->get_buffer(), task->get_buffer_size());
				if (read > 0)
				{
					task->init(read, _hash_file_buff, chunk_number * single_hash_size);
					_threads->add_task(chunk_number, task);
					++chunk_number;
				}
				else
				{
					_free_tasks->add(task);
				}
			}

			cout << "Reading completed" << endl;

			_threads->join();
			_exception_control->try_throw(); // Check if any exception occured.
			_hash_file->write_bytes(_hash_file_buff->buffer(), _hash_file_buff->size());
		}

	private:

		void allocate_tasks(unsigned chunks_in_source_file, unsigned chunk_size_bytes, unsigned max_free_task_count, unsigned long long max_free_task_memory)
		{
			if (!_proto_task)
			{
				throw exception("Application has not been inited yet.");
			}

			// Create buffer for output file.
			_hash_file_buff.reset(new byte_array(_proto_task->get_hash_size() * chunks_in_source_file));

			// Allocate as much tasks as possible.
			
			// Reserve some memory to leave it to the system.
			byte_array reserve(50 * 1024 * 1024);

			_free_tasks->clear();

			auto memory_allocated = 0ULL;
			while (memory_allocated < max_free_task_memory &&
				_free_tasks->size() < max_free_task_count &&
				_free_tasks->size() < chunks_in_source_file)
			{
				try
				{
					shared_ptr<hash_task> task(_proto_task->new_instance()); // See [2].
					task->create_buffer(chunk_size_bytes);
					memory_allocated += chunk_size_bytes;
					_free_tasks->add(task);
				}
				catch (const bad_alloc& e)
				{
					break; // Break on out-of-memory and free reserved memory.
				}
			}

			cout << "Free tasks count: " << _free_tasks->size() << endl;
		}
	};
}

#endif // ! _VHASHCPP_APP_H_