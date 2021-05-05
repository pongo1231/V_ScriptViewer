#include <stdafx.h>

static void OnPresenceCallback(void* swapChain)
{
	Main::OnPresenceCallback(static_cast<IDXGISwapChain*>(swapChain));
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		GetModuleFileName(hInstance, g_szFileName, MAX_PATH);
		strcpy_s(g_szFileName, strrchr(g_szFileName, '\\') + 1);

		Memory::Init();

		scriptRegister(hInstance, Main::Loop);

		scriptRegisterAdditionalThread(hInstance, Main::LoopWindowActionsBlock);

		presentCallbackRegister(OnPresenceCallback);

		break;
	case DLL_PROCESS_DETACH:
		Main::Uninit();

		Memory::Uninit();

		scriptUnregister(hInstance);

		presentCallbackUnregister(OnPresenceCallback);

		break;
	}

	return TRUE;
}