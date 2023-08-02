#pragma once

#include <map>
#include <memory>
#include <windows.h>

struct IDXGISwapChain;

inline wchar_t g_szFileName[MAX_PATH];

inline bool g_bPauseGameOnOverlay  = true;
inline bool g_bBlockKeyboardInputs = true;

namespace Main
{
	void Uninit();
	void Loop();
};