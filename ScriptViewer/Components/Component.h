#pragma once

#include <vector>

typedef unsigned long long DWORD64;

class Component;

namespace rage
{
	class scrThread;
}

inline std::vector<Component*> g_rgpComponents;

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

	Component(Component&) = delete;

	Component& operator=(Component& component) = delete;

	Component(Component&& component) noexcept : Component()
	{
		m_bIsOpen = component.m_bIsOpen;
	}

	Component& operator=(Component&& component) = delete;

	virtual inline bool RunHook(rage::scrThread* pScrThread)
	{
		return true;
	}

	virtual inline void RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime)
	{

	}

	virtual inline void RunImGui()
	{
		
	}

	virtual inline void RunScript()
	{

	}
};