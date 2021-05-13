#pragma once

#include "Component.h"

#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <array>
#include <chrono>

typedef unsigned long DWORD;
typedef unsigned long long DWORD64;

class ScriptProfile;

class ScriptView : public Component
{
private:
	std::unordered_map<DWORD, ScriptProfile> m_dcScriptProfiles;

	std::vector<DWORD> m_rgdwBlacklistedScriptThreadIds;
	std::mutex m_blacklistedScriptThreadIdsMutex;

	std::atomic<DWORD> m_dwKillScriptThreadId = 0;

	bool m_bShowExecutionTimes = false;
	bool m_bShowStackSizes = false;

	bool m_bIsNewScriptWindowOpen = false;
	std::array<char, 64> m_rgchNewScriptNameBuffer{};
	int m_iNewScriptStackSize = 0;
	bool m_bDoDispatchNewScript = false;

	DWORD64 m_qwLastProfileUpdatedTimestamp = 0;

public:
	inline [[nodiscard]] bool IsScriptThreadIdPaused(DWORD dwThreadId)
	{
		std::lock_guard lock(m_blacklistedScriptThreadIdsMutex);

		return std::find(m_rgdwBlacklistedScriptThreadIds.begin(), m_rgdwBlacklistedScriptThreadIds.end(), dwThreadId) != m_rgdwBlacklistedScriptThreadIds.end();
	}

	void PauseScriptThreadId(DWORD dwThreadId);

	void UnpauseScriptThreadId(DWORD dwThreadId);

	void ClearNewScriptWindowState();

	virtual bool RunHook(rage::scrThread* pScrThread) override;

	virtual void RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime) override;

	virtual void RunImGui() override;

	virtual void RunScript() override;
};

inline ScriptView g_scriptView;