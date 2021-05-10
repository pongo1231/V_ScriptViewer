#include "stdafx.h"

#include "Main.h"

#pragma region CONFIG

#define IMGUI_FILENAME NULL

// VK_Control as modifier
#define SCRIPT_WINDOW_OPEN_KEY 0x4F // "O" key

#define SCRIPT_UPDATE_LIST_FREQ 500

#define SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE 64

#define SCRIPT_PROFILING_UPDATE_FREQ 3000

#pragma endregion

static HWND* ms_phWnd = nullptr;

static bool* ms_pbUserPause = nullptr;

static rage::scrThread*** ms_pppThreads = nullptr;
static WORD* ms_pcwThreads = nullptr;

static rage::_ScriptStack** ms_ppStacks = nullptr;
static WORD* ms_pcwStacks = nullptr;

static std::unordered_map<DWORD, ScriptProfile> ms_dcScriptProfiles;

static std::vector<DWORD> ms_rgdwBlacklistedScriptThreadIds;
static std::mutex ms_blacklistedScriptThreadIdsMutex;

static std::atomic<DWORD> ms_dwKillScriptThreadId = 0;

static bool ms_bDidInit = false;
static bool ms_bDidImguiInit = false;
static bool ms_bIsMenuOpened = false;
static bool ms_bHasMenuOpenStateJustChanged = false;

static bool ms_bShowExecutionTimes = false;
static bool ms_bShowStackSizes = false;
static bool ms_bPauseGameOnOverlay = true;
static bool ms_bBlockKeyboardInputs = true;

static bool ms_bIsNewScriptWindowOpen = false;
static char ms_rgchNewScriptNameBuffer[SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE]{};
static int ms_iNewScriptStackSize = 0;
static bool ms_bDoDispatchNewScript = false;

static inline void SetWindowOpenState(bool state)
{
	ms_bIsMenuOpened = state;

	ms_bHasMenuOpenStateJustChanged = true;
}

static inline [[nodiscard]] bool IsScriptThreadIdPaused(DWORD dwThreadId)
{
	std::lock_guard lock(ms_blacklistedScriptThreadIdsMutex);

	return std::find(ms_rgdwBlacklistedScriptThreadIds.begin(), ms_rgdwBlacklistedScriptThreadIds.end(), dwThreadId) != ms_rgdwBlacklistedScriptThreadIds.end();
}

static inline void PauseScriptThreadId(DWORD dwThreadId)
{
	std::lock_guard lock(ms_blacklistedScriptThreadIdsMutex);

	ms_rgdwBlacklistedScriptThreadIds.push_back(dwThreadId);
}

static inline void UnpauseScriptThreadId(DWORD dwThreadId)
{
	std::lock_guard lock(ms_blacklistedScriptThreadIdsMutex);

	const auto& itScriptThreadId = std::find(ms_rgdwBlacklistedScriptThreadIds.begin(), ms_rgdwBlacklistedScriptThreadIds.end(), dwThreadId);

	if (itScriptThreadId != ms_rgdwBlacklistedScriptThreadIds.end())
	{
		ms_rgdwBlacklistedScriptThreadIds.erase(itScriptThreadId);
	}
}

static inline void ClearNewScriptWindowState()
{
	ms_bIsNewScriptWindowOpen = false;

	memset(ms_rgchNewScriptNameBuffer, 0, SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE);

	ms_iNewScriptStackSize = 0;

	ms_bDoDispatchNewScript = false;
}

static inline [[nodiscard]] rage::_ScriptStack* GetScriptStack(rage::scrThread* pThread)
{
	rage::_ScriptStack* pStacks = *ms_ppStacks;

	for (WORD wStackIdx = 0; wStackIdx < *ms_pcwStacks; wStackIdx++)
	{
		if (pStacks[wStackIdx].m_pScrThread == pThread)
		{
			return &pStacks[wStackIdx];
		}
	}

	return nullptr;
}

static LRESULT(*OG_WndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ms_bDidImguiInit)
	{
		static bool c_bIsCtrlPressed = false;

		switch (uMsg)
		{
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_CONTROL:
				c_bIsCtrlPressed = true;

				break;
			case SCRIPT_WINDOW_OPEN_KEY:
				if (c_bIsCtrlPressed)
				{
					SetWindowOpenState(!ms_bIsMenuOpened);
				}

				break;
			}

			break;
		case WM_KEYUP:
			switch (wParam)
			{
			case VK_CONTROL:
				c_bIsCtrlPressed = false;

				break;
			}

			break;
		}
	}

	if (ms_bIsMenuOpened)
	{
		ImGuiIO& io = ImGui::GetIO();

		switch (uMsg)
		{
		case WM_LBUTTONDOWN:
			io.MouseDown[0] = true;

			break;
		case WM_LBUTTONUP:
			io.MouseDown[0] = false;

			break;
		case WM_MOUSEWHEEL:
			io.MouseWheel = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

			break;
		case WM_KEYDOWN:
			if (wParam < 512)
			{
				io.KeysDown[wParam] = true;
			}

			break;
		case WM_KEYUP:
			if (wParam < 512)
			{
				io.KeysDown[wParam] = false;
			}

			break;
		case WM_CHAR:
			io.AddInputCharacter(wParam);

			break;
		}
	}

	return OG_WndProc(hWnd, uMsg, wParam, lParam);
}

static void** ms_pPresentVftEntryAddr = nullptr;
static HRESULT(*OG_OnPresence)(IDXGISwapChain* pSwapChain, UINT uiSyncInterval, UINT uiFlags);
static HRESULT HK_OnPresence(IDXGISwapChain* pSwapChain, UINT uiSyncInterval, UINT uiFlags)
{
	if (!ms_bDidImguiInit)
	{
		ID3D11Device* pDevice = nullptr;
		pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		pSwapChain->GetDesc(&swapChainDesc);

		ID3D11DeviceContext* ms_pDeviceContext = nullptr;
		pDevice->GetImmediateContext(&ms_pDeviceContext);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(swapChainDesc.OutputWindow);
		ImGui_ImplDX11_Init(pDevice, ms_pDeviceContext);

		ImGui::GetIO().IniFilename = IMGUI_FILENAME;

		if (!IMGUI_FILENAME)
		{
			LOG("Not saving Dear ImGui settings");
		}
		else
		{
			LOG("Saving Dear ImGui settings to " << IMGUI_FILENAME);
		}

		ms_bDidImguiInit = true;

		LOG("Dear ImGui initialized!");
	}
	else if (ms_bIsMenuOpened)
	{
		if (ms_bHasMenuOpenStateJustChanged)
		{
			ms_bHasMenuOpenStateJustChanged = false;
		}

		if (ms_pbUserPause)
		{
			*ms_pbUserPause = ms_bPauseGameOnOverlay;
		}

		ShowCursor(true);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 400.f, 600.f });

		ImGui::Begin("Script Viewer", NULL, ImGuiWindowFlags_NoCollapse);

		ImGui::PopStyleVar();

		static int c_iSelectedItem = 0;

		ImGui::PushItemWidth(-1);

		rage::scrThread** ppThreads = *ms_pppThreads;

		if (ImGui::ListBoxHeader("", { 0, -96.f }))
		{
			DWORD64 qwTimestamp = GetTickCount64();

			static DWORD64 c_qwLastProfileUpdatedTimestamp = qwTimestamp;

			bool bDoNewProfileRound = false;
			if (qwTimestamp > c_qwLastProfileUpdatedTimestamp)
			{
				c_qwLastProfileUpdatedTimestamp = qwTimestamp + SCRIPT_PROFILING_UPDATE_FREQ;

				bDoNewProfileRound = true;
			}

			for (WORD wScriptIdx = 0; wScriptIdx < *ms_pcwThreads; wScriptIdx++)
			{
				rage::scrThread* pThread = ppThreads[wScriptIdx];

				if (!pThread->m_dwThreadId)
				{
					continue;
				}

				const char* szScriptName = pThread->m_szName;
				DWORD dwThreadId = pThread->m_dwThreadId;

				bool bIsScriptAboutToBeKilled = ms_dwKillScriptThreadId == dwThreadId;
				bool bIsScriptPaused = IsScriptThreadIdPaused(dwThreadId);

				if (bIsScriptAboutToBeKilled)
				{
					ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });
				}
				else if (bIsScriptPaused)
				{
					ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 1.f, 0.f, 1.f });
				}

				ScriptProfile& scriptProfile = ms_dcScriptProfiles[dwThreadId];

				if (bDoNewProfileRound)
				{
					scriptProfile.NewRound();
				}

				bool bIsCustomThread = !strcmp(szScriptName, "control_thread") || strstr(szScriptName, ".asi");

				std::ostringstream ossScriptText;
				ossScriptText << szScriptName;

#ifdef RELOADABLE
				if (!bIsCustomThread)
#endif
				{
					if (ms_bShowExecutionTimes)
					{
						ossScriptText << " | " << scriptProfile.Get() << " ms";
					}
				}

				if (!bIsCustomThread && ms_bShowStackSizes)
				{
					rage::_ScriptStack* pStack = GetScriptStack(pThread);

					if (pStack)
					{
						ossScriptText << " | Stack Size: " << pStack->m_dwStackSize;
					}
				}

				if (ImGui::Selectable(ossScriptText.str().c_str(), wScriptIdx == c_iSelectedItem))
				{
					c_iSelectedItem = wScriptIdx;
				}

				if (bIsScriptAboutToBeKilled || bIsScriptPaused)
				{
					ImGui::PopStyleColor();
				}
			}

			ImGui::ListBoxFooter();
		}

		ImGui::PopItemWidth();

		ImGui::Spacing();

		std::string szSelectedScriptName = ppThreads[c_iSelectedItem]->m_szName;
		DWORD dwSelectedThreadId = ppThreads[c_iSelectedItem]->m_dwThreadId;

#ifdef RELOADABLE
		bool bIsSelectedScriptUnpausable = szSelectedScriptName == "control_thread" || szSelectedScriptName.find(".asi") != std::string::npos;
#else
		bool bIsSelectedScriptUnpausable = !_stricmp(szSelectedScriptName.c_str(), g_szFileName);
#endif
		bool bIsAnyScriptAboutToBeKilled = ms_dwKillScriptThreadId;

		bool bIsSelectedScriptPaused = IsScriptThreadIdPaused(dwSelectedThreadId);

		if (bIsSelectedScriptPaused)
		{
			ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 1.f, 0.f, 1.f });
		}

		if (bIsSelectedScriptUnpausable || bIsAnyScriptAboutToBeKilled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}

		if (ImGui::Button(bIsSelectedScriptPaused ? "Unpause" : "Pause"))
		{
			if (bIsSelectedScriptPaused)
			{
				UnpauseScriptThreadId(dwSelectedThreadId);
			}
			else
			{
				PauseScriptThreadId(dwSelectedThreadId);
			}
		}

		if (bIsSelectedScriptUnpausable)
		{
			ImGui::PopItemFlag();
		}

		if (bIsSelectedScriptPaused)
		{
			ImGui::PopStyleColor();
		}

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });

		if (ImGui::Button("Kill"))
		{
			ms_dwKillScriptThreadId = dwSelectedThreadId;
		}

		ImGui::PopStyleColor();

		if (bIsAnyScriptAboutToBeKilled && !bIsSelectedScriptUnpausable)
		{
			ImGui::PopItemFlag();
		}

		ImGui::SameLine();

		// Store current state (so PopItemFlag won't be unjustify called)
		bool bIsNewScriptWindowOpen = ms_bIsNewScriptWindowOpen;

		if (bIsNewScriptWindowOpen || ms_bDoDispatchNewScript)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		}

		if (ImGui::Button("Start New Script"))
		{
			ms_bIsNewScriptWindowOpen = true;
		}

		if (bIsNewScriptWindowOpen || ms_bDoDispatchNewScript)
		{
			ImGui::PopItemFlag();
		}

		if (ImGui::Button(ms_bShowExecutionTimes ? "Show Execution Times: On" : "Show Execution Times: Off"))
		{
			ms_bShowExecutionTimes = !ms_bShowExecutionTimes;
		}

		if (ms_bShowExecutionTimes)
		{
			ImGui::SameLine();

			if (ImGui::Button(ScriptProfile::ms_eProfileType == eScriptProfileType::HIGHEST_TIME ? "Profile Type: Highest Time"
				: "Profile Type: Average Time"))
			{
				switch (ScriptProfile::ms_eProfileType)
				{
				case eScriptProfileType::HIGHEST_TIME:
					ScriptProfile::ms_eProfileType = eScriptProfileType::AVERAGE_TIME;

					break;
				case eScriptProfileType::AVERAGE_TIME:
					ScriptProfile::ms_eProfileType = eScriptProfileType::HIGHEST_TIME;

					break;
				}
			}
		}

		if (ms_ppStacks && ms_pcwStacks && ImGui::Button(ms_bShowStackSizes ? "Show Stack Sizes: On" : "Show Stack Sizes: Off"))
		{
			ms_bShowStackSizes = !ms_bShowStackSizes;
		}

		if (ms_pbUserPause)
		{
			if (ImGui::Button(ms_bPauseGameOnOverlay ? "Pause Game: On" : "Pause Game: Off"))
			{
				ms_bPauseGameOnOverlay = !ms_bPauseGameOnOverlay;
			}

			ImGui::SameLine();
		}

		if (!ms_pbUserPause || !ms_bPauseGameOnOverlay)
		{
			if (ImGui::Button(ms_bBlockKeyboardInputs ? "Block Keyboard Inputs: On" : "Block Keyboard Inputs: Off"))
			{
				ms_bBlockKeyboardInputs = !ms_bBlockKeyboardInputs;
			}
		}

		ImGui::End();

		if (ms_bIsNewScriptWindowOpen)
		{
			ImGui::Begin("New Script", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

			ImGui::SetWindowSize({ 400.f, 100.f });

			ImGui::InputText("Script Name", ms_rgchNewScriptNameBuffer, SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE);

			ImGui::InputInt("Script Stack Size", &ms_iNewScriptStackSize);

			if (ImGui::Button("Cancel"))
			{
				ClearNewScriptWindowState();
			}

			ImGui::SameLine();

			if (ImGui::Button("Start"))
			{
				ms_bDoDispatchNewScript = true;

				ms_bIsNewScriptWindowOpen = false;
			}

			ImGui::End();
		}

		ImGui::Render();

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	else
	{
		if (ms_bHasMenuOpenStateJustChanged)
		{
			ms_bHasMenuOpenStateJustChanged = false;

			if (ms_pbUserPause)
			{
				*ms_pbUserPause = false;
			}

			ShowCursor(false);
		}
	}

	return OG_OnPresence(pSwapChain, uiSyncInterval, uiFlags);
}

static void** ms_pResizeBuffersAddr = nullptr;
static HRESULT(*OG_ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT uiBufferCount, UINT uiWidth, UINT uiHeight, DXGI_FORMAT newFormat, UINT uiSwapChainFlags);
static HRESULT HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT uiBufferCount, UINT uiWidth, UINT uiHeight, DXGI_FORMAT newFormat, UINT uiSwapChainFlags)
{
	if (ms_bDidImguiInit)
	{
		ImGui_ImplDX11_InvalidateDeviceObjects();
	}

	HRESULT hResult = OG_ResizeBuffers(pSwapChain, uiBufferCount, uiWidth, uiHeight, newFormat, uiSwapChainFlags);

	if (ms_bDidImguiInit)
	{
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	return hResult;
}

static __int64(*OG_ScriptRunHook)(rage::scrThread* pScrThread);
static __int64 HK_ScriptRunHook(rage::scrThread* pScrThread)
{
	if (IsScriptThreadIdPaused(pScrThread->m_dwThreadId))
	{
		return 0;
	}
	else if (ms_bIsMenuOpened)
	{
		ScriptProfile& scriptProfile = ms_dcScriptProfiles[pScrThread->m_dwThreadId];
		
		const auto& startTimestamp = std::chrono::high_resolution_clock::now();

		__int64 result = OG_ScriptRunHook(pScrThread);

		DWORD64 qwExecutionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTimestamp).count();

		scriptProfile.Add(qwExecutionTime);
	
		return result;
	}

	return OG_ScriptRunHook(pScrThread);
}

void Main::Uninit()
{
	if (ms_bDidImguiInit)
	{
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX11_Shutdown();

		ImGui::DestroyContext();
	}

	MH_Uninitialize();

	if (ms_pPresentVftEntryAddr)
	{
		Memory::Write<void*>(ms_pPresentVftEntryAddr, *OG_OnPresence);
	}

	if (ms_pResizeBuffersAddr)
	{
		Memory::Write<void*>(ms_pResizeBuffersAddr, *OG_ResizeBuffers);
	}
}

static bool InitSwapChainHooks()
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};

	swapChainDesc.BufferDesc.Width = 640;
	swapChainDesc.BufferDesc.Height = 480;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = *ms_phWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;

	IDXGISwapChain* pSwapChain = nullptr;

	auto D3D11CreateDeviceAndSwapChain = reinterpret_cast<HRESULT(*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
		const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**)>
		(GetProcAddress(GetModuleHandle("d3d11.dll"), "D3D11CreateDeviceAndSwapChain"));

	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, NULL, NULL, NULL);

	if (!pSwapChain)
	{
		LOG("Swap Chain was invalid, aborting!");

		return false;
	}

	Handle handle = Handle(*reinterpret_cast<DWORD64*>(pSwapChain));

	ms_pPresentVftEntryAddr = handle.At(64).Get<void*>();
	OG_OnPresence = *reinterpret_cast<HRESULT(**)(IDXGISwapChain*, UINT, UINT)>(ms_pPresentVftEntryAddr);

	Memory::Write<void*>(ms_pPresentVftEntryAddr, HK_OnPresence);

	LOG("Hooked IDXGISwapChain::Present through vftable injection");

	ms_pResizeBuffersAddr = handle.At(104).Get<void*>();
	OG_ResizeBuffers = *reinterpret_cast<HRESULT(**)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)>(ms_pResizeBuffersAddr);

	Memory::Write<void*>(ms_pResizeBuffersAddr, HK_ResizeBuffers);

	LOG("Hooked IDXGISwapChain::ResizeBuffers through vftable injection");

	pSwapChain->Release();

	return true;
}

void Main::Loop()
{
	if (ms_bDidInit)
	{
		LOG("Script thread has restarted");
	}
	else
	{
		MH_Initialize();

		Handle handle;

		handle = Memory::FindPattern("E8 ? ? ? ? 45 8D 46 2F");
		if (handle.IsValid())
		{
			ms_phWnd = handle.Into().At(0x79).Get<HWND>();

			LOG("Found hWnd");
		}
		else
		{
			LOG("hWnd not found, aborting!");

			return;
		}

		// Had no luck with SetWindowsHookEx so we're just going to straight up hook WndProc
		handle = Memory::FindPattern("48 8D 05 ? ? ? ? 33 C9 44 89 75 20");
		if (handle.IsValid())
		{
			Memory::AddHook(handle.At(2).Into().Get<void>(), HK_WndProc, &OG_WndProc);

			LOG("Hooked WndProc");
		}

		handle = Memory::FindPattern("E8 ? ? ? ? F3 0F 10 44 24 4C 66 41 C7 46 18 01 00");
		if (handle.IsValid())
		{
			ms_pbUserPause = handle.Into().At(0x5E).At(1).Into().Get<bool>();

			LOG("Found rage::fwTimer::sm_bUserPause");
		}

		handle = Memory::FindPattern("48 8B 05 ? ? ? ? 48 89 0C 06");
		if (handle.IsValid())
		{
			ms_pppThreads = handle.At(2).Into().Get<rage::scrThread**>();

			LOG("Found rage::scrThread::sm_Threads");
		}

		handle = Memory::FindPattern("66 89 3D ? ? ? ? 85 F6");
		if (handle.IsValid())
		{
			ms_pcwThreads = handle.At(2).Into().Get<WORD>();

			LOG("Found rage::scrThread::_sm_cwThreads");
		}

		handle = Memory::FindPattern("48 89 05 ? ? ? ? EB 07 48 89 1D ? ? ? ? 66 89 35 ? ? ? ? 85 FF");
		if (handle.IsValid())
		{
			ms_ppStacks = handle.At(2).Into().Get<rage::_ScriptStack*>();

			LOG("Found rage::scrThread::sm_Stacks");
		}

		handle = Memory::FindPattern("66 89 35 ? ? ? ? 85 FF");
		if (handle.IsValid())
		{
			ms_pcwStacks = handle.At(2).Into().Get<WORD>();

			LOG("Found rage::scrThread::_sm_cwStacks");
		}

#ifdef RELOADABLE
		handle = Memory::FindPattern("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 20 48 8D 81 D0 00 00 00");
#else
		handle = Memory::FindPattern("80 3D ? ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 59 0D");
#endif
		if (handle.IsValid())
		{
			Memory::AddHook(handle.Get<void>(), HK_ScriptRunHook, &OG_ScriptRunHook);

#ifdef RELOADABLE
			LOG("Hooked rage::scrThread::Run");
#else
			LOG("Hooked rage::scrThread::Update");
#endif
		}

		if (InitSwapChainHooks())
		{
			MH_EnableHook(MH_ALL_HOOKS);

			if (ms_pppThreads && ms_pcwThreads)
			{
				ms_bDidInit = true;

				LOG("Completed Init!");
			}
		}
	}

	while (true)
	{
		WAIT(SCRIPT_UPDATE_LIST_FREQ);

		if (!ms_bDidImguiInit)
		{
			continue;
		}

		if (ms_dwKillScriptThreadId)
		{
			rage::scrThread** ppThreads = *ms_pppThreads;

			for (WORD wScriptIdx = 0; wScriptIdx < *ms_pcwThreads; wScriptIdx++)
			{
				if (ppThreads[wScriptIdx]->m_dwThreadId == ms_dwKillScriptThreadId)
				{
					ppThreads[wScriptIdx]->Kill();

					break;
				}
			}

			ms_dwKillScriptThreadId = 0;
		}

		if (ms_bDoDispatchNewScript)
		{
			if (ms_rgchNewScriptNameBuffer[0] && ms_iNewScriptStackSize >= 0 && DOES_SCRIPT_EXIST(ms_rgchNewScriptNameBuffer))
			{
				REQUEST_SCRIPT(ms_rgchNewScriptNameBuffer);

				while (!HAS_SCRIPT_LOADED(ms_rgchNewScriptNameBuffer))
				{
					WAIT(0);
				}

				START_NEW_SCRIPT(ms_rgchNewScriptNameBuffer, ms_iNewScriptStackSize);

				SET_SCRIPT_AS_NO_LONGER_NEEDED(ms_rgchNewScriptNameBuffer);
			}

			ClearNewScriptWindowState();
		}
	}
}

void Main::LoopWindowActionsBlock()
{
	while (true)
	{
		WAIT(0);

		if (ms_bIsMenuOpened && (ms_bPauseGameOnOverlay || (!ms_bPauseGameOnOverlay && ms_bBlockKeyboardInputs)))
		{
			DISABLE_ALL_CONTROL_ACTIONS(0);
		}
	}
}