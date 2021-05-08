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
		LOG(Memory::FindPattern("88 05 ? ? ? ? E8 ? ? ? ? 48 8B CB 48 8B 5C 24 30").At(1).Into().Value<bool>());

		Main::Uninit();

		Memory::Uninit();

		scriptUnregister(hInstance);

		presentCallbackUnregister(OnPresenceCallback);

		break;
	}

	return TRUE;
}