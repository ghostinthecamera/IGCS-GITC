#include "stdafx.h"
#include <d3d11.h> 
#include "UniversalD3D11Hook.h"
#include "Console.h"
#include "MinHook.h"
#include "Globals.h"
#include "imgui.h"
#include "Defaults.h"
#include "imgui_impl_dx11.h"
#include "OverlayControl.h"
#include "OverlayConsole.h"

#pragma comment(lib, "d3d11.lib")

namespace IGCS::DX11Hooker
{
	#define DXGI_PRESENT_INDEX			8
	#define DXGI_RESIZEBUFFERS_INDEX	13

	//--------------------------------------------------------------------------------------------------------------------------------
	// Forward declarations
	void createRenderTarget(IDXGISwapChain* pSwapChain);
	void cleanupRenderTarget();
	void initImGui();
	void initImGuiStyle();

	//--------------------------------------------------------------------------------------------------------------------------------
	// Typedefs of functions to hook
	typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	typedef HRESULT(__stdcall *D3D11ResizeBuffersHook) (IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);

	static ID3D11Device* _device = nullptr;
	static ID3D11DeviceContext* _context = nullptr;
	static ID3D11RenderTargetView* _mainRenderTargetView = nullptr;

	//--------------------------------------------------------------------------------------------------------------------------------
	// Pointers to the original hooked functions
	static D3D11PresentHook hookedD3D11Present = nullptr;
	static D3D11ResizeBuffersHook hookedD3D11ResizeBuffers = nullptr;

	static bool _tmpSwapChainInitialized = false;
	static bool _showWindow = true;
	static bool _imGuiInitializing = false;

	HRESULT __stdcall detourD3D11ResizeBuffers(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
	{
		_imGuiInitializing = true;
		ImGui_ImplDX11_InvalidateDeviceObjects();
		cleanupRenderTarget();
		HRESULT toReturn = hookedD3D11ResizeBuffers(pSwapChain, bufferCount, width, height, newFormat, swapChainFlags);
		createRenderTarget(pSwapChain);
		ImGui_ImplDX11_CreateDeviceObjects();
		_imGuiInitializing = false;
		return toReturn;
	}


	HRESULT __stdcall detourD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
	{
		static bool _initializeDeviceAndContext = true;

		if (_tmpSwapChainInitialized)
		{
			if (!(Flags & DXGI_PRESENT_TEST) && !_imGuiInitializing)
			{
				if (_initializeDeviceAndContext)
				{
					if (FAILED(pSwapChain->GetDevice(__uuidof(_device), (void**)&_device)))
					{
						IGCS::Console::WriteError("Failed to get device from hooked swapchain");
					}
					else
					{
						OverlayConsole::instance().logDebug("DX11 Device: %p", (void*)_device);
						_device->GetImmediateContext(&_context);
					}
					if (nullptr == _context)
					{
						IGCS::Console::WriteError("Failed to get device context from hooked swapchain");
					}
					else
					{
						OverlayConsole::instance().logDebug("DX11 Context: %p", (void*)_context);
					}
					createRenderTarget(pSwapChain);
					initImGui();

					_initializeDeviceAndContext = false;
				}

				// render our own stuff
				_context->OMSetRenderTargets(1, &_mainRenderTargetView, NULL);
				OverlayControl::renderOverlay();
				ImGui_ImpDX11_ResetKeyStates();
			}
		}
		return hookedD3D11Present(pSwapChain, SyncInterval, Flags);
	}

	void initializeHook()
	{
		HWND hWnd = IGCS::Globals::instance().mainWindowHandle();
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		ID3D11Device *pTmpDevice = NULL;
		ID3D11DeviceContext *pTmpContext = NULL;
		IDXGISwapChain* pTmpSwapChain;
		HRESULT deviceCreationResult = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &pTmpSwapChain,
																	 &pTmpDevice, NULL, &pTmpContext);
		if (FAILED(deviceCreationResult))
		{
			IGCS::Console::WriteError("Failed to create directX device and swapchain!");
			return;
		}

		__int64* pSwapChainVtable = NULL;
		__int64* pDeviceContextVTable = NULL;
		pSwapChainVtable = (__int64*)pTmpSwapChain;
		pSwapChainVtable = (__int64*)pSwapChainVtable[0];
		pDeviceContextVTable = (__int64*)pTmpContext;
		pDeviceContextVTable = (__int64*)pDeviceContextVTable[0];

		OverlayConsole::instance().logDebug("Present Address: %p", (void*)(__int64*)pSwapChainVtable[DXGI_PRESENT_INDEX]);

		if (MH_CreateHook((LPBYTE)pSwapChainVtable[DXGI_PRESENT_INDEX], &detourD3D11Present, reinterpret_cast<LPVOID*>(&hookedD3D11Present)) != MH_OK)
		{
			IGCS::Console::WriteError("Hooking Present failed!");
		}
		if (MH_EnableHook((LPBYTE)pSwapChainVtable[DXGI_PRESENT_INDEX]) != MH_OK)
		{
			IGCS::Console::WriteError("Enabling of Present hook failed!");
		}

		OverlayConsole::instance().logDebug("ResizeBuffers Address: %p", (__int64*)pSwapChainVtable[DXGI_RESIZEBUFFERS_INDEX]);

		if (MH_CreateHook((LPBYTE)pSwapChainVtable[DXGI_RESIZEBUFFERS_INDEX], &detourD3D11ResizeBuffers, reinterpret_cast<LPVOID*>(&hookedD3D11ResizeBuffers)) != MH_OK)
		{
			IGCS::Console::WriteError("Hooking ResizeBuffers failed!");
		}
		if (MH_EnableHook((LPBYTE)pSwapChainVtable[DXGI_RESIZEBUFFERS_INDEX]) != MH_OK)
		{
			IGCS::Console::WriteError("Enabling of ResizeBuffers hook failed!");
		}

		pTmpDevice->Release();
		pTmpContext->Release();
		pTmpSwapChain->Release();
		_tmpSwapChainInitialized = true;

		OverlayConsole::instance().logDebug("DX11 hooks set");
	}


	void createRenderTarget(IDXGISwapChain* pSwapChain)
	{
		DXGI_SWAP_CHAIN_DESC sd;
		pSwapChain->GetDesc(&sd);
		ID3D11Texture2D* pBackBuffer;
		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
		render_target_view_desc.Format = sd.BufferDesc.Format;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		_device->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &_mainRenderTargetView);
		pBackBuffer->Release();
	}


	void cleanupRenderTarget()
	{
		if (nullptr != _mainRenderTargetView)
		{
			_mainRenderTargetView->Release();
			_mainRenderTargetView = nullptr;
		}
	}


	void initImGui()
	{
		ImGui_ImplDX11_Init(IGCS::Globals::instance().mainWindowHandle(), _device, _context);
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = IGCS_OVERLAY_INI_FILENAME;
		initImGuiStyle();
	}


	void initImGuiStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowRounding = 2.0f;
		style.FrameRounding = 1.0f;
		style.IndentSpacing = 25.0f;
		style.ScrollbarSize = 16.0f;
		style.ScrollbarRounding = 1.0f;

		style.Colors[ImGuiCol_Text] = ImVec4(0.84f, 0.84f, 0.88f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.24f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.90f);
		style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.24f, 0.31f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.33f, 0.37f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.37f, 0.37f, 0.42f, 0.42f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.45f, 0.45f, 0.45f, 0.30f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ComboBg] = ImVec4(0.13f, 0.13f, 0.16f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.53f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.65f, 0.31f, 0.00f, 0.71f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.65f, 0.31f, 0.00f, 0.86f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.41f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.50f, 0.50f, 0.53f, 0.49f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.44f, 0.31f);
		style.Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
		style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.44f, 0.44f, 0.30f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.40f, 0.40f, 0.44f);
		style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.93f);
		style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
	}
}