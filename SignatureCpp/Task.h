#pragma once
#ifndef  _VHASHCPP_TASK_H_
#define _VHASHCPP_TASK_H_

#include <memory>
#include <boost/uuid/detail/md5.hpp>
#include <boost/crc.hpp>

namespace VHASHCPP
{
	using boost::uuids::detail::md5;

	// Base class for hash-function tasks.
	class hash_task
	{
	protected:

		// Size of buffer to store data to hash.
		unsigned _buffer_size;

		// Smart-pointer to delete chunk buffer.
		boost::scoped_array<unsigned char> _buffer_ptr;

		// Actual data size.
		unsigned _actual_data_size;

		// Destination address to save hash.
		unsigned char* _dst_buffer;

	public:

		virtual void do_task() = 0;
		virtual unsigned get_hash_size() = 0;
		virtual hash_task* new_instance() = 0;
		virtual ~hash_task() {};

		hash_task()
		{
			_buffer_size = 0;
			_actual_data_size = 0;
			_dst_buffer = nullptr;
		}

		void create_buffer(unsigned buffer_size)
		{
			_buffer_size = buffer_size;
			_buffer_ptr.reset(new unsigned char[buffer_size]);
		}

		unsigned char* const get_buffer()
		{
			return _buffer_ptr.get();
		}

		unsigned const get_buffer_size()
		{
			return _buffer_size;
		}

		// Init task with actual data size and destination buffer.
		void init(unsigned data_size, unsigned char* dst_buffer)
		{
			_actual_data_size = data_size;
			_dst_buffer = dst_buffer;
		}
	};

	// MD5 implementation of hash_task.
	class hash_task_md5 : public hash_task
	{
		md5::digest_type _digest;

	public:

		static const char* name()
		{
			return "md5";
		}

		virtual hash_task* new_instance()
		{
			return new hash_task_md5();
		}

		virtual unsigned get_hash_size()
		{
			return sizeof(_digest);
		}

		virtual void do_task()
		{
			// Seems like boost::md5 cannot be reused after some amount of time. It gives different value.
			// So, recreate it on stack.

			md5 hash; 

			hash.process_bytes(_buffer_ptr.get(), _actual_data_size);
			hash.get_digest(_digest);

			memcpy(_dst_buffer, reinterpret_cast<const char*>(&_digest), get_hash_size());
		}
	};

	// CRC32 implementation of hash_task.
	class hash_task_crc32 : public hash_task
	{
	public:

		static const char* name()
		{
			return "crc32";
		}

		virtual hash_task* new_instance()
		{
			return new hash_task_crc32();
		}

		virtual unsigned get_hash_size()
		{
			return 4;
		}

		virtual void do_task()
		{
			boost::crc_32_type result;
			result.process_bytes(_buffer_ptr.get(), _actual_data_size);
			unsigned int crc32 = result.checksum();

			memcpy(_dst_buffer, reinterpret_cast<const char*>(&crc32), get_hash_size());
		}
	};
}

#endif // ! _VHASHCPP_TASK_H_