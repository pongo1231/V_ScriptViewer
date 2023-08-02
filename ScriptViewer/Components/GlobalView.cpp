#include <stdafx.h>

#include "GlobalView.h"

static __int64 *ms_pGlobals = nullptr;
static WORD *ms_pcwGlobals  = nullptr;

void GlobalView::RunImGui()
{
	ImGui::Begin("Global Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("Nothing to find here yet :)");

	if (ms_pGlobals && ms_pcwGlobals)
	{
		ImGui::ListBoxHeader("");

		for (WORD wGlobalIdx = 0; wGlobalIdx < *ms_pcwGlobals; wGlobalIdx++)
		{
			std::ostringstream oss;

			oss << "Offset: " << wGlobalIdx << " | " << ms_pGlobals[wGlobalIdx] << std::endl;

			if (ImGui::Selectable(oss.str().c_str()))
			{
			}
		}

		ImGui::ListBoxFooter();
	}

	ImGui::End();
}

void GlobalView::RunScript()
{
	/*static bool c_bFirstTime = true;
	if (c_bFirstTime)
	{
	    c_bFirstTime = false;

	    Handle handle;

	    handle = Memory::FindPattern("48 89 05 ? ? ? ? EB 07 48 89 1D ? ? ? ? 66 89 35 ? ? ? ? 85 FF");

	    ms_pGlobals = handle.At(2).Into().At(16).Value<__int64*>();

	    LOG("Found rage::scrThread::sm_Globals");

	    ms_pcwGlobals = handle.At(2).Into().At(24).Get<WORD>();

	    LOG("Found rage::scrThread::_sm_cwGlobals");
	}

	if (!ms_pGlobals || !ms_pcwGlobals)
	{
	    return;
	}*/
}