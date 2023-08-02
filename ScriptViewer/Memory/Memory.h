#pragma once

#include "../vendor/minhook/include/MinHook.h"

#include <string>

class Handle;

class HWND__;
typedef HWND__* HWND;

namespace Memory
{
	void Init();
	void InitHooks();
	void FinishHooks();
	void Uninit();

	Handle FindPattern(const std::string& szPattern);
	MH_STATUS AddHook(void* pTarget, void* pDetour, void* ppOrig);
	
	template <typename T>
	inline void Write(void* pAddr, T value, int count = 1)
	{
		DWORD_t dwOldProtect;
		VirtualProtect(static_cast<T*>(pAddr), sizeof(T) * count, PAGE_EXECUTE_READWRITE, &dwOldProtect);

		for (int i = 0; i < count; i++)
		{
			static_cast<T*>(pAddr)[i] = value;
		}

		VirtualProtect(static_cast<T*>(pAddr), sizeof(T) * count, dwOldProtect, &dwOldProtect);
    }

	const char* const GetTypeName(__int64 vptr);
}