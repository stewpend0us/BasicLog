//#include <Log.hpp>
#include <iostream>
#include "Log.hpp"

using namespace BasicLog;

int main(void)
{

	struct stuff
	{
		bool c[4];
		float d;
		int b[2];
		double a;
	};

	struct stuff stuff
	{
		true, true, true, true, 1.0, 2, 3, 3.14
	};

	double a = 42.0;
	int b;
	int8_t c;
	bool d;
	bool *e = &d;
	char f;
	static constexpr std::string_view aa = "aa";
	static constexpr std::string_view bb = "bb";
	auto E = Log::Entry<aa,bb>(&a);
	std::cout << E.header << '\n';

	//Entry L("test", "just a test log",
	//		// Entry("bad", "pointer type", &e), // doesn't compile
	//		Entry("a", "data a", &a),
	//		Entry("b", "data b", &b),
	//		Entry("container", "of things",
	//			  Entry("c", "data c", &c),
	//			  Entry("d", "data d", &d),
	//			  Entry("nested", "more things",
	//					Entry("a", "a again", &a))),
	//		Entry("e", "also d", e),
	//		Entry("stuff", "a struct", &stuff,
	//			  Child::Entry<bool,4>("s1", "element1"),
	//			  Child::Entry<float>("s2", "element2"),
	//			  Child::Entry<int,2>("s3", "entry3"),
////				Entry("bad","bad",&b), // doesn't compile
////				Child::Entry<bool>("extra", "too much"), // doesn't run
	//			  Child::Entry<double>("s4", "entry4")),
	//		// Entry("bad", "a struct again", &stuff), // doesn't compile
	//		// Entry("bad", "of nothing"), // doesn't compile
	//		Entry("f", "data f", &f),
	//		Entry("last", "last one", &a));
	//std::cout << L.get_header() << '\n';
}