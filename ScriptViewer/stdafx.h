#pragma once

typedef unsigned int DWORD_t;

#include "Main.h"
#include "nativesNoNamespaces.h"

#include "Components/Component.h"
#include "Components/ComponentView.h"
#include "Components/GlobalView.h"
#include "Components/OptionsView.h"
#include "Components/RecordView.h"
#include "Components/ScriptView.h"

#include "Memory/Handle.h"
#include "Memory/Hook.h"
#include "Memory/Hwnd.h"
#include "Memory/Memory.h"
#include "Memory/ScriptRoutineTracer.h"

#include "Util/Logging.h"
#include "Util/Misc.h"

#include "Lib/fwTimer.h"
#include "Lib/scrThread.h"

#include "../vendor/scripthookv/main.h"
#include "../vendor/scripthookv/natives.h"

#include "../vendor/minhook/include/MinHook.h"

#include "../vendor/imgui/backends/imgui_impl_dx11.h"
#include "../vendor/imgui/backends/imgui_impl_win32.h"
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <excpt.h>
#include <psapi.h>
#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#include <codecvt>
#include <fstream>
#include <locale>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>