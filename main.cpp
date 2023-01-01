#include <iostream>
#include "Log.hpp"

using namespace BasicLog;

int main(void)
{

	struct stuff
	{
		bool c[3];
		float d;
		int b[2];
		double a;
	};

	struct junk
	{
		double a;
	};

	struct stuff sstuff
	{
		true, true, true, 1.0, 2, 3, 3.14
	};

	double a[7] = {-42.0};
	int b;
	int8_t c;
	bool d;
	bool *e = &d;
	char f;

	// static auto E1 = Log::Entry("a","b", &b,3);
	//   static auto E2 = Log::Entry(aa, "dynamic", &a[0], 6);
	//   static auto E3 = Log::Entry<aa, _static__, 5>(&a[0]);
	////static auto E4 = Log::Entry(aa,"dynamic",&stuff);
	// std::cout << E1.header;
	//  std::cout << E2.header;
	//  std::cout << E3.header;

	Log L("test", "just a test log",
		  // Entry("bad", "pointer type", &e), // doesn't compile
		  Log::Entry("a", "data a", a),
		  Log::Entry("b", "data b", &b, 1),
		  Log::Entry("container", "of things",
					 Log::Entry("c", "data c", &c, 1),
					 Log::Entry("d", "data d", &d, 1),
					 Log::Entry("nested", "more things",
								Log::Entry("a", "a again", a))),
		  Log::Entry("e", "also d", e, 1),
		  Log::Entry("stuff", "a struct", &sstuff, 1,
					 Log::Entry("s1_long_na", "element1", &stuff::c),
					 Log::Entry("s2", "thing 2", &stuff::a)),
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
	std::cout << L.MainEntry.header << '\n';
}