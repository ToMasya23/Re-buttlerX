#pragma once

template <class T>
class Singleton {
public:
	static T& instance() {
		static T inst;      // C++11 起线程安全
		return inst;
	}
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
protected:
	Singleton() = default;
	~Singleton() = default;
};
