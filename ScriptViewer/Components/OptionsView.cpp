#include <stdafx.h>

#include "OptionsView.h"

void OptionsView::RunImGui()
{
	ImGui::SetNextWindowSize({ 325.f, 55.f });

	ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

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
}