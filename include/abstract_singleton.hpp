#pragma once
#include <cassert>
#include <typeinfo>

template <typename T> 
class CSingleton
{
	static T * ms_singleton;

public:
	CSingleton()
	{
		if (ms_singleton)
			MessageBoxA(nullptr, typeid(T).name(), "CSingleton() DECLARED MORE THAN ONCE", MB_ICONEXCLAMATION | MB_OK);

		assert(!ms_singleton);
		ms_singleton = static_cast<T*>(this);
	}

	virtual ~CSingleton()
	{
		if (!ms_singleton)
			MessageBoxA(nullptr, typeid(T).name(), "~CSingleton() FREED AT RUNTIME", MB_ICONEXCLAMATION | MB_OK);

		assert(ms_singleton);
		ms_singleton = nullptr;
	}

	CSingleton(const CSingleton&) = delete;
	CSingleton(CSingleton&&) noexcept = delete;
	CSingleton& operator=(const CSingleton&) = delete;
	CSingleton& operator=(CSingleton&&) noexcept = delete;

	__forceinline static T & Instance()
	{
		if (!ms_singleton)
			MessageBoxA(nullptr, typeid(T).name(), "CSingleton::Instance() NEVER DECLARED", MB_ICONEXCLAMATION | MB_OK);

		assert(ms_singleton);
		return (*ms_singleton);
	}

	__forceinline static T * InstancePtr()
	{
		return (ms_singleton);
	}
};

template <typename T>
T * CSingleton<T>::ms_singleton = nullptr;
