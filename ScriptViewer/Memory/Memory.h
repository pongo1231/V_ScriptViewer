#pragma once

#include <string>

class Handle;
enum MH_STATUS : int;

class HWND__;
typedef HWND__* HWND;

namespace Memory
{
	void Init();
	void Uninit();

	Handle FindPattern(const std::string& pattern);
	MH_STATUS AddHook(void* target, void* detour, void** orig);
	
	template <typename T>
	inline void Write(T* pAddr, T value, int count = 1)
	{
		DWORD dOldProtect;
		VirtualProtect(pAddr, sizeof(T) * count, PAGE_EXECUTE_READWRITE, &dOldProtect);

		for (int i = 0; i < count; i++)
		{
			pAddr[i] = value;
		}

		VirtualProtect(pAddr, sizeof(T) * count, dOldProtect, &dOldProtect);
	}

	const char* const GetTypeName(__int64 vptr);
}