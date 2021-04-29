#pragma once

#include <Windows.h>
#include <memory>
#include <map>

struct IDXGISwapChain;

namespace Main
{
	void Init();
	void Uninit();
	void Loop();
	void LoopWindowActionsBlock();
	void OnPresence(IDXGISwapChain* swapChain);
};