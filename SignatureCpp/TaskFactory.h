#pragma once
#ifndef  _VHASHCPP_TASKF_H_
#define _VHASHCPP_TASKF_H_

#include <memory>
#include "Task.h"

namespace VHASHCPP
{
	using namespace std;

	class hash_task_factory
	{
	public:

		static shared_ptr<hash_task> new_instance_by_name(const char* name)
		{
			if (!_strcmpi(name, hash_task_md5::name()))
			{
				shared_ptr<hash_task> task(new hash_task_md5());
				return task;
			}
			else if (!_strcmpi(name, hash_task_crc32::name()))
			{
				shared_ptr<hash_task> task(new hash_task_crc32());
				return task;
			}

			throw exception("Unsupported hash name.");
		}
	};
}

#endif // ! _VHASHCPP_TASKF_H_