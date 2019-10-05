#pragma once
#ifndef  _VHASHCPP_TYPES_H_
#define _VHASHCPP_TYPES_H_

#include <memory>
#include <boost/scoped_array.hpp>

namespace VHASHCPP
{
	using namespace std;

	class byte_array
	{
		boost::scoped_array<byte> _array;
		size_t _size;

	public:

		byte_array(size_t size)
		{
			_size = size;
			_array.reset(new byte[size]);
		}

		void write(size_t start_index, const void* src, size_t src_size)
		{
			if (start_index + src_size > _size)
			{
				throw exception("Trying to overwrite bytes array bounds.");
			}

			memcpy(_array.get() + start_index, src, src_size);
		}

		byte* const buffer()
		{
			return _array.get();
		}

		size_t const size()
		{
			return _size;
		}
	};
}

#endif // ! _VHASHCPP_TYPES_H_