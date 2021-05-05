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

	Handle FindPattern(const std::string& szPattern);
	MH_STATUS AddHook(void* pTarget, void* pDetour, void* ppOrig);
	
	template <typename T>
	inline void Write(T* pAddr, T value, int count = 1)
	{
		DWORD dwOldProtect;
		VirtualProtect(pAddr, sizeof(T) * count, PAGE_EXECUTE_READWRITE, &dwOldProtect);

		for (int i = 0; i < count; i++)
		{
			pAddr[i] = value;
		}

		VirtualProtect(pAddr, sizeof(T) * count, dwOldProtect, &dwOldProtect);
	}

	const char* const GetTypeName(__int64 vptr);
}