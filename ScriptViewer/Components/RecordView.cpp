#include <stdafx.h>

#include "RecordView.h"

void RecordView::RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime)
{
	if (!m_bIsRecording)
	{
		return;
	}

	if (m_dictScriptProfiles.find(pScrThread->m_dwThreadId) == m_dictScriptProfiles.end())
	{
		m_dictScriptProfiles.emplace(pScrThread->m_dwThreadId, pScrThread->m_szName);
	}

	m_dictScriptProfiles.at(pScrThread->m_dwThreadId).AddWithTrace(pScrThread->m_dwIP, qwExecutionTime);
}

void RecordView::RunImGui()
{
	ImGui::Begin("Record View", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::PushItemWidth(-1);

	if (ImGui::ListBoxHeader("", { 0, -25.f }))
	{
		for (const DetailedScriptProfile& scriptProfile : m_rgFinalScriptProfileSet)
		{
			std::ostringstream oss;

			oss << scriptProfile.GetScriptName() << " | " << scriptProfile.GetSecs() << "s" << std::endl;

			ImGui::Selectable(oss.str().c_str());
		}

		ImGui::ListBoxFooter();
	}

	if (ImGui::Button(!m_bIsRecording ? "Start Recording" : "Stop Recording"))
	{
		m_bIsRecording = !m_bIsRecording;

		if (m_bIsRecording)
		{
			m_dictScriptProfiles.clear();
			m_rgFinalScriptProfileSet.clear();

			m_qwStartTimestamp = GetTickCount64();
		}
		else
		{
			for (const auto& pair : m_dictScriptProfiles)
			{
				m_rgFinalScriptProfileSet.insert(pair.second);
			}
		}
	}

	if (m_bIsRecording)
	{
		m_qwLastProfileTimestamp = GetTickCount64();
	}

	ImGui::SameLine();

	std::ostringstream oss;
	oss << "Time: " << (m_qwLastProfileTimestamp - m_qwStartTimestamp) / 1000.f << "s" << std::endl;

	ImGui::Text(oss.str().c_str());

	ImGui::PopItemWidth();

	ImGui::End();
}