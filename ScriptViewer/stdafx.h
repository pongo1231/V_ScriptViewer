#pragma once

#include "nativesNoNamespaces.h"
#include "Main.h"

#include "Components/Component.h"
#include "Components/ComponentView.h"
#include "Components/ScriptView.h"
#include "Components/GlobalView.h"
#include "Components/RecordView.h"
#include "Components/OptionsView.h"

#include "Memory/Memory.h"
#include "Memory/Handle.h"
#include "Memory/Hook.h"
#include "Memory/Hwnd.h"
#include "Memory/ScriptRoutineTracer.h"

#include "Util/Logging.h"
#include "Util/Misc.h"

#include "Lib/scrThread.h"
#include "Lib/fwTimer.h"

#include "../vendor/scripthookv/inc/main.h"
#include "../vendor/scripthookv/inc/natives.h"

#include "../vendor/minhook/include/MinHook.h"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"
#include "../vendor/imgui/backends/imgui_impl_dx11.h"
#include "../vendor/imgui/backends/imgui_impl_win32.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Psapi.h>

#include <dxgi.h>
#include <d3d11.h>

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <mutex>