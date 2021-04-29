#include <stdafx.h>

static void OnPresence(void* swapChain)
{
	Main::OnPresence(static_cast<IDXGISwapChain*>(swapChain));
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		Memory::Init();

		scriptRegister(hInstance, Main::Loop);

		scriptRegisterAdditionalThread(hInstance, Main::LoopWindowActionsBlock);

		presentCallbackRegister(OnPresence);

		break;
	case DLL_PROCESS_DETACH:
		Main::Uninit();

		Memory::Uninit();

		scriptUnregister(hInstance);

		presentCallbackUnregister(OnPresence);

		break;
	}

	return TRUE;
}