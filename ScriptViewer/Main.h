#pragma once

#include <Windows.h>
#include <memory>
#include <map>

struct IDXGISwapChain;

namespace Main
{
	void Uninit();
	void Loop();
	void LoopWindowActionsBlock();
	void OnPresenceCallback(IDXGISwapChain* swapChain);
};