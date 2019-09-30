#pragma once
#ifndef  _VHASHCPP_FILE_H_
#define _VHASHCPP_FILE_H_

#include <string>
#include <fstream>

namespace VHASHCPP
{
	using namespace std;

	// Simple wrapper for a file.
	class binary_file
	{
		FILE* _file;
		string _name;
		bool _for_read;

	public:

		binary_file()
		{
			_file = nullptr;
			_for_read = true;
		}

		~binary_file()
		{
			if (_file)
			{
				fclose(_file);
				_file = nullptr;
			}
		}

		void open_for_read(const string& file_name)
		{
			open(file_name, true);
		}

		void open_for_write(const string& file_name)
		{
			open(file_name, false);
		}

		int read_bytes(void* src, int count)
		{
			if (!_for_read)
			{
				throw exception("File is not for read.");
			}

			return fread(src, sizeof(unsigned char), count, _file);
		}

		void write_bytes(void* dst, int count)
		{
			if (_for_read)
			{
				throw exception("File is not for write.");
			}

			fwrite(dst, sizeof(unsigned char), count, _file);
		}

		long long const size()
		{
			if (!_file)
			{
				throw exception("File is not opened.");
			}
			else
			{
				streampos fsize = 0;
				ifstream myfile(_name.c_str(), ios::in);
				fsize = myfile.tellg();
				myfile.seekg(0, ios::end);
				fsize = myfile.tellg() - fsize;
				myfile.close();
				return fsize;
			}
		}

	protected:

		void open(const string& file_name, bool for_read)
		{
			_name = file_name;
			_for_read = for_read;
			_file = fopen(file_name.c_str(), for_read ? "rb" : "wb");
			if (!_file)
			{
				throw exception("File does not exist or cannot be opened.");
			}
		}
	};
}

#endif // ! _VHASHCPP_FILE_H_