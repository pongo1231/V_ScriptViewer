#include <stdafx.h>

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD_t reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		GetModuleFileName(hInstance, g_szFileName, MAX_PATH);
		wcscpy_s(g_szFileName, wcsrchr(g_szFileName, '\\') + 1);

		Memory::Init();

		scriptRegister(hInstance, Main::Loop);

		break;
	case DLL_PROCESS_DETACH:
		Main::Uninit();

		Memory::Uninit();

		scriptUnregister(hInstance);

		break;
	}

	return TRUE;
}