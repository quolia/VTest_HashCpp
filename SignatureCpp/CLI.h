#pragma once
#ifndef  _VHASHCPP_CLI_H_
#define _VHASHCPP_CLI_H_

#include <string>

namespace VHASHCPP
{
	using namespace std;

	// Command line parameters partial validator.
	class command_line
	{
		string _src_file_name;
		string _hash_file_name;
		string _hash_name;
		int _mb;

	public:

		command_line()
		{
			_mb = 0;
		}

		const char* src_file_name()
		{
			return _src_file_name.c_str();
		}

		const char* hash_file_name()
		{
			return _hash_file_name.c_str();
		}

		const char* hash_name()
		{
			return _hash_name.c_str();
		}

		unsigned const chunk_size_mb()
		{
			return _mb;
		}

		void init(int argc, char* argv[])
		{
			if (argc < 3)
			{
				throw exception("Invalid parameters count. Example parameters: srcfile hashfile [>=1] [md5,crc32]");
			}

			_src_file_name = argv[1];
			_hash_file_name = argv[2];

			_mb = argc > 3 ? (unsigned)_strtoi64(argv[3], nullptr, 10) : 1;
			if (_mb < 1)
			{
				throw exception("Chunk size is invalid. Must be equal or larger than 1.");
			}

			_hash_name = argc > 4 ? argv[4] : "md5";
		}
	};
}

#endif // ! _VHASHCPP_CLI_H_