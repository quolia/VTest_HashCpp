// SignatureCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <boost/scoped_array.hpp>
#include "Thread.h"
#include "File.h"

using namespace VHASHCPP;
using namespace std;

int main(int argc, char* argv[])
{
	// Restrictions.
	const unsigned MAX_FREE_TASK_COUNT = 10000;
	const unsigned long MAX_FREE_TASK_MEMORY = 1024 * 1024 * 1024 * 2UL; // 2GB

	cout << "Started" << endl;
	
	try
	{
		if (argc < 3)
		{
			throw exception("Invalid parameters count. Example parameters: srcfile hashfile [>=1] [md5,crc32]");
		}

		string src_file_name = argv[1];
		cout << "Source file: " << src_file_name << endl;

		binary_file src_file;
		src_file.open_for_read(src_file_name);

		long long src_file_size = src_file.size();
		cout << "Source file size, bytes: " << src_file_size << endl;
		if (src_file_size < 1)
		{
			throw exception("Invalid file size.");
		}

		string hash_file_name = argv[2];
		cout << "Hash file: " << hash_file_name << endl;

		binary_file hash_file;
		hash_file.open_for_write(hash_file_name);

		const unsigned mb = argc > 3 ? (unsigned)_strtoi64(argv[3], nullptr, 10) : 1;
		if (mb < 1)
		{
			throw exception("Chunk size is invalid. Must be equal or larger than 1.");
		}
		cout << "Source chunk size to hash, MBs: " << mb << endl;

		const unsigned chunk_size = mb * 1024 * 1024;
		cout << "Source chunk size, bytes: " << chunk_size << endl;

		unsigned chunks_in_file = (unsigned)(src_file_size / chunk_size);
		if (src_file_size % chunk_size)
		{
			++chunks_in_file;
		}
		cout << "Chunks in source file: " << chunks_in_file << endl;

		string hash_name = argc > 4 ? argv[4] : "md5";
		cout << "Hash name: " << hash_name << endl;

		hash_task* hash = nullptr;
		shared_ptr<hash_task> base_hash_ptr; // Base instance from which all other instances will be created. See [2].
		if (!_strcmpi(hash_name.c_str(), "md5"))
		{
			base_hash_ptr.reset(new hash_task_md5());
		}
		else if (!_strcmpi(hash_name.c_str(), "crc32"))
		{
			base_hash_ptr.reset(new hash_task_crc32());
		}
		else
		{
			throw exception("Unsupported hash method.");
		}

		task_queue free_tasks;

		int single_hash_size = base_hash_ptr->get_hash_size();
		cout << "Chunk hash size, bytes: " << single_hash_size << endl;
		cout << "Hash file size, bytes: " << single_hash_size * chunks_in_file << endl;

		// Allocate as much tasks as possible.
		{
			// Reserve some memory to leave it to the system.
			unsigned char* reserve = new unsigned char[50 * 1024 * 1024];
			boost::scoped_array<unsigned char> hash_buff_ptr(reserve);

			unsigned memory_allocated = 0;
			while (memory_allocated < MAX_FREE_TASK_MEMORY &&
				   free_tasks.size() < MAX_FREE_TASK_COUNT &&
				   free_tasks.size() < chunks_in_file)
			{
				try
				{
					shared_ptr<hash_task> task_ptr(base_hash_ptr->new_instance()); // See [2].
					task_ptr->create_buffer(chunk_size);
					memory_allocated += chunk_size;

					free_tasks.add(task_ptr);
				}
				catch (const bad_alloc& e)
				{
					break; // Break on out-of-memory and free reserved memory.
				}
			}

			cout << "Free tasks count: " << free_tasks.size() << endl;
		}

		// Create buffer for output file.
		unsigned char* hash_buff = new unsigned char[single_hash_size * chunks_in_file];
		boost::scoped_array<unsigned char> hash_buff_ptr(hash_buff);

		unsigned cpu_count = thread::hardware_concurrency();
		cout << "Threads count: " << cpu_count << endl;
		if (cpu_count < 1)
		{
			throw exception("Unable to get CPU count.");
		}

		vector<task_thread> threads(cpu_count);

		for (unsigned i = 0; i < threads.size(); ++i)
		{
			threads[i].start(free_tasks);
		}
		cout << "Threads started" << endl;

		int read = 1;
		int chunk_number = 0;
		while (read > 0)
		{
			auto task = free_tasks.wait_and_get();
			read = src_file.read_bytes(task->get_buffer(), task->get_buffer_size());
			if (read > 0)
			{
				task->init(read, hash_buff + chunk_number * single_hash_size);
				threads[chunk_number % cpu_count].add_task(task);
				++chunk_number;
			}
			else
			{
				free_tasks.add(task);
			}
		}

		cout << "Reading completed" << endl;

		for (unsigned i = 0; i < threads.size(); ++i)
		{
			threads[i].join();
		}

		hash_file.write_bytes(hash_buff, single_hash_size * chunks_in_file);

		cout << "Done" << endl;

		return 0;
	}
	catch (const exception& e)
	{
		cout << "Error: " << e.what() << endl;
		return -1;
	}
}