#include <stdafx.h>

#include "Hwnd.h"

static void OnHook()
{
	Handle handle;

	handle = Memory::FindPattern("E8 ? ? ? ? 45 8D 46 2F");
	if (handle.IsValid())
	{
		Memory::g_hWnd = handle.Into().At(0x79).At(2).Into().Value<HWND>();

		LOG("Found hWnd");
	}
}

static RegisterHook hook(OnHook);