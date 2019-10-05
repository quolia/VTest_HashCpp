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

	public:

		byte_array(size_t size)
		{
			_array.reset(new byte[size]);
		}

		byte* const get()
		{
			return _array.get();
		}
	};
}

#endif // ! _VHASHCPP_TYPES_H_