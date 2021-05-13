#include <stdafx.h>

#include "fwTimer.h"

static void OnHook()
{
	Handle handle;

	handle = Memory::FindPattern("E8 ? ? ? ? F3 0F 10 44 24 4C 66 41 C7 46 18 01 00");
	if (handle.IsValid())
	{
		rage::fwTimer::ms_pbUserPause = handle.Into().At(0x5E).At(1).Into().Get<bool>();

		LOG("Found rage::fwTimer::sm_bUserPause");
	}
}

static RegisterHook hook(OnHook);