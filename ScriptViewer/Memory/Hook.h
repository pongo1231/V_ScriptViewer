#pragma once

#include <vector>

namespace Memory
{
	inline std::vector<void (*)()> g_rgHooks;
}

class RegisterHook
{
  public:
	RegisterHook(void (*pHook)())
	{
		if (pHook)
		{
			Memory::g_rgHooks.push_back(pHook);
		}
	}

	RegisterHook(RegisterHook &) = delete;
};