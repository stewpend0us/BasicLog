#include <iostream>
#include "Log.hpp"

using namespace BasicLog;

int main(void)
{

	struct simple
	{
		char a[3];
		uint16_t b;
	};

	struct simple simple_arr[3];
	// Entry(simple_arr,
	//	Entry(&stuff::a),
	//   Entry(&stuff::b));
	//|a0|a1|a2|  |b___|a0|a1|a2|  |b___|a0|a1|a2|  |b___|
	// Entry(simple_arr,
	//   Entry(&stuff::b));
	//|  |  |  |  |b___|  |  |  |  |b___|  |  |  |  |b___|
	// Entry(simple_arr,
	//   Entry(&stuff::a, 3, 1, 0);
	//|  |a1|  |  |    |  |a1|  |  |    |  |a1|  |  |    |
	// Entry(simple_arr, 2, 2, 0
	//   Entry(&stuff::a, 1, 1);
	//|  |a1|  |  |    |  |  |  |  |    |  |a1|  |  |    |

	//{"name":"top","desc":"","type":"","count":3,"parent":"","ind":0} // data: ptr to a, size = 3, ptr to b, size = 1, ptr to a, size = 3, ptr to b size = 1, ptr to a
	//{"name":"a","desc":"","type":"char","count":3,"parent":"top","ind":0} // data: ptr to a, size = sizeof(char)*3
	//{"name":"b","desc":"","type":"uint16_t","count":1,"parent":"top","ind":1} // data: ptr to b, size = sizeof(uint16_t)*1

	struct stuff
	{
		bool a[3];
		float b;
		int c[2];
		double d;
	};

	struct junk
	{
		double a;
	};

	struct stuff sstuff
	{
		true, true, true, 1.0, 2, 3, 3.14
	};

	struct stuff sarr[3];

	std::cout << std::setw(15) << "sstuff" << std::setw(20) << (void *)&sstuff << '\n';
	double a[3] = {-2.0, -1.0, 0.0};
	int b = 1;
	int8_t c = 2;
	bool d = true;
	bool *e = &d;
	char f = 5;

	// static auto E1 = Log::Entry("a","b", &b,3);
	//   static auto E2 = Log::Entry(aa, "dynamic", &a[0], 6);
	//   static auto E3 = Log::Entry<aa, _static__, 5>(&a[0]);
	////static auto E4 = Log::Entry(aa,"dynamic",&stuff);
	// std::cout << E1.header;
	//  std::cout << E2.header;
	//  std::cout << E3.header;

	Log L("test", "just a test log", Log::CompressionMethod::RAW,
				// Entry("bad", "pointer type", &e), // doesn't compile
				Log::Entry("a", "data a", a),
				Log::Entry("b", "data b", &b),
				Log::Entry("container", "of things",
									 Log::Entry("c", "data c", &c, 1),
									 Log::Entry("d", "data d", &d, 1),
									 Log::Entry("nested", "more things",
															Log::Entry("a", "a again", a))),
				Log::Entry("e", "also d", e, 1),
				Log::Entry("stuff", "a struct", &sstuff, 1,
									 Log::StructMember("a", "element1", &stuff::a),
									 Log::StructMember("b", "thing 2", &stuff::b),
									 Log::StructMember("c", "2 ints", &stuff::c),
									 Log::StructMember("d", "a float", &stuff::d)),
				Log::Entry("sarr", "array of stuff", sarr,
									 Log::StructMember("a", "element1", &stuff::a),
									 Log::StructMember("c", "2 ints", &stuff::c),
									 Log::StructMember("b", "thing 2", &stuff::b),
									 Log::StructMember("d", "a float", &stuff::d)),
				// Log::Entry<bool,3>("s1_long_na", "element1"),
				// Log::Entry<float>("s2", "element2"),
				// Log::Entry<int,3>("s3", "entry3"),
				////				Entry("bad","bad",&b), // doesn't compile
				////				Child::Entry<bool>("extra", "too much"), // doesn't run
				// Log::Entry<double>("s4", "entry4")),
				// Entry("bad", "a struct again", &stuff), // doesn't compile
				// Entry("bad", "of nothing"), // doesn't compile
				Log::Entry("f", "data f", &f, 1),
				Log::Entry("last", "last one", a));

	std::cout << "\n====================\n\n\n";

	Log L2("simple", "log", Log::CompressionMethod::RAW,
				 Log::Entry("a", "data a", a),
				 Log::Entry("b", "data b", &b),
				 Log::Entry("c", "data c", &c),
				 Log::Entry("d", "data d", &d),
				 Log::Entry("f", "data f", &f));
	//	std::cout << L.MainEntry.header() << '\n';

	std::vector<Log::Entry> test;
	test.push_back(Log::Entry("a", "data a", a));
	test.push_back(Log::Entry("b", "data b", &b));
	test.push_back(Log::Entry("c", "data c", &c));
	test.push_back(Log::Entry("d", "data d", &d));
	test.push_back(Log::Entry("f", "data f", &f));
	Log L3("dynamic","log",Log::CompressionMethod::DIFF, test);

	std::cout << "L2 data (manual):\n";
	std::cout << "size " << (sizeof(a) + sizeof(b) + sizeof(c) + sizeof(d) + sizeof(f)) << '\n';
	for (int i = 0; i < sizeof(c); i++)
	{
		std::cout << (int)(((char *)(&c))[i]) << ", ";
	}
	for (int i = 0; i < sizeof(d); i++)
	{
		std::cout << (int)(((char *)(&d))[i]) << ", ";
	}
	for (int i = 0; i < sizeof(f); i++)
	{
		std::cout << (int)(((char *)(&f))[i]) << ", ";
	}
	for (int i = 0; i < sizeof(b); i++)
	{
		std::cout << (int)(((char *)(&b))[i]) << ", ";
	}
	for (int i = 0; i < sizeof(a); i++)
	{
		std::cout << (int)(((char *)(&a))[i]) << ", ";
	}
	std::cout << "\n\nL2 data (auto):\n";
	for (auto &e : L2.data)
	{
		for (int i = 0; i < e.count; i++)
		{
			std::cout << (int)(e.ptr[i]) << ", ";
		}
	}
	std::cout << "\n\nL3 data (auto):\n";
	for (auto &e : L3.data)
	{
		for (int i = 0; i < e.count; i++)
		{
			std::cout << (int)(e.ptr[i]) << ", ";
		}
	}
	std::cout << "\n";


	L3.record();
	L3.start("/home/swadsworth/test/logs");
	L3.record();
	L3.stop();
	L3.record();
}