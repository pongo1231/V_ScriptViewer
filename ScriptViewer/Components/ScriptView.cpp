#include <stdafx.h>

#include "ScriptView.h"

#pragma region CONFIG

#define SCRIPT_PROFILING_UPDATE_FREQ 3000

#pragma endregion

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
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 400.f, 500.f });
	ImGui::SetNextWindowSize({ 500.f, 700.f }, ImGuiCond_Once);

	ImGui::Begin("Script Viewer", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::PushItemWidth(-1);

	int ciColumns = 1;
	if (m_bShowExecutionTimes)
	{
		ciColumns++;
	}
	if (m_bShowStackSizes)
	{
		ciColumns++;
	}

	if (ImGui::BeginTable("", ciColumns, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
		| ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, { 0, -73.f }))
	{
		ImGui::TableSetupColumn("Script Name", 0, .6f);
		if (m_bShowExecutionTimes)
		{
			ImGui::TableSetupColumn("Exec Time", 0, .2f);
		}
		if (m_bShowStackSizes)
		{
			ImGui::TableSetupColumn("Stack Size", 0, .2f);
		}
		ImGui::TableSetupScrollFreeze(ciColumns, 1);
		ImGui::TableHeadersRow();

		ImGui::TableNextColumn();

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

			bool bIsCustomThread = Util::IsCustomScriptName(szScriptName);

			if (ImGui::Selectable(szScriptName, wScriptIdx == m_wSelectedItemIdx, ImGuiSelectableFlags_SpanAllColumns))
			{
				m_wSelectedItemIdx = wScriptIdx;
			}

			if (m_bShowExecutionTimes)
			{
				ImGui::TableNextColumn();

#ifdef RELOADABLE
				if (!bIsCustomThread)
#endif
				{
					std::ostringstream oss;
					oss << scriptProfile.GetMs() << "ms" << std::endl;

					ImGui::Text(oss.str().c_str());
				}
			}

			if (m_bShowStackSizes)
			{
				ImGui::TableNextColumn();

				if (!bIsCustomThread)
				{
					rage::_scrStack* pStack = pThread->GetScriptStack();

					if (pStack)
					{
						std::ostringstream oss;
						oss << pStack->m_dwStackSize << std::endl;

						ImGui::Text(oss.str().c_str());
					}
				}
			}

			if (bIsScriptAboutToBeKilled || bIsScriptPaused)
			{
				ImGui::PopStyleColor();
			}

			ImGui::TableNextColumn();
		}

		ImGui::EndTable();
	}

	ImGui::PopItemWidth();

	ImGui::Spacing();

	m_wSelectedItemIdx = min(m_wSelectedItemIdx, *rage::scrThread::ms_pcwThreads);

	rage::scrThread* pThread = rage::scrThread::ms_ppThreads[m_wSelectedItemIdx];

	std::string szSelectedScriptName = pThread->m_szName;
	DWORD dwSelectedThreadId = pThread->m_dwThreadId;

#ifdef RELOADABLE
	bool bIsSelectedScriptUnpausable = Util::IsCustomScriptName(szSelectedScriptName);
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

	ImGui::End();

	ImGui::PopStyleVar();

	if (m_bIsNewScriptWindowOpen)
	{
		ImGui::SetNextWindowSize({ 400.f, 100.f });

		ImGui::Begin("New Script", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

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

[[nodiscard]] bool ScriptView::IsScriptThreadIdPaused(DWORD dwThreadId)
{
	std::lock_guard lock(m_blacklistedScriptThreadIdsMutex);

	return std::find(m_rgdwBlacklistedScriptThreadIds.begin(), m_rgdwBlacklistedScriptThreadIds.end(), dwThreadId) != m_rgdwBlacklistedScriptThreadIds.end();
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