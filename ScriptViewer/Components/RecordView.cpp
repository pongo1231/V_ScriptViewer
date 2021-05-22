#include <stdafx.h>

#include "RecordView.h"

void RecordView::RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime)
{
	if (!m_bIsRecording)
	{
		return;
	}

	std::lock_guard lock(m_scriptProfilesMutex);

	if (m_dictScriptProfiles.find(pScrThread->m_dwThreadId) == m_dictScriptProfiles.end())
	{
		m_dictScriptProfiles.emplace(pScrThread->m_dwThreadId, pScrThread->m_szName);
	}

	m_dictScriptProfiles.at(pScrThread->m_dwThreadId)
		.AddWithTrace(Util::IsCustomScriptName(pScrThread->m_szName) ? -1 : pScrThread->m_dwIP, qwExecutionTime);
}

void RecordView::RunImGui()
{
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
			m_fRecordTimeSecs = 0.f;
		}
		else
		{
			for (const auto& pair : m_dictScriptProfiles)
			{
				m_rgFinalScriptProfileSet.insert(pair.second);
			}
		}

		lock.unlock();
	}

	ImGui::SameLine();

	std::ostringstream oss;
	oss << "Time: " << m_fRecordTimeSecs << "s" << std::endl;

	ImGui::Text(oss.str().c_str());

	ImGui::PushItemWidth(-1);

	if (ImGui::BeginTable("", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
		| ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
	{
		ImGui::TableSetupColumn("Script Name", 0, .7f);
		ImGui::TableSetupColumn("Total Exec Time", 0, .3f);
		ImGui::TableSetupScrollFreeze(2, 1);
		ImGui::TableHeadersRow();

		ImGui::TableNextColumn();

		for (const DetailedScriptProfile& scriptProfile : m_rgFinalScriptProfileSet)
		{
			ImGui::Text(scriptProfile.GetScriptName().c_str());
			for (const auto& scriptTrace : scriptProfile.GetTraces())
			{
				std::ostringstream oss;
				oss << "\tIP: 0x" << std::hex << scriptTrace.GetIP() << std::endl;

				ImGui::Text(oss.str().c_str());
			}

			ImGui::TableNextColumn();

			std::ostringstream oss;
			oss << scriptProfile.GetSecs() << "s" << std::endl;

			ImGui::Text(oss.str().c_str());
			for (const auto& scriptTrace : scriptProfile.GetTraces())
			{
				std::ostringstream oss;
				oss << "\t" << scriptTrace.GetSecs() << "s" << std::endl;

				ImGui::Text(oss.str().c_str());
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
			m_ciFrames = ciFrames;
			m_fRecordTimeSecs += GET_FRAME_TIME();
		}
	}
}