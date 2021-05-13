#pragma once

#include <Windows.h>
#include <memory>
#include <map>

struct IDXGISwapChain;

inline char g_szFileName[MAX_PATH];

inline bool g_bIsMenuOpened = false;

inline bool g_bPauseGameOnOverlay = true;
inline bool g_bBlockKeyboardInputs = true;

namespace Main
{
	void Uninit();
	void Loop();
};