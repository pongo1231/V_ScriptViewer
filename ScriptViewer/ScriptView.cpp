#include <stdafx.h>

#include "ScriptView.h"

#pragma region CONFIG

#define SCRIPT_PROFILING_UPDATE_FREQ 3000

#pragma endregion

static inline [[nodiscard]] rage::_scrStack* GetScriptStack(rage::scrThread* pThread)
{
	for (WORD wStackIdx = 0; wStackIdx < *rage::scrThread::ms_pcwStacks; wStackIdx++)
	{
		rage::_scrStack& stack = rage::scrThread::ms_pStacks[wStackIdx];

		if (stack.m_pScrThread == pThread)
		{
			return &stack;
		}
	}

	return nullptr;
}

void ScriptView::PauseScriptThreadId(DWORD dwThreadId)
{
	std::lock_guard lock(m_blacklistedScriptThreadIdsMutex);

	m_rgdwBlacklistedScriptThreadIds.push_back(dwThreadId);
}

void ScriptView::UnpauseScriptThreadId(DWORD dwThreadId)
{
	std::lock_guard lock(m_blacklistedScriptThreadIdsMutex);

	const auto& itScriptThreadId = std::find(m_rgdwBlacklistedScriptThreadIds.begin(), m_rgdwBlacklistedScriptThreadIds.end(), dwThreadId);

	if (itScriptThreadId != m_rgdwBlacklistedScriptThreadIds.end())
	{
		m_rgdwBlacklistedScriptThreadIds.erase(itScriptThreadId);
	}
}

void ScriptView::ClearNewScriptWindowState()
{
	m_bIsNewScriptWindowOpen = false;

	m_rgchNewScriptNameBuffer.fill(0);

	m_iNewScriptStackSize = 0;

	m_bDoDispatchNewScript = false;
}

bool ScriptView::RunHook(rage::scrThread* pScrThread)
{
	return !IsScriptThreadIdPaused(pScrThread->m_dwThreadId);
}

void ScriptView::RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime)
{
	m_dcScriptProfiles[pScrThread->m_dwThreadId].Add(qwExecutionTime);
}

void ScriptView::RunImGui()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 400.f, 600.f });

	ImGui::Begin("Script Viewer", NULL, ImGuiWindowFlags_NoCollapse);

	ImGui::PopStyleVar();

	static int c_iSelectedItem = 0;

	ImGui::PushItemWidth(-1);

	if (ImGui::ListBoxHeader("", { 0, -96.f }))
	{
		DWORD64 qwTimestamp = GetTickCount64();

		bool bDoNewProfileRound = false;
		if (qwTimestamp > m_qwLastProfileUpdatedTimestamp)
		{
			m_qwLastProfileUpdatedTimestamp = qwTimestamp + SCRIPT_PROFILING_UPDATE_FREQ;

			bDoNewProfileRound = true;
		}

		for (WORD wScriptIdx = 0; wScriptIdx < *rage::scrThread::ms_pcwThreads; wScriptIdx++)
		{
			rage::scrThread* pThread = rage::scrThread::ms_ppThreads[wScriptIdx];

			if (!pThread->m_dwThreadId)
			{
				continue;
			}

			const char* szScriptName = pThread->m_szName;
			DWORD dwThreadId = pThread->m_dwThreadId;

			bool bIsScriptAboutToBeKilled = m_dwKillScriptThreadId == dwThreadId;
			bool bIsScriptPaused = IsScriptThreadIdPaused(dwThreadId);

			if (bIsScriptAboutToBeKilled)
			{
				ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });
			}
			else if (bIsScriptPaused)
			{
				ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 1.f, 0.f, 1.f });
			}

			ScriptProfile& scriptProfile = m_dcScriptProfiles[dwThreadId];

			if (bDoNewProfileRound)
			{
				scriptProfile.NewRound();
			}

			bool bIsCustomThread = !strcmp(szScriptName, "control_thread") || strstr(szScriptName, ".asi");

			std::ostringstream ossScriptText;
			ossScriptText << szScriptName;

#ifdef RELOADABLE
			if (!bIsCustomThread)
#endif
			{
				if (m_bShowExecutionTimes)
				{
					ossScriptText << " | " << scriptProfile.Get() << " ms";
				}
			}

			if (!bIsCustomThread && m_bShowStackSizes)
			{
				rage::_scrStack* pStack = GetScriptStack(pThread);

				if (pStack)
				{
					ossScriptText << " | Stack Size: " << pStack->m_dwStackSize;
				}
			}

			if (ImGui::Selectable(ossScriptText.str().c_str(), wScriptIdx == c_iSelectedItem))
			{
				c_iSelectedItem = wScriptIdx;
			}

			if (bIsScriptAboutToBeKilled || bIsScriptPaused)
			{
				ImGui::PopStyleColor();
			}
		}

		ImGui::ListBoxFooter();
	}

	ImGui::PopItemWidth();

	ImGui::Spacing();

	c_iSelectedItem = min(c_iSelectedItem, *rage::scrThread::ms_pcwThreads);

	rage::scrThread* pThread = rage::scrThread::ms_ppThreads[c_iSelectedItem];

	std::string szSelectedScriptName = pThread->m_szName;
	DWORD dwSelectedThreadId = pThread->m_dwThreadId;

#ifdef RELOADABLE
	bool bIsSelectedScriptUnpausable = szSelectedScriptName == "control_thread" || szSelectedScriptName.find(".asi") != std::string::npos;
#else
	bool bIsSelectedScriptUnpausable = !_stricmp(szSelectedScriptName.c_str(), g_szFileName);
#endif
	bool bIsAnyScriptAboutToBeKilled = m_dwKillScriptThreadId;

	bool bIsSelectedScriptPaused = IsScriptThreadIdPaused(dwSelectedThreadId);

	if (bIsSelectedScriptPaused)
	{
		ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 1.f, 0.f, 1.f });
	}

	if (bIsSelectedScriptUnpausable || bIsAnyScriptAboutToBeKilled)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}

	if (ImGui::Button(bIsSelectedScriptPaused ? "Unpause" : "Pause"))
	{
		if (bIsSelectedScriptPaused)
		{
			UnpauseScriptThreadId(dwSelectedThreadId);
		}
		else
		{
			PauseScriptThreadId(dwSelectedThreadId);
		}
	}

	if (bIsSelectedScriptUnpausable)
	{
		ImGui::PopItemFlag();
	}

	if (bIsSelectedScriptPaused)
	{
		ImGui::PopStyleColor();
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });

	if (ImGui::Button("Kill"))
	{
		m_dwKillScriptThreadId = dwSelectedThreadId;
	}

	ImGui::PopStyleColor();

	if (bIsAnyScriptAboutToBeKilled && !bIsSelectedScriptUnpausable)
	{
		ImGui::PopItemFlag();
	}

	ImGui::SameLine();

	// Store current state (so PopItemFlag won't be unjustify called)
	bool bIsNewScriptWindowOpen = m_bIsNewScriptWindowOpen;

	if (bIsNewScriptWindowOpen || m_bDoDispatchNewScript)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}

	if (ImGui::Button("Start New Script"))
	{
		m_bIsNewScriptWindowOpen = true;
	}

	if (bIsNewScriptWindowOpen || m_bDoDispatchNewScript)
	{
		ImGui::PopItemFlag();
	}

	if (ImGui::Button(m_bShowExecutionTimes ? "Show Execution Times: On" : "Show Execution Times: Off"))
	{
		m_bShowExecutionTimes = !m_bShowExecutionTimes;
	}

	if (m_bShowExecutionTimes)
	{
		ImGui::SameLine();

		if (ImGui::Button(ScriptProfile::ms_eProfileType == eScriptProfileType::HIGHEST_TIME ? "Profile Type: Highest Time"
			: "Profile Type: Average Time"))
		{
			switch (ScriptProfile::ms_eProfileType)
			{
			case eScriptProfileType::HIGHEST_TIME:
				ScriptProfile::ms_eProfileType = eScriptProfileType::AVERAGE_TIME;

				break;
			case eScriptProfileType::AVERAGE_TIME:
				ScriptProfile::ms_eProfileType = eScriptProfileType::HIGHEST_TIME;

				break;
			}
		}
	}

	if (rage::scrThread::ms_pStacks && rage::scrThread::ms_pcwStacks && ImGui::Button(m_bShowStackSizes ? "Show Stack Sizes: On" : "Show Stack Sizes: Off"))
	{
		m_bShowStackSizes = !m_bShowStackSizes;
	}

	if (rage::fwTimer::ms_pbUserPause)
	{
		if (ImGui::Button(g_bPauseGameOnOverlay ? "Pause Game: On" : "Pause Game: Off"))
		{
			g_bPauseGameOnOverlay = !g_bPauseGameOnOverlay;
		}

		ImGui::SameLine();
	}

	if (!rage::fwTimer::ms_pbUserPause || !g_bPauseGameOnOverlay)
	{
		if (ImGui::Button(g_bBlockKeyboardInputs ? "Block Keyboard Inputs: On" : "Block Keyboard Inputs: Off"))
		{
			g_bBlockKeyboardInputs = !g_bBlockKeyboardInputs;
		}
	}

	ImGui::End();

	if (m_bIsNewScriptWindowOpen)
	{
		ImGui::Begin("New Script", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		ImGui::SetWindowSize({ 400.f, 100.f });

		ImGui::InputText("Script Name", m_rgchNewScriptNameBuffer.data(), m_rgchNewScriptNameBuffer.max_size());

		ImGui::InputInt("Script Stack Size", &m_iNewScriptStackSize);

		if (ImGui::Button("Cancel"))
		{
			ClearNewScriptWindowState();
		}

		ImGui::SameLine();

		if (ImGui::Button("Start"))
		{
			m_bDoDispatchNewScript = true;

			m_bIsNewScriptWindowOpen = false;
		}

		ImGui::End();
	}
}

void ScriptView::RunScript()
{
	if (m_dwKillScriptThreadId)
	{
		for (WORD wScriptIdx = 0; wScriptIdx < *rage::scrThread::ms_pcwThreads; wScriptIdx++)
		{
			rage::scrThread* pThread = rage::scrThread::ms_ppThreads[wScriptIdx];

			if (pThread->m_dwThreadId == m_dwKillScriptThreadId)
			{
				pThread->Kill();

				break;
			}
		}

		m_dwKillScriptThreadId = 0;
	}

	if (m_bDoDispatchNewScript)
	{
		if (m_rgchNewScriptNameBuffer[0] && m_iNewScriptStackSize >= 0 && DOES_SCRIPT_EXIST(m_rgchNewScriptNameBuffer.data()))
		{
			REQUEST_SCRIPT(m_rgchNewScriptNameBuffer.data());

			while (!HAS_SCRIPT_LOADED(m_rgchNewScriptNameBuffer.data()))
			{
				WAIT(0);
			}

			START_NEW_SCRIPT(m_rgchNewScriptNameBuffer.data(), m_iNewScriptStackSize);

			SET_SCRIPT_AS_NO_LONGER_NEEDED(m_rgchNewScriptNameBuffer.data());
		}

		ClearNewScriptWindowState();
	}
}