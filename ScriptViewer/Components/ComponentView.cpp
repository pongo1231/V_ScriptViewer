#include <stdafx.h>

#include "ComponentView.h"

ComponentView::ComponentView() : Component()
{
	m_bIsOpen = true;

	// Avoid unnecessary moves
	m_rgComponents.reserve(3);

	m_rgComponents.emplace_back("Script Viewer", std::make_unique<ScriptView>());

	// m_rgComponents.emplace_back("Global Editor", std::make_unique<GlobalView>());

	m_rgComponents.emplace_back("Record View", std::make_unique<RecordView>());

	m_rgComponents.emplace_back("Options", std::make_unique<OptionsView>());
}

void ComponentView::RunImGui()
{
	ImGui::SetNextWindowPos({ Memory::g_fWndInitialHorizontalCenter, 100.f }, ImGuiCond_Once);

	ImGui::Begin("Components", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

	for (ComponentEntry &componentEntry : m_rgComponents)
	{
		if (componentEntry.m_pComponent->m_bIsOpen)
		{
			ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 0.f, 1.f, 0.f, 1.f });
		}
		else
		{
			ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });
		}

		if (ImGui::Button(componentEntry.m_szComponentName))
		{
			componentEntry.m_pComponent->m_bIsOpen = !componentEntry.m_pComponent->m_bIsOpen;
		}

		ImGui::PopStyleColor();
	}

	ImGui::End();
}