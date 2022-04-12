#include <iostream>
#include <set>
#include <vector>
#include <algorithm>

int main() {
#if 0
	struct s1 {
		std::string s;
		std::vector<int> v;
		bool operator==(const s1& o) const {
			std::cout << "used" << std::endl;
			return s == o.s && v == o.v;
		}
		bool operator<(const s1& o) const {
			return s < o.s;
		}
	};

	s1 t1{"a", {1,2}};
	s1 t2{"c", {11,22}};
	s1 t3{"b", {13,20}};
	std::set<s1> tt1{t1, t2, t3};
	for (auto& it: tt1) {
		std::cout << it.s << ": ";
		std::for_each(it.v.begin(), it.v.end(), [](int a) { std::cout << a << " ";});
		std::cout << std::endl;
	}
	std::cout << "============" << std::endl;
	t2.v.pop_back();
	t2.v.push_back(30);

	for (auto& it: tt1) {
		std::cout << it.s << ": ";
		std::for_each(it.v.begin(), it.v.end(), [](int a) { std::cout << a << " ";});
		std::cout << std::endl;
	}
	std::cout << "============" << std::endl;

	std::set<s1> tt2{t1, t2, t3};
	for (auto& it: tt2) {
		std::cout << it.s << ": ";
		std::for_each(it.v.begin(), it.v.end(), [](int a) { std::cout << a << " ";});
		std::cout << std::endl;
	}

	std::cout << "Test Find" << std::endl;
	std::cout << &t1 << " " << std::addressof(*tt1.begin()) << std::endl;
	std::cout << &t1 << " " << std::addressof(*tt2.begin()) << std::endl;
	for (auto& it: tt1) {
		auto f = tt2.find(it);
		if (f != tt2.end()) {
			std::cout << "f.s= " << f->s  << std::endl;
			std::for_each(f->v.begin(), f->v.end(), [](int a) { std::cout << a << " ";});
			std::cout << std::endl << " == " << std::endl;
			std::cout << " it.s= " << it.s << std::endl;
			std::for_each(it.v.begin(), it.v.end(), [](int a) { std::cout << a << " ";});
			std::cout << std::endl << " == " << std::endl;
		}
	}

	for (auto& it: tt1) {
		std::cout <<  &it << std::endl;
	}
	for (auto& it: tt2) {
		std::cout <<  &it << std::endl;
	}
#endif
	int a = 10, b=20, c=30, d=40;
	std::set<int> s1{a, b, c, d};
	b = 50;
	auto f = s1.find(b);
	if (f != s1.end()) {
		std::cout << "found" << std::endl;
	} else {
		std::cout << "not found" << std::endl;
	}
	return 0;
}
