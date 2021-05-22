#pragma once

#include <string>

namespace Util
{
	inline [[nodiscard]] bool IsCustomScriptName(const std::string& szScriptName)
	{
		return !szScriptName.c_str() || szScriptName == "control_thread" || szScriptName.find(".asi") != std::string::npos;
	}
}