#include <stdafx.h>

#include "ComponentView.h"

ComponentView::ComponentView() : Component()
{
	m_bIsOpen = true;

	m_rgComponents.emplace_back("Script Viewer", std::make_unique<ScriptView>());
}

void ComponentView::RunImGui()
{
	ImGui::SetNextWindowPos({ Memory::g_fWndInitialHorizontalCenter, 100.f }, ImGuiCond_Once);

	ImGui::Begin("Components", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

	for (ComponentEntry& componentEntry : m_rgComponents)
	{
		if (ImGui::Button(componentEntry.m_szComponentName))
		{
			componentEntry.m_pComponent->m_bIsOpen = !componentEntry.m_pComponent->m_bIsOpen;
		}
	}

	ImGui::End();
}