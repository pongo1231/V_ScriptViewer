#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

namespace Util
{
	inline [[nodiscard]] bool IsCustomScriptName(const std::string& szScriptName)
	{
		return !szScriptName.c_str() || szScriptName == "control_thread" || szScriptName.find(".asi") != std::string::npos;
	}

	inline [[nodiscard]] DWORD64 GetTimeMcS()
	{
		LARGE_INTEGER ticks;
		QueryPerformanceCounter(&ticks);

		return ticks.QuadPart;
	}
}