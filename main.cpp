//#include <Log.hpp>
#include <iostream>
#include "Entry.cpp"

using namespace BasicLog;

int main(void)
{

	struct stuff
	{
		double a;
		int b;
		bool c;
		float d;
	};

	struct stuff stuff
	{
		1.0, 2, true, 3.14
	};

	double a;
	int b;
	int8_t c;
	bool d;
	bool *e = &d;
	Entry L("test", "just a test log",
			Entry("a", "data a", &a),
			Entry("b", "data b", &b),
			Entry("container", "of things",
				  Entry("c", "data c", &c),
				  Entry("d", "data d", &d),
				  Entry("nested", "more things",
						Entry("a", "a again", &a))),
			Entry("e", "also d", e));
	//		  DataStructure("stuff", "a struct", &stuff, typeid(double), typeid(int), typeid(bool),typeid(float)));
	std::cout << L.get_header() << "\n";
}