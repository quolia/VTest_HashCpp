#pragma once
#ifndef  _VHASHCPP_EXCC_H_
#define _VHASHCPP_EXCC_H_

#include <string>
#include <mutex>

namespace VHASHCPP
{
	using namespace std;

	class exception_control
	{
		volatile bool _is_exception;
		mutex _lock;
		string _what;

	public:

		exception_control()
		{
			_is_exception = false;
		}

		bool const is_exception()
		{
			return _is_exception;
		}

		void set_exception(const exception& e)
		{
			lock_guard<mutex> lock(_lock);
			if (!_is_exception)
			{
				_is_exception = true;
				_what = e.what();
			}
		}

		void try_throw()
		{
			lock_guard<mutex> lock(_lock);
			if (_is_exception)
			{
				throw exception(_what.c_str());
			}
		}
	};
}

#endif // ! _VHASHCPP_EXCC_H_