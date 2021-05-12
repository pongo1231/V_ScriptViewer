#pragma once

#include <vector>

namespace Memory
{
	inline std::vector<void(*)()> g_rgHooks;
}

class RegisterHook
{
public:
	RegisterHook(void(*pOnHook)())
	{
		if (pOnHook)
		{
			Memory::g_rgHooks.push_back(pOnHook);
		}
	}

	RegisterHook(RegisterHook&) = delete;
};