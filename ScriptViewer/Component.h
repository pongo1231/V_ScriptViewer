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
	Component()
	{
		g_rgpComponents.push_back(this);
	}

	~Component()
	{
		g_rgpComponents.erase(std::find(g_rgpComponents.begin(), g_rgpComponents.end(), this));
	}

	virtual bool RunHook(rage::scrThread* pScrThread)
	{
		return true;
	}

	virtual void RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime)
	{

	}

	virtual void RunImGui()
	{

	}

	virtual void RunScript()
	{

	}
};