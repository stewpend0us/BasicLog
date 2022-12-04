#include <Log.hpp>

using namespace BasicLog;
int main(void)
{

	double a;
	int b;
	int8_t c;
	bool d;
	bool *e = &d;
	Log L("test", "just a test log",
		  DataEntry("a", "data a", &a),
		  DataEntry("b", "data b", &b),
		  ContainerEntry("container", "of things",
						 DataEntry("c", "data c", &c),
						 DataEntry("d", "data d", &d),
						 ContainerEntry("nested", "more things",
										DataEntry("a", "a again", &a))),
		  DataEntry("e", "also d", e));
}