//#include <Log.hpp>
#include <iostream>
#include "Entry.cpp"

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

	double a;
	int b;
	int8_t c;
	bool d;
	bool *e = &d;
	Entry L("test", "just a test log",
			// Entry("bad", "pointer type", &e), // doesn't compile
			Entry("a", "data a", &a),
			Entry("b", "data b", &b),
			Entry("container", "of things",
				  Entry("c", "data c", &c),
				  Entry("d", "data d", &d),
				  Entry("nested", "more things",
						Entry("a", "a again", &a))),
			Entry("e", "also d", e),
			Entry("stuff", "a struct", &stuff,
				  Entry("s1", "element1", Represents<bool>(4)),
				  Entry("s2", "element2", Represents<float>()),
				  Entry("s3", "entry3", Represents<int>(2)),
				  // Entry("bad","pointer type", Represents<float*>()), // doesn't run
				  Entry("s4", "entry4", Represents<double>())),
			// Entry("bad", "a struct again", &stuff), // doesn't compile
			// Entry("bad", "of nothing"), // doesn't compile
			Entry("last", "last one", &a));
	std::cout << L.get_header() << "\n";
}