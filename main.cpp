//#include <Log.hpp>
#include <iostream>
#include "Log.hpp"
#include "chararr.hpp"

using namespace BasicLog;

int main(void)
{

	// struct stuff
	//{
	//	bool c[4];
	//	float d;
	//	int b[2];
	//	double a;
	// };

	// struct stuff stuff
	//{
	//	true, true, true, true, 1.0, 2, 3, 3.14
	// };

	// double a[7] = {-42.0};
	// int b;
	// int8_t c;
	// bool d;
	// bool *e = &d;
	// char f;
	static constexpr const char c1[] = "abc";
	static constexpr std::array<char,4> c3 = {'1','2','3','4'};
	static constexpr char q = '"';
	static constexpr auto l = concat("{", "name", q, c1, "defg", c3);
	std::cout << std::string_view(l.data(), l.size());
	std::cout << '\n';
	std::cout << l.size();
	std::cout << '\n';
	//std::cout << l.data() << tl;
	//std::cout << '\n';

	// std::string s = Header_complex(aa, aa, aa, aa);
	// std::cout << s << '\n';
	//  static auto E1 = Log::Entry<aa, _static__>(a);
	//  static auto E2 = Log::Entry(aa, "dynamic", &a[0], 6);
	//  static auto E3 = Log::Entry<aa, _static__, 5>(&a[0]);
	////static auto E4 = Log::Entry(aa,"dynamic",&stuff);
	// std::cout << E1.header;
	// std::cout << E2.header;
	// std::cout << E3.header;

	// Entry L("test", "just a test log",
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
	// std::cout << L.get_header() << '\n';
}