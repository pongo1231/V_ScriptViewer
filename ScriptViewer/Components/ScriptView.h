#pragma once

#include "Component.h"

#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <array>
#include <chrono>

typedef unsigned short WORD;
typedef unsigned long DWORD;

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

	WORD m_wSelectedItemIdx = 0;

	DWORD64 m_qwLastProfileUpdatedTimestamp = 0;

public:
	ScriptView() : Component()
	{
		m_bIsOpen = true;
	}

	ScriptView(const ScriptView&) = delete;

	ScriptView& operator=(const ScriptView&) = delete;

	ScriptView(ScriptView&& scriptView) noexcept : Component(std::move(scriptView))
	{

	}

	ScriptView& operator=(ScriptView&&) = delete;

	virtual bool RunHook(rage::scrThread* pScrThread) override;

	virtual void RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime) override;

	virtual void RunImGui() override;

	virtual void RunScript() override;

private:
	[[nodiscard]] bool IsScriptThreadIdPaused(DWORD dwThreadId);

	void PauseScriptThreadId(DWORD dwThreadId);

	void UnpauseScriptThreadId(DWORD dwThreadId);

	void ClearNewScriptWindowState();
};