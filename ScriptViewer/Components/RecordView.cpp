#include <stdafx.h>

#include "RecordView.h"

#pragma region CONFIG

#define UI_REALTIME_RESULTS_CACHE_FREQ 1000

#pragma endregion

bool RecordView::RunHook(rage::scrThread *pScrThread)
{
	if (!m_bIsRecording || m_eTraceMethod != ETraceMethod::Routine)
	{
		return true;
	}

	if (!Util::IsCustomScriptName(pScrThread->m_szName))
	{
		std::lock_guard lock(m_scriptProfilesMutex);

		if (m_dictScriptProfiles.find(pScrThread->m_dwThreadId) == m_dictScriptProfiles.end())
		{
			m_dictScriptProfiles.emplace(pScrThread->m_dwThreadId,
			                             DetailedScriptProfile(pScrThread->m_szName, ETraceMethod::Routine));
		}

		m_dictScriptProfiles.at(pScrThread->m_dwThreadId).SendMsg(EMessageType::Resume);
	}

	return true;
}

void RecordView::RunCallback(rage::scrThread *pScrThread, DWORD64 qwExecutionTime)
{
	if (!m_bIsRecording)
	{
		return;
	}

	std::lock_guard lock(m_scriptProfilesMutex);

	bool bIsCustomScript = Util::IsCustomScriptName(pScrThread->m_szName);

	if (m_dictScriptProfiles.find(pScrThread->m_dwThreadId) == m_dictScriptProfiles.end())
	{
		m_dictScriptProfiles.emplace(pScrThread->m_dwThreadId,
		                             DetailedScriptProfile(pScrThread->m_szName, m_eTraceMethod, bIsCustomScript));
	}

	if (bIsCustomScript)
	{
		m_dictScriptProfiles.at(pScrThread->m_dwThreadId).AddWithTrace(-1, qwExecutionTime);
	}
	else
	{
		m_dictScriptProfiles.at(pScrThread->m_dwThreadId).AddWithTrace(pScrThread->m_dwIP, qwExecutionTime);

		if (m_eTraceMethod == ETraceMethod::Routine)
		{
			m_dictScriptProfiles.at(pScrThread->m_dwThreadId).SendMsg(EMessageType::Pause);
		}
	}
}

void RecordView::RunImGui()
{
	if (m_bIsRecording)
	{
		DWORD64 qwCurTimestamp = GetTickCount64();

		if (m_qwLastCacheTimestamp < qwCurTimestamp - UI_REALTIME_RESULTS_CACHE_FREQ)
		{
			m_qwLastCacheTimestamp = qwCurTimestamp;

			m_rgFinalScriptProfileSet.clear();
			for (const auto &pair : m_dictScriptProfiles)
			{
				DetailedScriptProfile scriptProfile = pair.second;
				scriptProfile.End();

				m_rgFinalScriptProfileSet.insert(std::move(scriptProfile));
			}
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 400.f, 500.f });
	ImGui::SetNextWindowSize({ 500.f, 700.f }, ImGuiCond_Once);

	ImGui::Begin("Record View", nullptr, ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button(!m_bIsRecording ? "Start Recording" : "Stop Recording"))
	{
		m_bIsRecording = !m_bIsRecording;

		std::unique_lock lock(m_scriptProfilesMutex);

		if (m_bIsRecording)
		{
			m_dictScriptProfiles.clear();
			m_rgFinalScriptProfileSet.clear();

			m_qwRecordBeginTimestamp = GetTickCount64();
			m_fRecordTimeSecs        = 0.f;

			if (m_eTraceMethod == ETraceMethod::Routine)
			{
				Memory::RegisterTraceCallback(this);
			}
		}
		else
		{
			m_rgFinalScriptProfileSet.clear();
			for (auto &pair : m_dictScriptProfiles)
			{
				pair.second.End();

				m_rgFinalScriptProfileSet.insert(std::move(pair.second));
			}

			if (m_eTraceMethod == ETraceMethod::Routine)
			{
				Memory::UnregisterTraceCallback(this);
			}
		}

		lock.unlock();
	}

	ImGui::SameLine();

	std::ostringstream oss;
	oss << "Time: " << m_fRecordTimeSecs << "s" << std::endl;

	const auto &str = oss.str();
	ImGui::Text("%s", str.c_str());

	if (m_bIsRecording)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}

	ImGui::Text("Trace by:");
	ImGui::SameLine();
	ImGui::RadioButton("WAIT calls", reinterpret_cast<int *>(&m_eTraceMethod), static_cast<char>(ETraceMethod::IP));
	ImGui::SameLine();
	ImGui::RadioButton("Routines (slow, also kinda broken)", reinterpret_cast<int *>(&m_eTraceMethod), static_cast<char>(ETraceMethod::Routine));

	if (m_bIsRecording)
	{
		ImGui::PopItemFlag();
	}

	ImGui::PushItemWidth(-1);

	if (ImGui::BeginTable("##scriptTable", 2,
	                      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
	                          | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
	{
		ImGui::TableSetupColumn("Script Name", 0, .7f);
		ImGui::TableSetupColumn("Total Exec Time", 0, .3f);
		ImGui::TableSetupScrollFreeze(2, 1);
		ImGui::TableHeadersRow();

		ImGui::TableNextColumn();

		for (const DetailedScriptProfile &scriptProfile : m_rgFinalScriptProfileSet)
		{
			bool bShowChildren = false;
			if (scriptProfile.IsCustomScript())
			{
				const auto &str = scriptProfile.GetScriptName();
				ImGui::Text("%s", str.c_str());
			}
			else
			{
				bShowChildren = ImGui::CollapsingHeader(scriptProfile.GetScriptName().c_str());
			}

			if (bShowChildren)
			{
				for (const auto &scriptTrace : scriptProfile.GetTraces())
				{
					std::ostringstream oss;
					oss << "\t";

					if (scriptProfile.GetUsedTraceMethod() == ETraceMethod::IP)
					{
						oss << "IP: ";
					}

					oss << "0x" << std::uppercase << std::hex << scriptTrace->GetIP() << std::endl;

					const auto &str = oss.str();
					ImGui::Text("%s", str.c_str());
				}
			}

			ImGui::TableNextColumn();

			std::ostringstream oss;
			oss << scriptProfile.GetSecs() << "s" << std::endl;

			const auto &str = oss.str();
			ImGui::Text("%s", str.c_str());

			if (bShowChildren)
			{
				for (const auto &scriptTrace : scriptProfile.GetTraces())
				{
					std::ostringstream oss;
					oss << "\t" << scriptTrace->GetSecs() << "s" << std::endl;

					const auto &str = oss.str();
					ImGui::Text("%s", str.c_str());
				}
			}

			ImGui::TableNextColumn();
		}

		ImGui::EndTable();
	}

	ImGui::PopItemWidth();

	ImGui::End();

	ImGui::PopStyleVar();
}

void RecordView::RunScript()
{
	if (!m_bIsRecording)
	{
		return;
	}

	if (g_bPauseGameOnOverlay)
	{
		m_fRecordTimeSecs = (GetTickCount64() - m_qwRecordBeginTimestamp) / 1000.f;
	}
	else
	{
		int ciFrames = GET_FRAME_COUNT();
		if (m_ciFrames < ciFrames)
		{
			m_ciFrames       = ciFrames;

			float fFrameTime = GET_FRAME_TIME();

			m_fRecordTimeSecs += fFrameTime;

			/*if (m_eTraceMethod == ETraceMethod::Routine)
			{
			    std::unique_lock lock(m_scriptProfilesMutex);

			    for (auto& pair : m_dictScriptProfiles)
			    {
			        pair.second.AddToAllTraces(fFrameTime * 1000000.f);
			    }

			    lock.unlock();
			}*/
		}
	}
}

void RecordView::ScriptRoutineCallback(rage::scrThread *pThread, ERoutineTraceType eTraceType, DWORD_t dwEnterIP)
{
	std::lock_guard lock(m_scriptProfilesMutex);

	if (m_dictScriptProfiles.find(pThread->m_dwThreadId) == m_dictScriptProfiles.end())
	{
		m_dictScriptProfiles.emplace(
		    pThread->m_dwThreadId,
		    DetailedScriptProfile(pThread->m_szName, m_eTraceMethod, Util::IsCustomScriptName(pThread->m_szName)));
	}

	switch (eTraceType)
	{
	case ERoutineTraceType::Enter:
		m_dictScriptProfiles.at(pThread->m_dwThreadId).SendMsg(EMessageType::EnterCalled, dwEnterIP);

		break;
	case ERoutineTraceType::Leave:
		m_dictScriptProfiles.at(pThread->m_dwThreadId).SendMsg(EMessageType::LeaveCalled);

		break;
	}
}