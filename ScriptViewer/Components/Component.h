#pragma once

#include <algorithm>
#include <vector>

typedef unsigned long long DWORD64;

class Component;

namespace rage
{
	class scrThread;
}

inline std::vector<Component *> g_rgpComponents;

class Component
{
  public:
	bool m_bIsOpen = false;

	Component()
	{
		g_rgpComponents.push_back(this);
	}

	virtual ~Component()
	{
		g_rgpComponents.erase(std::find(g_rgpComponents.begin(), g_rgpComponents.end(), this));
	}

	Component(const Component &)            = delete;

	Component &operator=(const Component &) = delete;

	Component(Component &&component) noexcept : Component()
	{
		m_bIsOpen = component.m_bIsOpen;
	}

	Component &operator=(Component &&) = delete;

	virtual bool RunHook(rage::scrThread *pScrThread)
	{
		return true;
	}

	virtual void RunCallback(rage::scrThread *pScrThread, DWORD64 qwExecutionTime)
	{
	}

	virtual void RunImGui()
	{
	}

	virtual void RunScript()
	{
	}
};