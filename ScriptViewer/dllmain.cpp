#include <stdafx.h>

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

		break;
	case DLL_PROCESS_DETACH:
		Main::Uninit();

		Memory::Uninit();

		scriptUnregister(hInstance);

		break;
	}

	return TRUE;
}