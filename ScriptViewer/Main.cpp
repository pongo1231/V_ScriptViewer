#include <stdafx.h>

#include "Main.h"

#pragma region CONFIG

#define IMGUI_FILENAME NULL

// VK_Control as modifier
#define SCRIPT_WINDOW_OPEN_KEY 0x4F // "O" key

#pragma endregion

static bool ms_bDidInit                     = false;
static bool ms_bDidImguiInit                = false;
static bool ms_bHasMenuOpenStateJustChanged = false;

static bool ms_bIsMenuOpened                = false;

static inline void SetWindowOpenState(bool state)
{
	ms_bIsMenuOpened                = state;

	ms_bHasMenuOpenStateJustChanged = true;
}

static LRESULT (*OG_WndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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
		ImGuiIO &io = ImGui::GetIO();

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

static void **ms_pPresentVftEntryAddr = nullptr;
static HRESULT (*OG_OnPresence)(IDXGISwapChain *pSwapChain, UINT uiSyncInterval, UINT uiFlags);
static HRESULT HK_OnPresence(IDXGISwapChain *pSwapChain, UINT uiSyncInterval, UINT uiFlags)
{
	if (!ms_bDidImguiInit)
	{
		ID3D11Device *pDevice = nullptr;
		pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void **>(&pDevice));

		ID3D11DeviceContext *ms_pDeviceContext = nullptr;
		pDevice->GetImmediateContext(&ms_pDeviceContext);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(Memory::g_hWnd);
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

		if (rage::fwTimer::ms_pbUserPause)
		{
			*rage::fwTimer::ms_pbUserPause = g_bPauseGameOnOverlay;
		}

		ShowCursor(true);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		for (Component *pComponent : g_rgpComponents)
		{
			if (pComponent->m_bIsOpen)
			{
				pComponent->RunImGui();
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

			if (rage::fwTimer::ms_pbUserPause)
			{
				*rage::fwTimer::ms_pbUserPause = false;
			}

			ShowCursor(false);
		}
	}

	return OG_OnPresence(pSwapChain, uiSyncInterval, uiFlags);
}

static void **ms_pResizeBuffersAddr = nullptr;
static HRESULT (*OG_ResizeBuffers)(IDXGISwapChain *pSwapChain, UINT uiBufferCount, UINT uiWidth, UINT uiHeight,
                                   DXGI_FORMAT newFormat, UINT uiSwapChainFlags);
static HRESULT HK_ResizeBuffers(IDXGISwapChain *pSwapChain, UINT uiBufferCount, UINT uiWidth, UINT uiHeight,
                                DXGI_FORMAT newFormat, UINT uiSwapChainFlags)
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

static __int64 (*OG_ScriptRunHook)(rage::scrThread *pScrThread);
static __int64 HK_ScriptRunHook(rage::scrThread *pScrThread)
{
	bool bAbort = false;
	for (Component *pComponent : g_rgpComponents)
	{
		if (!pComponent->RunHook(pScrThread))
		{
			bAbort = true;
		}
	}

	if (bAbort)
	{
		return 0;
	}

	const auto &startTimestamp = Util::GetTimeMcS();

	__int64 result             = OG_ScriptRunHook(pScrThread);

	DWORD64 qwExecutionTime    = Util::GetTimeMcS() - startTimestamp;

	for (Component *pComponent : g_rgpComponents)
	{
		pComponent->RunCallback(pScrThread, qwExecutionTime);
	}

	return OG_ScriptRunHook(pScrThread);
}

void Main::Uninit()
{
	if (ms_bDidImguiInit)
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

	if (ms_pPresentVftEntryAddr)
	{
		Memory::Write(ms_pPresentVftEntryAddr, *OG_OnPresence);
	}

	if (ms_pResizeBuffersAddr)
	{
		Memory::Write(ms_pResizeBuffersAddr, *OG_ResizeBuffers);
	}
}

static bool InitSwapChainHooks()
{
	// Create dummy device and swap chain to patch IDXGISwapChain's vftable
	DXGI_SWAP_CHAIN_DESC swapChainDesc {};

	swapChainDesc.BufferDesc.Width                   = 640;
	swapChainDesc.BufferDesc.Height                  = 480;
	swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapChainDesc.SampleDesc.Count                   = 1;
	swapChainDesc.SampleDesc.Quality                 = 0;

	swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount                        = 1;
	swapChainDesc.OutputWindow                       = Memory::g_hWnd;
	swapChainDesc.Windowed                           = TRUE;
	swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags                              = 0;

	D3D_FEATURE_LEVEL featureLevel                   = D3D_FEATURE_LEVEL_10_0;

	IDXGISwapChain *pSwapChain                       = nullptr;

	auto D3D11CreateDeviceAndSwapChain               = reinterpret_cast<HRESULT (*)(
        IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *, UINT, UINT,
        const DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **)>(
        GetProcAddress(GetModuleHandle(L"d3d11.dll"), "D3D11CreateDeviceAndSwapChain"));

	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION,
	                              &swapChainDesc, &pSwapChain, NULL, NULL, NULL);

	if (!pSwapChain)
	{
		LOG("Swap Chain was invalid, aborting!");

		return false;
	}

	Handle handle           = Handle(*reinterpret_cast<DWORD64 *>(pSwapChain));

	ms_pPresentVftEntryAddr = handle.At(64).Get<void *>();
	OG_OnPresence           = *reinterpret_cast<HRESULT (**)(IDXGISwapChain *, UINT, UINT)>(ms_pPresentVftEntryAddr);

	Memory::Write(ms_pPresentVftEntryAddr, HK_OnPresence);

	LOG("Hooked IDXGISwapChain::Present through vftable injection");

	ms_pResizeBuffersAddr = handle.At(104).Get<void *>();
	OG_ResizeBuffers =
	    *reinterpret_cast<HRESULT (**)(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT)>(ms_pResizeBuffersAddr);

	Memory::Write(ms_pResizeBuffersAddr, HK_ResizeBuffers);

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
		Memory::InitHooks();

		Handle handle;

		// Had no luck with SetWindowsHookEx so we're just going to straight up hook WndProc
		handle = Memory::FindPattern("48 83 EC 28 33 D2 B1 01 E8 ? ? ? ? 83 F8 01 0F 97 C0 48 83 C4 28 C3");
		if (handle.IsValid())
		{
			Memory::AddHook(handle.At(24).Get<void>(), HK_WndProc, &OG_WndProc);

			LOG("Hooked WndProc");
		}

#ifdef RELOADABLE
		handle = Memory::FindPattern(
		    "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC 20 48 8D 81 ? 00 00 00");
#else
		if (getGameVersion() < eGameVersion::VER_1_0_2944_0) // Just a guess, haven't checked if this is actually the
		                                                     // build which broke it
		{
			handle = Memory::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 80 B9 46 01 00 00 00");
		}
		else
		{
			handle = Memory::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 80 B9 4E 01 00 00 00 8B FA");
		}
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
			Memory::FinishHooks();

			if (rage::scrThread::ms_ppThreads && rage::scrThread::ms_pcwThreads)
			{
				ms_bDidInit = true;

				LOG("Completed Init!");
			}
		}
	}

	while (true)
	{
		WAIT(0);

		if (!ms_bDidImguiInit)
		{
			continue;
		}

		if (ms_bIsMenuOpened && (g_bPauseGameOnOverlay || (!g_bPauseGameOnOverlay && g_bBlockKeyboardInputs)))
		{
			DISABLE_ALL_CONTROL_ACTIONS(0);
		}

		for (Component *pComponent : g_rgpComponents)
		{
			pComponent->RunScript();
		}
	}
}