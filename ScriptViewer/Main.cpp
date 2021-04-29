#include "stdafx.h"

#include "Main.h"

#pragma region CONFIG

#define IMGUI_FILENAME NULL

// VK_Control as modifier
#define SCRIPT_WINDOW_OPEN_KEY 0x4F // "O" key

#define SCRIPT_UPDATE_LIST_FREQ 500

#define SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE 64

#pragma endregion

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11DeviceContext* ms_pDeviceContext = nullptr;

static HWND ms_hWnd = 0;

static __int64 ms_dwPresentVftEntryAddr = 0;

static bool* ms_pbUserPause = nullptr;

static rage::scrThread*** ms_pppThreads = nullptr;
static WORD* ms_pcwThreads = nullptr;
static std::vector<std::string> ms_rgszScriptNames;
static std::vector<std::string> ms_rgszBlacklistedScriptNames;
static std::vector<std::string> ms_rgszKillScriptNamesBuffer;
static bool ms_bScriptNamesGathered = false;

static bool ms_bDidInit = false;
static bool ms_bDidImguiInit = false;
static bool ms_bIsMenuOpened = false;
static bool ms_bHasMenuOpenStateJustChanged = false;

static bool ms_bIsNewScriptWindowOpen = false;
static char ms_rgchNewScriptNameBuffer[SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE]{};
static int ms_iNewScriptStackSize = 0;
static bool ms_bDoDispatchNewScript = false;

static inline void SetWindowOpenState(bool state)
{
	ms_bIsMenuOpened = state;

	ms_bHasMenuOpenStateJustChanged = true;
}

static inline bool IsScriptNameBlacklisted(const std::string& szScriptName)
{
	return std::find(ms_rgszBlacklistedScriptNames.begin(), ms_rgszBlacklistedScriptNames.end(), szScriptName)
		!= ms_rgszBlacklistedScriptNames.end();
}

static inline bool IsScriptNameAboutToBeKilled(const std::string& szScriptName)
{
	return std::find(ms_rgszKillScriptNamesBuffer.begin(), ms_rgszKillScriptNamesBuffer.end(), szScriptName)
		!= ms_rgszKillScriptNamesBuffer.end();
}

static inline void ClearNewScriptWindowState()
{
	ms_bIsNewScriptWindowOpen = false;

	memset(ms_rgchNewScriptNameBuffer, 0, SCRIPT_NEW_SCRIPT_NAME_BUFFER_SIZE);

	ms_iNewScriptStackSize = 0;

	ms_bDoDispatchNewScript = false;
}

static LRESULT(*OG_WndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ms_bDidImguiInit && !ms_bDoDispatchNewScript /* Wait for script to dispatch before allowing user to open menu again */)
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

					return TRUE;
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

			return TRUE;
		case WM_KEYUP:
			if (wParam < 512)
			{
				io.KeysDown[wParam] = false;
			}

			return TRUE;
		case WM_CHAR:
			io.AddInputCharacter(wParam);

			break;
		}

		return TRUE;
	}

	return OG_WndProc(hWnd, uMsg, wParam, lParam);
}

static HRESULT(*OG_OnPresence)(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags);
static HRESULT HK_OnPresence(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags)
{
	if (ms_bDidImguiInit)
	{
		if (ms_bIsMenuOpened)
		{
			if (ms_bHasMenuOpenStateJustChanged)
			{
				ms_bHasMenuOpenStateJustChanged = false;
			}

			if (ms_pbUserPause)
			{
				*ms_pbUserPause = true;
			}

			ShowCursor(true);

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			if (ms_bScriptNamesGathered)
			{
				ImGui::Begin("Script Viewer", NULL, ImGuiWindowFlags_NoCollapse);

				ImGui::SetWindowSize({ 400.f, 600.f });

				static int c_iSelectedItem = 0;

				ImGui::PushItemWidth(-1);

				ImGui::ListBoxHeader("Running Scripts", { 0, -30 });
				for (int i = 0; i < ms_rgszScriptNames.size(); i++)
				{
					bool bIsScriptNameAboutToBeKilled = IsScriptNameAboutToBeKilled(ms_rgszScriptNames[i]);
					bool bIsScriptNameBlacklisted = IsScriptNameBlacklisted(ms_rgszScriptNames[i]);

					if (bIsScriptNameAboutToBeKilled)
					{
						ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });
					}
					else if (bIsScriptNameBlacklisted)
					{
						ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 1.f, 0.f, 1.f });
					}

					if (ImGui::Selectable(ms_rgszScriptNames[i].c_str(), i == c_iSelectedItem))
					{
						c_iSelectedItem = i;
					}

					if (bIsScriptNameAboutToBeKilled || bIsScriptNameBlacklisted)
					{
						ImGui::PopStyleColor();
					}
				}
				ImGui::ListBoxFooter();

				ImGui::PopItemWidth();

				ImGui::Spacing();

				const std::string& szSelectedScriptName = ms_rgszScriptNames[c_iSelectedItem];

				bool bIsSelectedScriptUnpausable = szSelectedScriptName.find(".asi") != std::string::npos;
				bool bIsSelectedScriptNameAboutToBeKilled = IsScriptNameAboutToBeKilled(szSelectedScriptName);

				const auto& itSelectedScriptBlacklistedEntry = std::find(ms_rgszBlacklistedScriptNames.begin(), ms_rgszBlacklistedScriptNames.end(), szSelectedScriptName);
				bool bIsSelectedScriptNameBlacklisted = itSelectedScriptBlacklistedEntry != ms_rgszBlacklistedScriptNames.end();

				if (bIsSelectedScriptNameBlacklisted)
				{
					ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 1.f, 0.f, 1.f });
				}

				if (bIsSelectedScriptUnpausable || bIsSelectedScriptNameAboutToBeKilled)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				}

				if (ImGui::Button(bIsSelectedScriptNameBlacklisted ? "Unpause" : "Pause"))
				{
					if (bIsSelectedScriptNameBlacklisted)
					{
						ms_rgszBlacklistedScriptNames.erase(itSelectedScriptBlacklistedEntry);
					}
					else
					{
						ms_rgszBlacklistedScriptNames.push_back(szSelectedScriptName);
					}
				}

				if (bIsSelectedScriptUnpausable)
				{
					ImGui::PopItemFlag();
				}

				if (bIsSelectedScriptNameBlacklisted)
				{
					ImGui::PopStyleColor();
				}

				ImGui::SameLine();

				ImGui::PushStyleColor(ImGui::GetColumnIndex(), { 1.f, 0.f, 0.f, 1.f });

				if (ImGui::Button("Kill"))
				{
					ms_rgszKillScriptNamesBuffer.push_back(ms_rgszScriptNames[c_iSelectedItem]);
				}

				ImGui::PopStyleColor();

				if (bIsSelectedScriptNameAboutToBeKilled)
				{
					ImGui::PopItemFlag();
				}

				ImGui::SameLine();

				// Store current state (so PopItemFlag won't be unjustify called)
				bool bIsNewScriptWindowOpen = ms_bIsNewScriptWindowOpen;

				if (bIsNewScriptWindowOpen)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				}

				if (ImGui::Button("Start New Script"))
				{
					ms_bIsNewScriptWindowOpen = true;
				}

				if (bIsNewScriptWindowOpen)
				{
					ImGui::PopItemFlag();
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

						SetWindowOpenState(false);
					}

					ImGui::End();
				}
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
	}

	return OG_OnPresence(pSwapChain, syncInterval, flags);
}

static __int64(*OG_rage__scrThread__Run)(rage::scrThread* pScrThread);
static __int64 HK_rage__scrThread__Run(rage::scrThread* pScrThread)
{
	return IsScriptNameBlacklisted(pScrThread->m_szName) ? 0 : OG_rage__scrThread__Run(pScrThread);
}

void Main::Init()
{
	if (ms_bDidInit)
	{
		LOG("Script thread has restarted");

		return;
	}

	MH_Initialize();

	Handle handle;

	// Had no luck with SetWindowsHookEx so we're just going to straight up hook WndProc
	handle = Memory::FindPattern("48 8B C4 48 89 58 08 4C 89 48 20 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC F0 00 00 00");
	if (handle.IsValid())
	{
		MH_CreateHook(handle.Get<void>(), HK_WndProc, reinterpret_cast<void**>(&OG_WndProc));

		LOG("Hooked WndProc");
	}

	handle = Memory::FindPattern("E8 ? ? ? ? F3 0F 10 44 24 4C 66 41 C7 46 18 01 00");
	if (handle.IsValid())
	{
		ms_pbUserPause = handle.Into().At(0x5E).At(1).Into().Get<bool>();

		LOG("Found sm_bUserPause");
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

		LOG("Found rage::scrThread::_sm_nThreads");
	}

	handle = Memory::FindPattern("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 20 48 8D 81 D0 00 00 00");
	if (handle.IsValid())
	{
		MH_CreateHook(handle.Get<void>(), HK_rage__scrThread__Run, reinterpret_cast<void**>(&OG_rage__scrThread__Run));

		LOG("Hooked rage::scrThread::Run");
	}

	MH_EnableHook(MH_ALL_HOOKS);

	ms_bDidInit = true;

	LOG("Completed Init!");
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

	Memory::Write<void*>(reinterpret_cast<void**>(ms_dwPresentVftEntryAddr), reinterpret_cast<void*>(*OG_OnPresence));
}

void Main::Loop()
{
	Init();

	while (true)
	{
		WAIT(SCRIPT_UPDATE_LIST_FREQ);

		if (ms_bIsMenuOpened || !ms_bDidImguiInit || !ms_pppThreads || !ms_pcwThreads)
		{
			continue;
		}

		ms_rgszScriptNames.clear();

		rage::scrThread** ppThreads = *ms_pppThreads;

		for (DWORD i = 0; i < *ms_pcwThreads; i++)
		{
			if ((ppThreads[i]->m_dwSomething) /* Check if running */)
			{
				ms_rgszScriptNames.push_back(ppThreads[i]->m_szName);
			}
		}

		ms_bScriptNamesGathered = true;

		if (!ms_rgszKillScriptNamesBuffer.empty())
		{
			for (const std::string& szScriptName : ms_rgszKillScriptNamesBuffer)
			{
				TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME(szScriptName.c_str());
			}

			ms_rgszKillScriptNamesBuffer.clear();
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

		if (ms_bIsMenuOpened)
		{
			DISABLE_ALL_CONTROL_ACTIONS(0);
		}
	}
}

void Main::OnPresence(IDXGISwapChain* pSwapChain)
{
	if (!ms_bDidImguiInit)
	{
		LOG("First IDXGISwapChain::Present cb call, initializing Dear ImGui");

		ms_bDidImguiInit = true;

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		pSwapChain->GetDesc(&swapChainDesc);

		ms_hWnd = swapChainDesc.OutputWindow;

		ms_dwPresentVftEntryAddr = *reinterpret_cast<__int64*>(pSwapChain) + 64;

		OG_OnPresence = *reinterpret_cast<HRESULT(**)(IDXGISwapChain*, UINT, UINT)>(ms_dwPresentVftEntryAddr);

		Memory::Write<void*>(reinterpret_cast<void**>(ms_dwPresentVftEntryAddr), reinterpret_cast<void*>(HK_OnPresence));

		LOG("Hooked IDXGISwapChain::Present through vftable injection");

		ID3D11Device* pDevice = nullptr;
		pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));

		pDevice->GetImmediateContext(&ms_pDeviceContext);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(ms_hWnd);
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

		LOG("Dear ImGui initialized!");
	}
}