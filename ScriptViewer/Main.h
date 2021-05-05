#pragma once

#include <Windows.h>
#include <memory>
#include <map>

struct IDXGISwapChain;

inline char g_szFileName[MAX_PATH];

namespace Main
{
	void Uninit();
	void Loop();
	void LoopWindowActionsBlock();
	void OnPresenceCallback(IDXGISwapChain* swapChain);
};