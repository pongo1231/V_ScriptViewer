#include <stdafx.h>

static void OnHook()
{
	Handle handle;

	handle = Memory::FindPattern("48 8B 05 ? ? ? ? 8B CB 48 8B 0C C8 83 79 10 02");
	if (handle.IsValid())
	{
		rage::scrThread::ms_ppThreads = handle.At(2).Into().Value<rage::scrThread**>();

		LOG("Found rage::scrThread::sm_Threads");
	}

	handle = Memory::FindPattern("66 3B 2D ? ? ? ? 8B F5 8B FD 73 46");
	if (handle.IsValid())
	{
		rage::scrThread::ms_pcwThreads = handle.At(2).Into().Get<WORD>();

		LOG("Found rage::scrThread::_sm_cwThreads");
	}

	handle = Memory::FindPattern("48 8B 1D ? ? ? ? 48 C7 C1 FF FF FF FF 4B 8D 3C 40 66 45 03 C7");
	if (handle.IsValid())
	{
		rage::scrThread::ms_pStacks = handle.At(2).Into().Value<rage::_scrStack*>();

		LOG("Found rage::scrThread::sm_Stacks");
	}

	handle = Memory::FindPattern("66 89 35 ? ? ? ? 85 FF"); // Still broken on 2545 :(
	if (handle.IsValid())
	{
		rage::scrThread::ms_pcwStacks = handle.At(2).Into().Get<WORD>();

		LOG("Found rage::scrThread::_sm_cwStacks");
	}
}

static RegisterHook hook(OnHook);