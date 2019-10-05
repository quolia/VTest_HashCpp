// SignatureCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>	
#include <chrono>
#include "App.h"

using namespace VHASHCPP;
using namespace std;

int main(int argc, char* argv[])
{
	cout << "Started" << endl;
	
	try
	{
		auto start = chrono::system_clock::now();

		application app;
		app.init(argc, argv, 10000, 1024 * 1024 * 1024 * 2ULL);
		app.run();

		chrono::duration<double> elapsed_seconds = chrono::system_clock::now() - start;
		cout << "Done in " << elapsed_seconds.count() << " sec, " << app.src_file_size() / 1024 / 1024 / elapsed_seconds.count() << " MB/s" << endl;

		return 0;
	}
	catch (const exception& e)
	{
		cout << "Error: " << e.what() << endl;
		return -1;
	}
}