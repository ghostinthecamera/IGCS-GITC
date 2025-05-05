#include "stdafx.h"
#include "D3DHook.h"
#include "Globals.h"
#include "MessageHandler.h"
#include "CameraManipulator.h"
#include "MinHook.h"
#include "PathManager.h"
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <TlHelp32.h>
#include "Bezier.h"
#include "CentripetalCatmullRom.h"
#include "CatmullRom.h"
#include "BSpline.h"
#include "Cubic.h"
#include "PathUtils.h"
#include "RiemannCubic.h"
#include "Console.h"
#include "Utils.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace IGCS {

    //==================================================================================================
    // Static Member Initialization
    //==================================================================================================
    D3DHook::Present_t D3DHook::_originalPresent = nullptr;
    D3DHook::ResizeBuffers_t D3DHook::_originalResizeBuffers = nullptr;
    D3DHook::OMSetRenderTargets_t D3DHook::_originalOMSetRenderTargets = nullptr;

    ID3D11Device* D3DHook::_pLastDevice = nullptr;
    ID3D11DeviceContext* D3DHook::_pLastContext = nullptr;
    ID3D11RenderTargetView* D3DHook::_pLastRTV = nullptr;

    ID3D11Buffer* D3DHook::_sphereVertexBuffer = nullptr;
    ID3D11Buffer* D3DHook::_sphereIndexBuffer = nullptr;
    ID3D11VertexShader* D3DHook::_simpleVertexShader = nullptr;
    ID3D11PixelShader* D3DHook::_simplePixelShader = nullptr;
    ID3D11InputLayout* D3DHook::_simpleInputLayout = nullptr;
    ID3D11Buffer* D3DHook::_constantBuffer = nullptr;
    ID3D11BlendState* D3DHook::_blendState = nullptr;
    ID3D11PixelShader* D3DHook::_coloredPixelShader = nullptr;
    ID3D11Buffer* D3DHook::_colorBuffer = nullptr;

    UINT D3DHook::_sphereIndexCount = 0;
    DirectX::XMFLOAT3 D3DHook::_fixedSpherePosition = { 0.0f, 0.0f, 0.0f };
    bool D3DHook::_spherePositionInitialized = false;

    // Depth buffer management
    std::vector<ID3D11DepthStencilView*> D3DHook::_detectedDepthStencilViews;
    std::vector<ID3D11Texture2D*> D3DHook::_detectedDepthTextures;
    std::vector<D3D11_TEXTURE2D_DESC> D3DHook::_depthTextureDescs;
    int D3DHook::_currentDepthBufferIndex = 0;
    int D3DHook::_depthScanCount = 0;
    bool D3DHook::_hookingOMSetRenderTargets = false;
    bool D3DHook::_useDetectedDepthBuffer = false;
    ID3D11DepthStencilView* D3DHook::_currentDepthStencilView = nullptr;

    // Path visualization
    bool D3DHook::_pathVisualizationEnabled = false;
    bool D3DHook::_visualizationEnabled = false;
    bool D3DHook::_renderSelectedPathOnly = false;

    //==================================================================================================
    // Singleton Implementation
    //==================================================================================================
    D3DHook& D3DHook::instance() {
        static D3DHook instance;
        return instance;
    }

    //==================================================================================================
    // Constructor & Destructor
    //==================================================================================================
    D3DHook::D3DHook()
        : _isInitialized(false)
        , _renderTargetInitialized(false)
        , _pathResourcesCreated(false)
        , _isVisible(true)
        , _pSwapChain(nullptr)
        , _pDevice(nullptr)
        , _pContext(nullptr)
        , _pRenderTargetView(nullptr)
        , _pBlendState(nullptr)
        , _pDepthStencilState(nullptr)
        , _pRasterizerState(nullptr)
        , _windowWidth(0)
        , _windowHeight(0)
        , _pathVertexBuffer(nullptr)
        , _nodeVertexBuffer(nullptr)
        , _interpolatedPathVertexBuffer(nullptr)
        , _directionIndicatorVertexBuffer(nullptr)
        , _pathTubeVertexBuffer(nullptr)
        , _pathTubeIndexBuffer(nullptr)
        , _interpolatedDirectionsVertexBuffer(nullptr)
        , _pathVertexCount(0)
        , _nodeVertexCount(0)
        , _interpolatedPathVertexCount(0)
        , _directionIndicatorVertexCount(0)
        , _pathTubeVertexCount(0)
        , _pathTubeIndexCount(0)
        , _interpolatedDirectionsVertexCount(0)
        , _arrowHeadVertexBuffer(nullptr)
        , _arrowHeadIndexBuffer(nullptr)
        , _arrowShaftVertexBuffer(nullptr)
        , _arrowShaftIndexBuffer(nullptr)
        , _arrowHeadIndexCount(0)
        , _arrowShaftIndexCount(0)
        , _isChangingMode(false)
        , _resourcesNeedUpdate(false)
        , _interpolationSteps(30)
        , _nodeSize(0.05f)
        , _directionLength(0.3f)
        , _tubeDiameter(0.05f)
        , _tubeSegments(8)
        , _baseDirectionLength(0.3f)
        , _defaultFOV(DEFAULT_FOV)
        , _rotationSampleFrequency(3)
        , _arrowHeadLength(0.05f)
        , _arrowHeadRadius(0.025f)
        , _arrowShaftRadius(0.008f)
        , _arrowSegments(12)
        , _needsInitialization(false)
    {
    }

    D3DHook::~D3DHook() {
        cleanUp();
    }

    //==================================================================================================
    // Public Interface Methods
    //==================================================================================================
    bool D3DHook::initialize() {
        if (_isInitialized) {
            return true;
        }

        MessageHandler::logDebug("D3DHook::initialize: Starting D3D11 hook initialization");

        // Hook the "Present" function - all we care about is correctly hooking this
        // First, create a temporary D3D11 device/swapchain just to get the Present address
        HWND hWnd = Globals::instance().mainWindowHandle();
        if (!hWnd) {
            MessageHandler::logError("D3DHook::initialize: Failed to get game window");
            return false;
        }

        // Create a dummy device and swap chain to get the Present address
        IDXGISwapChain* pTempSwapChain = nullptr;
        ID3D11Device* pTempDevice = nullptr;
        ID3D11DeviceContext* pTempContext = nullptr;

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &swapChainDesc, &pTempSwapChain, &pTempDevice, nullptr, &pTempContext
        );

        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initialize: Failed to create temporary device/swapchain");
            return false;
        }

        // Get the Present function address from the temp swap chain's vftable
        void** vftable = *reinterpret_cast<void***>(pTempSwapChain);
        void* presentAddress = vftable[8]; // Present is at index 8

        MessageHandler::logDebug("D3DHook::initialize: Hooking Present function at %p", presentAddress);

        // Create the hook
        MH_STATUS status = MH_CreateHook(
            presentAddress,
            &D3DHook::hookedPresent,
            reinterpret_cast<void**>(&_originalPresent)
        );

        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::initialize: Failed to create Present hook, error: %d", status);
            pTempContext->Release();
            pTempDevice->Release();
            pTempSwapChain->Release();
            return false;
        }

        // Enable the hook
        status = MH_EnableHook(presentAddress);
        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::initialize: Failed to enable Present hook, error: %d", status);
            pTempContext->Release();
            pTempDevice->Release();
            pTempSwapChain->Release();
            return false;
        }

        MessageHandler::logLine("Successfully hooked Direct3D 11 Present function");

        // Clean up temporary objects - we don't need them anymore
        pTempContext->Release();
        pTempDevice->Release();
        pTempSwapChain->Release();

        _isInitialized = true;
        return true;
    }

    void D3DHook::cleanUp() {
        if (!_isInitialized) {
            return;
        }

        // Acquire the resource mutex to ensure no rendering is in progress
        std::lock_guard<std::mutex> lock(_resourceMutex);

        // Reset initialization flags
        _needsInitialization = false;
        _isInitialized = false;
        _renderTargetInitialized = false;

        releaseD3DResources();
        cleanupDepthBufferResources();

        _isInitialized = false;
        _renderTargetInitialized = false;
        _pathResourcesCreated = false;
        _visualizationEnabled = false;
        _renderSelectedPathOnly = false;
        _pathInfos.clear();
        _nodePositions.clear();
    }

    void D3DHook::toggleVisibility() {
        _isVisible = !_isVisible;
        MessageHandler::logDebug("D3DHook::toggleVisibility: Visibility is now %s", _isVisible ? "ON" : "OFF");
    }

    bool D3DHook::isVisible() const {
        return _isVisible;
    }

    void D3DHook::setVisualization(bool enabled) {
        _visualizationEnabled = enabled;
        MessageHandler::logDebug("D3DHook::setVisualization: Visualization is now %s", _visualizationEnabled ? "ON" : "OFF");
    }

    bool D3DHook::isVisualizationEnabled() const {
        return _visualizationEnabled;
    }

    bool D3DHook::isPathVisualizationEnabled() const {
        return _pathVisualizationEnabled;
    }

    void D3DHook::togglePathVisualization() {
        _pathVisualizationEnabled = !_pathVisualizationEnabled;
        //MessageHandler::logDebug("D3DHook::togglePathVisualization: Path visualization is now %s",
        //    _pathVisualizationEnabled ? "ON" : "OFF");
    }

    void D3DHook::setRenderPathOnly(bool renderPathOnly) {
        _renderSelectedPathOnly = renderPathOnly;
        MessageHandler::logDebug("D3DHook::setRenderPathOnly: Render selected path only is now %s",
            _renderSelectedPathOnly ? "ON" : "OFF");
    }

    bool D3DHook::isRenderPathOnly() const {
        return _renderSelectedPathOnly;
    }

    void D3DHook::cycleDepthBuffer() {
        if (_detectedDepthStencilViews.empty()) {
            MessageHandler::logDebug("D3DHook::cycleDepthBuffer: No depth buffers detected");
            return;
        }

        // Enable depth buffer usage if not already enabled
        if (!_useDetectedDepthBuffer) {
            _useDetectedDepthBuffer = true;
            MessageHandler::logDebug("D3DHook: Depth buffer usage enabled");
        }

        // Cycle to the next depth buffer
        _currentDepthBufferIndex = (_currentDepthBufferIndex + 1) % _detectedDepthStencilViews.size();

        // Ensure we have at least basic info for all buffers
        if (_depthTextureDescs.size() < _detectedDepthStencilViews.size()) {
            MessageHandler::logDebug("D3DHook::cycleDepthBuffer: Description count mismatch, adding placeholders");
            // Add placeholder descriptions if needed
            while (_depthTextureDescs.size() < _detectedDepthStencilViews.size()) {
                D3D11_TEXTURE2D_DESC placeholderDesc = {};
                placeholderDesc.Width = 0;
                placeholderDesc.Height = 0;
                placeholderDesc.Format = DXGI_FORMAT_UNKNOWN;
                _depthTextureDescs.push_back(placeholderDesc);
            }
        }

        // Log info about the current depth buffer
        if (_currentDepthBufferIndex < _depthTextureDescs.size()) {
            const auto& desc = _depthTextureDescs[_currentDepthBufferIndex];
            MessageHandler::logDebug("D3DHook::cycleDepthBuffer: Switched to depth buffer %d of %zu, dimensions: %dx%d, format: %d",
                _currentDepthBufferIndex + 1, _detectedDepthStencilViews.size(),
                desc.Width, desc.Height, desc.Format);
        }
        else {
            // This should never happen with the fix above, but just in case
            MessageHandler::logDebug("D3DHook::cycleDepthBuffer: Switched to depth buffer %d of %zu (no description available)",
                _currentDepthBufferIndex + 1, _detectedDepthStencilViews.size());
        }
    }

    bool D3DHook::isUsingDepthBuffer() const {
        return _useDetectedDepthBuffer;
    }

    void D3DHook::toggleDepthBufferUsage() {
        _useDetectedDepthBuffer = !_useDetectedDepthBuffer;
        MessageHandler::logDebug("D3DHook::toggleDepthBufferUsage: Depth buffer usage is now %s",
            _useDetectedDepthBuffer ? "ON" : "OFF");
    }

    void D3DHook::safeInterpolationModeChange() {
        std::lock_guard lock(_resourceMutex);

        // Set the flag to indicate we're changing modes
        _isChangingMode.store(true, std::memory_order_release);

        // Release existing visualization resources
        if (_pathVertexBuffer) { _pathVertexBuffer->Release(); _pathVertexBuffer = nullptr; }
        if (_nodeVertexBuffer) { _nodeVertexBuffer->Release(); _nodeVertexBuffer = nullptr; }
        if (_interpolatedPathVertexBuffer) { _interpolatedPathVertexBuffer->Release(); _interpolatedPathVertexBuffer = nullptr; }
        if (_directionIndicatorVertexBuffer) { _directionIndicatorVertexBuffer->Release(); _directionIndicatorVertexBuffer = nullptr; }
        if (_pathTubeVertexBuffer) { _pathTubeVertexBuffer->Release(); _pathTubeVertexBuffer = nullptr; }
        if (_pathTubeIndexBuffer) { _pathTubeIndexBuffer->Release(); _pathTubeIndexBuffer = nullptr; }
        if (_interpolatedDirectionsVertexBuffer) { _interpolatedDirectionsVertexBuffer->Release(); _interpolatedDirectionsVertexBuffer = nullptr; }

        _pathResourcesCreated = false;
        _pathInfos.clear();
        _nodePositions.clear();

        // Update the path visualization with new resources
        createPathVisualization();

        // Reset the flag
        _isChangingMode.store(false, std::memory_order_release);

        MessageHandler::logDebug("D3DHook::safeInterpolationModeChange: Interpolation mode changed successfully");
    }

    ID3D11Device* D3DHook::getDevice() const {
        return _pDevice;
    }

    ID3D11DeviceContext* D3DHook::getContext() const {
        return _pContext;
    }

    //==================================================================================================
    // Resource Management Methods
    //==================================================================================================
    void D3DHook::releaseD3DResources() {
        // Safe release helper macro
		#define SAFE_RELEASE(obj) if(obj) { (obj)->Release(); (obj) = nullptr; }

		// Core D3D resources
        SAFE_RELEASE(_pRenderTargetView);
        SAFE_RELEASE(_pBlendState);
        SAFE_RELEASE(_pDepthStencilState);
        SAFE_RELEASE(_pRasterizerState);
        SAFE_RELEASE(_pContext);
        SAFE_RELEASE(_pDevice);
        SAFE_RELEASE(_pSwapChain);

        // Path visualization resources
        SAFE_RELEASE(_pathVertexBuffer);
        SAFE_RELEASE(_nodeVertexBuffer);
        SAFE_RELEASE(_interpolatedPathVertexBuffer);
        SAFE_RELEASE(_directionIndicatorVertexBuffer);
        SAFE_RELEASE(_pathTubeVertexBuffer);
        SAFE_RELEASE(_pathTubeIndexBuffer);
        SAFE_RELEASE(_interpolatedDirectionsVertexBuffer);

        // Pixel shader resources
        SAFE_RELEASE(_coloredPixelShader);
        SAFE_RELEASE(_colorBuffer);

		#undef SAFE_RELEASE
    }

    //==================================================================================================
	// Initialization Methods
	//==================================================================================================
    bool D3DHook::createRenderTargetView() {
        if (!_pSwapChain || !_pDevice) {
            MessageHandler::logError("D3DHook::createRenderTargetView: SwapChain or Device is null");
            return false;
        }

        // Get the back buffer texture from the swap chain
        ID3D11Texture2D* pBackBuffer = nullptr;
        HRESULT hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createRenderTargetView: Failed to get back buffer, error: 0x%X", hr);
            return false;
        }

        // Create a render target view for the back buffer
        hr = _pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
        pBackBuffer->Release();

        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createRenderTargetView: Failed to create render target view, error: 0x%X", hr);
            return false;
        }

        // Create blend state for alpha blending
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = _pDevice->CreateBlendState(&blendDesc, &_pBlendState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createRenderTargetView: Failed to create blend state, error: 0x%X", hr);
            return false;
        }

        // Create depth stencil state
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

        hr = _pDevice->CreateDepthStencilState(&depthStencilDesc, &_pDepthStencilState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createRenderTargetView: Failed to create depth stencil state, error: 0x%X", hr);
            return false;
        }

        // Create rasterizer state
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;

        hr = _pDevice->CreateRasterizerState(&rasterizerDesc, &_pRasterizerState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createRenderTargetView: Failed to create rasterizer state, error: 0x%X", hr);
            return false;
        }

        _renderTargetInitialized = true;
        return true;
    }

    bool D3DHook::hookDXGIFunctions() {
        if (!_pSwapChain) {
            MessageHandler::logError("D3DHook::hookDXGIFunctions: SwapChain is null");
            return false;
        }

        // Get the vtable address for IDXGISwapChain
        void** vtable = *reinterpret_cast<void***>(_pSwapChain);

        // Present is at index 8 in the vtable
        void* presentAddress = vtable[8];

        // ResizeBuffers is at index 13 in the vtable
        void* resizeBuffersAddress = vtable[13];

        // Create hooks
        MH_STATUS status = MH_CreateHook(
            presentAddress,
            &D3DHook::hookedPresent,
            reinterpret_cast<void**>(&_originalPresent)
        );

        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::hookDXGIFunctions: Failed to create Present hook, error: %d", status);
            return false;
        }

        status = MH_CreateHook(
            resizeBuffersAddress,
            &D3DHook::hookedResizeBuffers,
            reinterpret_cast<void**>(&_originalResizeBuffers)
        );

        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::hookDXGIFunctions: Failed to create ResizeBuffers hook, error: %d", status);
            return false;
        }

        // Enable the hooks
        status = MH_EnableHook(MH_ALL_HOOKS);
        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::hookDXGIFunctions: Failed to enable hooks, error: %d", status);
            return false;
        }

        MessageHandler::logDebug("D3DHook::hookDXGIFunctions: Successfully hooked DXGI functions");
        return true;
    }

    //bool D3DHook::findActiveSwapChain() {

    //	MessageHandler::logDebug("D3DHook::findActiveSwapChain: Attempting to find active swap chain");

    //    // First try Process Enumeration technique to find D3D11 module
    //    DWORD processId = GetCurrentProcessId();
    //    MODULEENTRY32 moduleEntry{};
    //    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    //    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
    //    if (snapshot == INVALID_HANDLE_VALUE) {
    //        MessageHandler::logError("D3DHook::findActiveSwapChain: Failed to create module snapshot");
    //        return false;
    //    }

    //    bool foundD3D11 = false;
    //    bool foundDXGI = false;
    //    HMODULE d3d11Module = nullptr;
    //    HMODULE dxgiModule = nullptr;

    //    if (Module32First(snapshot, &moduleEntry)) {
    //        do {
    //            if (_wcsicmp(moduleEntry.szModule, L"d3d11.dll") == 0) {
    //                foundD3D11 = true;
    //                d3d11Module = moduleEntry.hModule;
    //                MessageHandler::logDebug("D3DHook::findActiveSwapChain: Found D3D11 module at %p", d3d11Module);
    //            }
    //            else if (_wcsicmp(moduleEntry.szModule, L"dxgi.dll") == 0) {
    //                foundDXGI = true;
    //                dxgiModule = moduleEntry.hModule;
    //                MessageHandler::logDebug("D3DHook::findActiveSwapChain: Found DXGI module at %p", dxgiModule);
    //            }

    //            if (foundD3D11 && foundDXGI)
    //                break;

    //        } while (Module32Next(snapshot, &moduleEntry));
    //    }

    //    CloseHandle(snapshot);

    //    if (!foundD3D11 || !foundDXGI) {
    //        MessageHandler::logError("D3DHook::findActiveSwapChain: Failed to find D3D11 or DXGI modules");
    //        return false;
    //    }

    //    // Get the game window
    //    HWND hWnd = Globals::instance().mainWindowHandle();
    //    if (!hWnd) {
    //        MessageHandler::logError("D3DHook::findActiveSwapChain: Failed to get game window");
    //        return false;
    //    }

    //    // Get window dimensions
    //    RECT rect;
    //    GetClientRect(hWnd, &rect);
    //    UINT width = rect.right - rect.left;
    //    UINT height = rect.bottom - rect.top;

    //    // Create a temporary D3D11 device and swap chain
    //    IDXGISwapChain* pTempSwapChain = nullptr;
    //    ID3D11Device* pTempDevice = nullptr;
    //    ID3D11DeviceContext* pTempContext = nullptr;

    //    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    //    swapChainDesc.BufferCount = 1;
    //    swapChainDesc.BufferDesc.Width = width;
    //    swapChainDesc.BufferDesc.Height = height;
    //    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //    swapChainDesc.OutputWindow = hWnd;
    //    swapChainDesc.SampleDesc.Count = 1;
    //    swapChainDesc.Windowed = TRUE;

    //    HRESULT hr = D3D11CreateDeviceAndSwapChain(
    //        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
    //        &swapChainDesc, &pTempSwapChain, &pTempDevice, nullptr, &pTempContext
    //    );

    //    if (FAILED(hr)) {
    //        MessageHandler::logError("D3DHook::findActiveSwapChain: Failed to create temporary device and swap chain");
    //        return false;
    //    }

    //    // Store the temporary swap chain and device (we created these so we'll use them)
    //    _pSwapChain = pTempSwapChain;
    //    _pDevice = pTempDevice;
    //    _pContext = pTempContext;

    //    // Store window dimensions
    //    _windowWidth = width;
    //    _windowHeight = height;

    //    // Now hook the swap chain functions - CRITICAL STEP
    //    bool hookResult = hookDXGIFunctions();

    //    if (!hookResult) {
    //        // Clean up if hook fails
    //        MessageHandler::logError("D3DHook::findActiveSwapChain: Failed to hook DXGI functions");

    //        // Release resources
    //        if (_pContext) { _pContext->Release(); _pContext = nullptr; }
    //        if (_pDevice) { _pDevice->Release(); _pDevice = nullptr; }
    //        if (_pSwapChain) { _pSwapChain->Release(); _pSwapChain = nullptr; }

    //        return false;
    //    }

    //    MessageHandler::logDebug("D3DHook::findActiveSwapChain: Successfully found and hooked swap chain");
    //    return true;
    //}

    bool D3DHook::initializeRenderingResources() {

    	if (!_pLastDevice || !_pLastContext) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Device or Context is null");
            return false;
        }

        // 1. Create shaders
        // Simple vertex shader
        const char* vsCode = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix worldViewProjection;
        };

        struct VS_INPUT
        {
            float3 position : POSITION;
            float4 color : COLOR;
        };

        struct PS_INPUT
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        PS_INPUT main(VS_INPUT input)
        {
            PS_INPUT output;
            output.position = mul(float4(input.position, 1.0f), worldViewProjection);
            output.color = input.color;
            return output;
        }
    )";

        // Simple pixel shader
        const char* psCode = R"(
        struct PS_INPUT
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(PS_INPUT input) : SV_Target
        {
            return input.color;
        }
    )";

        const char* coloredPsCode = R"(
        cbuffer ColorBuffer : register(b0)
        {
            float4 color;
        };

        struct PS_INPUT
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(PS_INPUT input) : SV_Target
        {
            return color; // Use the uniform color instead of vertex color
        }
    )";

        // Compile shaders
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        ID3DBlob* coloredPsBlob = nullptr;

        // Helper to release blobs on scope exit
        class BlobGuard {
        public:
            BlobGuard(ID3DBlob** blobs, int count) : _blobs(blobs), _count(count) {}
            ~BlobGuard() {
                for (int i = 0; i < _count; i++) {
                    if (_blobs[i]) {
                        _blobs[i]->Release();
                        _blobs[i] = nullptr;
                    }
                }
            }
        private:
            ID3DBlob** _blobs;
            int _count;
        };

        // Array of blobs for cleanup
        ID3DBlob* blobs[] = { vsBlob, psBlob, errorBlob, coloredPsBlob };
        BlobGuard blobGuard(blobs, 4);

        // Compile vertex shader
        HRESULT hr = D3DCompile(vsCode, strlen(vsCode), "VS", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to compile vertex shader: %s",
                    (char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        // Compile pixel shader
        hr = D3DCompile(psCode, strlen(psCode), "PS", nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to compile pixel shader: %s",
                    (char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        // Compile colored pixel shader
        hr = D3DCompile(coloredPsCode, strlen(coloredPsCode), "ColoredPS", nullptr, nullptr, "main", "ps_4_0", 0, 0, &coloredPsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                MessageHandler::logError("Failed to compile colored pixel shader: %s",
                    (char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        // Create shaders
        hr = _pLastDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_simpleVertexShader);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create vertex shader");
            return false;
        }

        hr = _pLastDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_simplePixelShader);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create pixel shader");
            return false;
        }

        // Create the colored pixel shader
        hr = _pLastDevice->CreatePixelShader(coloredPsBlob->GetBufferPointer(),
            coloredPsBlob->GetBufferSize(), nullptr, &_coloredPixelShader);
        if (FAILED(hr)) {
            MessageHandler::logError("Failed to create colored pixel shader");
            return false;
        }

        // Create input layout
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = _pLastDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_simpleInputLayout);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create input layout");
            return false;
        }

        // Create constant buffer
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = sizeof(SimpleConstantBuffer);
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = _pLastDevice->CreateBuffer(&cbDesc, nullptr, &_constantBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create constant buffer");
            return false;
        }

        // Create blend state for transparency
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = _pLastDevice->CreateBlendState(&blendDesc, &_blendState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create blend state");
            return false;
        }

        // Create the color buffer
        D3D11_BUFFER_DESC colorBufferDesc = {};
        colorBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        colorBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
        colorBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        colorBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = _pLastDevice->CreateBuffer(&colorBufferDesc, nullptr, &_colorBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("Failed to create color buffer");
            return false;
        }

        // Create a simple sphere
        D3DHook::instance().createSolidColorSphere(1.0f, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));

        // Create path visualization resources
        D3DHook::instance().createPathVisualization();

        // Create arrow geometry for direction indicators
        D3DHook::instance().createArrowGeometry();

        return true;
    }

    //==================================================================================================
    // Hook Functions
    //==================================================================================================
    HRESULT STDMETHODCALLTYPE D3DHook::hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {

        static uint32_t frameCounter = 0;
        frameCounter = (frameCounter + 1) % 60;  // Only directly store the 0-59 value

    	// Acquire device only once, without initializing resources
        if (!_pLastDevice && pSwapChain) {
            // Signal that we need initialization rather than doing it here
            bool needsInit = false;

            {
                // Use lock to prevent race conditions during device acquisition
                std::lock_guard<std::mutex> lock(instance()._resourceMutex);

                if (!_pLastDevice) { // Double-check after acquiring lock
                    ID3D11Device* pDevice = nullptr;
                    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice)))) {
                        _pLastDevice = pDevice;
                        pDevice->GetImmediateContext(&_pLastContext);

                        // Get back buffer for RTVs
                        ID3D11Texture2D* pBackBuffer = nullptr;
                        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                            _pLastDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pLastRTV);
                            pBackBuffer->Release();

                            needsInit = true;
                        }

                        MessageHandler::logDebug("D3DHook::hookedPresent: Successfully captured device and context");
                    }
                }
            }

            // Signal that initialization is needed, but don't do it here
            if (needsInit) {
                instance().queueInitialization();
            }
        }

        if (Globals::instance().systemActive())
        {
            System::instance().timestop();
        }

        if (frameCounter == 0) {
            D3DHook::instance().scanForDepthBuffers();
        }

        // Process depth buffer detection
        // This needs to happen during rendering to detect active depth buffers
        if (!_detectedDepthStencilViews.empty()) {
            D3DHook::instance().selectAppropriateDepthBuffer();
        }

        // Render paths if visualization is enabled
        if (instance().isVisualizationEnabled()) {
            instance().renderPaths();
        }

         //System update if active
        if (Globals::instance().systemActive() && RUN_IN_HOOKED_PRESENT) {
            IGCS::System::instance().updateFrame();
        }

        //if (Globals::instance().systemActive())
        //{
        //    System::instance().timestop();
        //}

        // Call the original Present function
        return _originalPresent(pSwapChain, SyncInterval, Flags);
    }

    void D3DHook::queueInitialization() {
        // Set a flag indicating initialization is needed
        _needsInitialization = true;

        // Log the queuing
        MessageHandler::logDebug("D3DHook::queueInitialization: D3D initialization has been queued");
    }

    // New method to check if initialization is complete
    bool D3DHook::isFullyInitialized() const {
        return _isInitialized && _renderTargetInitialized && !_needsInitialization;
    }

    // New method to perform the actual initialization
    // This should be called from a controlled point in your main loop
    void D3DHook::performQueuedInitialization() {
        // Only proceed if initialization is needed
        if (!_needsInitialization) {
            return;
        }

        // Use lock to prevent race conditions
        std::lock_guard<std::mutex> lock(_resourceMutex);

        // Double-check after acquiring lock
        if (!_needsInitialization) {
            return;
        }

        MessageHandler::logDebug("D3DHook::performQueuedInitialization: Starting initialization");

        // Setup hook for depth buffer detection if needed
        if (!_hookingOMSetRenderTargets) {
            setupOMSetRenderTargetsHook();
        }

        // Start scanning for depth buffers
        scanForDepthBuffers();

        // Initialize rendering resources
        if (initializeRenderingResources()) {
            MessageHandler::logDebug("D3DHook::performQueuedInitialization: Rendering resources initialized");
        }
        else {
            MessageHandler::logError("D3DHook::performQueuedInitialization: Failed to initialize rendering resources");
        }

        // Create path visualization resources
        //createPathVisualization();

        // Mark initialization as complete
        _needsInitialization = false;
        _isInitialized = true;
        _renderTargetInitialized = true;

        MessageHandler::logDebug("D3DHook::performQueuedInitialization: Initialization complete");
    }

    HRESULT STDMETHODCALLTYPE D3DHook::hookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
        UINT Width, UINT Height, DXGI_FORMAT NewFormat,
        UINT SwapChainFlags) {
        D3DHook& hook = D3DHook::instance();

        // Release render target before resizing
        if (hook._pRenderTargetView) {
            hook._pRenderTargetView->Release();
            hook._pRenderTargetView = nullptr;
        }

        hook._renderTargetInitialized = false;
        hook._windowWidth = Width;
        hook._windowHeight = Height;

        // Call original function
        HRESULT result = hook._originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

        // Recreate render target view after resizing
        if (SUCCEEDED(result)) {
            hook.createRenderTargetView();
        }

        return result;
    }

    void STDMETHODCALLTYPE D3DHook::hookedOMSetRenderTargets(ID3D11DeviceContext* pContext, UINT NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView* pDepthStencilView) {
        // Call original function first
        _originalOMSetRenderTargets(pContext, NumViews, ppRenderTargetViews, pDepthStencilView);

        // Capture the depth buffer if it exists
        if (pDepthStencilView) {
            D3DHook::instance().captureActiveDepthBuffer(pDepthStencilView);
        }
    }

    //==================================================================================================
	// Depth Buffer Management
	//==================================================================================================
    void D3DHook::setupOMSetRenderTargetsHook() {
        if (!_pLastContext) {
            MessageHandler::logError("D3DHook::setupOMSetRenderTargetsHook: Context is null");
            return;
        }

        // Get the virtual table pointer
        void** vtbl = *reinterpret_cast<void***>(_pLastContext);

        // OMSetRenderTargets is at index 33 in the ID3D11DeviceContext vtable
        void* originalFunc = vtbl[33];

        // Create the hook
        MH_STATUS status = MH_CreateHook(
            originalFunc,
            &D3DHook::hookedOMSetRenderTargets,
            reinterpret_cast<void**>(&_originalOMSetRenderTargets)
        );

        if (status == MH_OK) {
            status = MH_EnableHook(originalFunc);
            if (status == MH_OK) {
                _hookingOMSetRenderTargets = true;
                //MessageHandler::logDebug("D3DHook::setupOMSetRenderTargetsHook: Successfully hooked OMSetRenderTargets");
            }
            else {
                MessageHandler::logError("D3DHook::setupOMSetRenderTargetsHook: Failed to enable hook, error: %d", status);
            }
        }
        else {
            MessageHandler::logError("D3DHook::setupOMSetRenderTargetsHook: Failed to create hook, error: %d", status);
        }
    }

    void D3DHook::scanForDepthBuffers() {
        if (!_pLastDevice || !_detectedDepthStencilViews.empty()) {
            return;
        }

        _depthScanCount++;

        // Only scan every 60 frames to avoid performance impact
        if (_depthScanCount % 60 != 0) {
            return;
        }

        // If we have no depth buffers yet, try to hook OMSetRenderTargets to capture active ones
        static bool hookAttempted = false;
        if (_detectedDepthStencilViews.empty() && _pLastContext && !hookAttempted) {
            MessageHandler::logDebug("D3DHook::scanForDepthBuffers: No depth buffers yet so scanning in setupOMSSetRenderTargetsHook");
            setupOMSetRenderTargetsHook();
            hookAttempted = true;
        }

        // If we already have some depth buffers, don't perform additional scans
        if (!_detectedDepthStencilViews.empty() && hookAttempted) {
            return;
        }

        MessageHandler::logDebug("D3DHook::scanForDepthBuffers: Scanning for depth buffers...");

        // Try to create a DSV from active RT to find compatible depth buffer
        if (_pLastRTV) {
            // Get the texture from the RTV
            ID3D11Resource* pRTResource = nullptr;
            _pLastRTV->GetResource(&pRTResource);

            if (pRTResource) {
                ID3D11Texture2D* pRTTexture = nullptr;
                HRESULT hr = pRTResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pRTTexture);

                if (SUCCEEDED(hr) && pRTTexture) {
                    // Get the dimensions
                    D3D11_TEXTURE2D_DESC rtDesc;
                    pRTTexture->GetDesc(&rtDesc);

                    // Try to create a depth texture with matching dimensions
                    D3D11_TEXTURE2D_DESC depthDesc = {};
                    depthDesc.Width = rtDesc.Width;
                    depthDesc.Height = rtDesc.Height;
                    depthDesc.MipLevels = 1;
                    depthDesc.ArraySize = 1;
                    depthDesc.SampleDesc.Count = rtDesc.SampleDesc.Count;
                    depthDesc.SampleDesc.Quality = rtDesc.SampleDesc.Quality;
                    depthDesc.Usage = D3D11_USAGE_DEFAULT;
                    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

                    // Try a few common depth formats
                    DXGI_FORMAT formats[] = {
                        DXGI_FORMAT_D24_UNORM_S8_UINT,
                        DXGI_FORMAT_D32_FLOAT,
                        DXGI_FORMAT_D16_UNORM,
                        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
                        DXGI_FORMAT_R16_TYPELESS,
                        DXGI_FORMAT_R24G8_TYPELESS,
                        DXGI_FORMAT_R32_TYPELESS,
                        DXGI_FORMAT_R32G8X24_TYPELESS,
                    };

                    for (DXGI_FORMAT format : formats) {
                        depthDesc.Format = format;

                        ID3D11Texture2D* pDepthTexture = nullptr;
                        hr = _pLastDevice->CreateTexture2D(&depthDesc, nullptr, &pDepthTexture);

                        if (SUCCEEDED(hr) && pDepthTexture) {
                            // Try to create a DSV from this texture
                            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
                            dsvDesc.Format = format;
                            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                            dsvDesc.Texture2D.MipSlice = 0;

                            ID3D11DepthStencilView* pDSV = nullptr;
                            hr = _pLastDevice->CreateDepthStencilView(pDepthTexture, &dsvDesc, &pDSV);

                            if (SUCCEEDED(hr) && pDSV) {
                                // Store the depth texture description
                                D3D11_TEXTURE2D_DESC texDesc;
                                pDepthTexture->GetDesc(&texDesc);
                                _depthTextureDescs.push_back(texDesc);

                                // Store the texture
                                _detectedDepthTextures.push_back(pDepthTexture);

                                // Store the DSV
                                addDepthStencilView(pDSV);

                                pDSV->Release(); // addDepthStencilView adds its own reference

                                //MessageHandler::logDebug("D3DHook::scanForDepthBuffers: Created compatible depth buffer: %dx%d, format: %d",
                                //    texDesc.Width, texDesc.Height, texDesc.Format);
                            }
                            else {
                                // Failed to create DSV, release the texture
                                pDepthTexture->Release();
                            }
                        }
                    }

                    pRTTexture->Release();
                }

                pRTResource->Release();
            }
        }

        // Look for active depth buffers in the current depth stencil view
        if (_currentDepthStencilView) {
            ID3D11Resource* pDepthResource = nullptr;
            _currentDepthStencilView->GetResource(&pDepthResource);

            if (pDepthResource) {
                ID3D11Texture2D* pDepthTexture = nullptr;
                HRESULT hr = pDepthResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pDepthTexture);

                if (SUCCEEDED(hr) && pDepthTexture) {
                    // Get the description
                    D3D11_TEXTURE2D_DESC desc;
                    pDepthTexture->GetDesc(&desc);

                    // Add this to our collection (it might already be there)
                    bool found = false;
                    for (auto& tex : _detectedDepthTextures) {
                        if (tex == pDepthTexture) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        // Add reference and store
                        pDepthTexture->AddRef();
                        _detectedDepthTextures.push_back(pDepthTexture);
                        _depthTextureDescs.push_back(desc);

                        // Try to create a new DSV
                        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
                        dsvDesc.Format = getDepthViewFormat(desc.Format);
                        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        dsvDesc.Texture2D.MipSlice = 0;

                        ID3D11DepthStencilView* pDSV = nullptr;
                        hr = _pLastDevice->CreateDepthStencilView(pDepthTexture, &dsvDesc, &pDSV);

                        if (SUCCEEDED(hr) && pDSV) {
                            addDepthStencilView(pDSV);
                            pDSV->Release(); // addDepthStencilView adds its own reference
                        }

                        MessageHandler::logDebug("D3DHook::scanForDepthBuffers: Found active depth buffer: %dx%d, format: %d",
                            desc.Width, desc.Height, desc.Format);
                    }

                    pDepthTexture->Release();
                }

                pDepthResource->Release();
            }
        }
    }

    void D3DHook::captureActiveDepthBuffer(ID3D11DepthStencilView* pDSV) {
        if (!pDSV) {
            return;
        }

        // Store current DSV for potential capture in the next Present call
        _currentDepthStencilView = pDSV;

        // Try to extract information from this DSV
        ID3D11Resource* pResource = nullptr;
        pDSV->GetResource(&pResource);

        if (pResource) {
            ID3D11Texture2D* pTexture = nullptr;
            HRESULT hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);

            if (SUCCEEDED(hr) && pTexture) {
                // Get texture description
                D3D11_TEXTURE2D_DESC desc;
                pTexture->GetDesc(&desc);

                // Add this DSV to our collection if not already present
                bool foundMatch = false;

                // Look for a texture match in our existing collection
                for (size_t i = 0; i < _detectedDepthTextures.size(); i++) {
                    if (_detectedDepthTextures[i] == pTexture) {
                        foundMatch = true;
                        break;
                    }
                }

                if (!foundMatch) {
                    MessageHandler::logDebug("D3DHook::captureActiveDepthBuffer: Found active game depth buffer: %dx%d, format: %d",
                        desc.Width, desc.Height, desc.Format);

                    // Add reference to the texture
                    pTexture->AddRef();
                    _detectedDepthTextures.push_back(pTexture);
                    _depthTextureDescs.push_back(desc);

                    // Add the DSV to our collection
                    addDepthStencilView(pDSV);
                }

                pTexture->Release();
            }

            pResource->Release();
        }
    }

    void D3DHook::addDepthStencilView(ID3D11DepthStencilView* pDSV) {
        if (!pDSV) {
            return;
        }

        // Check if we already have this DSV
        for (auto& dsv : _detectedDepthStencilViews) {
            if (dsv == pDSV) {
                return; // Already in our list
            }
        }

        // Add a reference to the DSV
        pDSV->AddRef();
        _detectedDepthStencilViews.push_back(pDSV);

        // Extract the texture from this DSV
        extractTextureFromDSV(pDSV);

        // If this is our first DSV, set it as current
        if (_detectedDepthStencilViews.size() == 1) {
            _currentDepthBufferIndex = 0;
        }

        MessageHandler::logDebug("D3DHook::addDepthStencilView: Added DSV %p, total count: %zu",
            pDSV, _detectedDepthStencilViews.size());
    }

    void D3DHook::extractTextureFromDSV(ID3D11DepthStencilView* pDSV) {
        if (!pDSV) {
            return;
        }

        // Get the texture from the DSV
        ID3D11Resource* pResource = nullptr;
        pDSV->GetResource(&pResource);

        if (pResource) {
            ID3D11Texture2D* pTexture = nullptr;
            HRESULT hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
            pResource->Release();

            if (SUCCEEDED(hr) && pTexture) {
                // Check if we already have this texture
                bool found = false;
                for (auto& tex : _detectedDepthTextures) {
                    if (tex == pTexture) {
                        found = true;
                        pTexture->Release();
                        break;
                    }
                }

                if (!found) {
                    // Get the texture description
                    D3D11_TEXTURE2D_DESC desc;
                    pTexture->GetDesc(&desc);

                    // Store the texture and its description
                    _detectedDepthTextures.push_back(pTexture);
                    _depthTextureDescs.push_back(desc);

                    MessageHandler::logDebug("D3DHook::extractTextureFromDSV: Found depth texture %p, dimensions: %dx%d, format: %d",
                        pTexture, desc.Width, desc.Height, desc.Format);
                }
            }
        }
    }

    void D3DHook::selectAppropriateDepthBuffer() {
        if (_detectedDepthStencilViews.empty()) {
            return;
        }

        static bool foundmatch = false;

        if (foundmatch) {
            return;
        }

        /*MessageHandler::logDebug("D3DHook::selectAppropriateDepthBuffer: Selecting appropriate depth buffer...");*/

        // Get current viewport dimensions
        D3D11_VIEWPORT currentViewport;
        UINT numViewports = 1;
        _pLastContext->RSGetViewports(&numViewports, &currentViewport);

        UINT viewportWidth = static_cast<UINT>(currentViewport.Width);
        UINT viewportHeight = static_cast<UINT>(currentViewport.Height);

        // Get executable name without extension
        const char* exeName = Utils::getExecutableName();

        // Game-specific depth buffer selection
        if (strcmp(exeName, "darksoulsiii.exe") == 0) {
            // For Dark Souls 3, find 1280x720 format 19 buffer
            for (size_t i = 0; i < _depthTextureDescs.size(); i++) {
                const auto& desc = _depthTextureDescs[i];
                if (desc.Width == viewportWidth && desc.Height == viewportHeight && desc.Format == 19) {
                    _currentDepthBufferIndex = static_cast<int>(i);
                    _useDetectedDepthBuffer = true;
                    foundmatch = true;
                    MessageHandler::logLine("Automatically selected depth buffer for Dark Souls 3: %dx%d format %d",
                        desc.Width, desc.Height, desc.Format);
                    break;
                }
            }
        }
        // Add more game-specific matches here

        // If no game-specific match, try to find a buffer matching the viewport
        if (!foundmatch) {
            // Try exact match first
            for (size_t i = 0; i < _depthTextureDescs.size(); i++) {
                const auto& desc = _depthTextureDescs[i];
                if (desc.Width == viewportWidth && desc.Height == viewportHeight) {
                    _currentDepthBufferIndex = static_cast<int>(i);
                    _useDetectedDepthBuffer = true;
                    foundmatch = true;
                    MessageHandler::logDebug("Selected depth buffer matching viewport: %dx%d", desc.Width, desc.Height);
                    break;
                }
            }

            // If no exact match, try matching aspect ratio
            if (!foundmatch) {
                float viewportAspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

                for (size_t i = 0; i < _depthTextureDescs.size(); i++) {
                    const auto& desc = _depthTextureDescs[i];
                    if (desc.Width > 0 && desc.Height > 0) {
                        float bufferAspect = static_cast<float>(desc.Width) / static_cast<float>(desc.Height);

                        if (fabs(bufferAspect - viewportAspect) < 0.01f) {
                            _currentDepthBufferIndex = static_cast<int>(i);
                            _useDetectedDepthBuffer = true;
                            MessageHandler::logDebug("Selected depth buffer with matching aspect ratio: %dx%d",
                                desc.Width, desc.Height);
                            break;
                        }
                    }
                }
            }
        }

        // If no buffer selected yet, just use the first one
        if (_currentDepthBufferIndex < 0 && !_detectedDepthStencilViews.empty()) {
            _currentDepthBufferIndex = 0;
            _useDetectedDepthBuffer = true;

            if (_currentDepthBufferIndex < _depthTextureDescs.size()) {
                const auto& desc = _depthTextureDescs[_currentDepthBufferIndex];
                MessageHandler::logDebug("Selected first available depth buffer: %dx%d", desc.Width, desc.Height);
            }
        }
    }

    void D3DHook::cleanupDepthBufferResources() {
        // Release all DSVs
        for (auto& dsv : _detectedDepthStencilViews) {
            if (dsv) {
                dsv->Release();
            }
        }
        _detectedDepthStencilViews.clear();

        // Release all textures
        for (auto& tex : _detectedDepthTextures) {
            if (tex) {
                tex->Release();
            }
        }
        _detectedDepthTextures.clear();

        _depthTextureDescs.clear();
        _currentDepthBufferIndex = 0;
        _useDetectedDepthBuffer = false;

        // If we hooked OMSetRenderTargets, unhook it
        if (_hookingOMSetRenderTargets && _originalOMSetRenderTargets) {
            void** vtbl = *reinterpret_cast<void***>(_pLastContext);
            void* originalFunc = vtbl[33];
            MH_DisableHook(originalFunc);
            MH_RemoveHook(originalFunc);
            _hookingOMSetRenderTargets = false;
        }
    }

    bool D3DHook::isDepthFormat(DXGI_FORMAT format) {
        switch (format) {
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            // Typeless formats that can be used as depth
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return true;
        default:
            return false;
        }
    }

    DXGI_FORMAT D3DHook::getDepthViewFormat(DXGI_FORMAT resourceFormat) {
        switch (resourceFormat) {
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_D16_UNORM;
        case DXGI_FORMAT_R24G8_TYPELESS:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_D32_FLOAT;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            // Already depth formats
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return resourceFormat;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    //==================================================================================================
    // Sphere Rendering
    //==================================================================================================
    void D3DHook::createSolidColorSphere(float radius, const DirectX::XMFLOAT4& color) {
        // Clean up existing sphere resources
        if (_sphereVertexBuffer) { _sphereVertexBuffer->Release(); _sphereVertexBuffer = nullptr; }
        if (_sphereIndexBuffer) { _sphereIndexBuffer->Release(); _sphereIndexBuffer = nullptr; }

        // Create a simple sphere using stacks and slices
        const int stacks = 10;
        const int slices = 10;

        std::vector<SimpleVertex> vertices;
        std::vector<UINT> indices;

        // Add top vertex
        SimpleVertex topVertex;
        topVertex.position = DirectX::XMFLOAT3(0.0f, radius, 0.0f);
        topVertex.color = color;  // Use the provided solid color
        vertices.push_back(topVertex);

        // Add middle vertices
        for (int i = 1; i < stacks; i++) {
            float phi = DirectX::XM_PI * i / stacks;
            for (int j = 0; j < slices; j++) {
                float theta = DirectX::XM_2PI * j / slices;

                SimpleVertex vertex{};
                vertex.position.x = radius * sin(phi) * cos(theta);
                vertex.position.y = radius * cos(phi);
                vertex.position.z = radius * sin(phi) * sin(theta);

                // Use the provided solid color for all vertices
                vertex.color = color;

                vertices.push_back(vertex);
            }
        }

        // Add bottom vertex
        SimpleVertex bottomVertex;
        bottomVertex.position = DirectX::XMFLOAT3(0.0f, -radius, 0.0f);
        bottomVertex.color = color;  // Use the provided solid color
        vertices.push_back(bottomVertex);

        // Add top triangles
        for (int i = 0; i < slices; i++) {
            indices.push_back(0);
            indices.push_back(1 + (i + 1) % slices);
            indices.push_back(1 + i);
        }

        // Add middle triangles
        for (int i = 0; i < stacks - 2; i++) {
            for (int j = 0; j < slices; j++) {
                int base = 1 + i * slices;
                int nextBase = 1 + (i + 1) * slices;

                indices.push_back(base + j);
                indices.push_back(nextBase + j);
                indices.push_back(nextBase + (j + 1) % slices);

                indices.push_back(base + j);
                indices.push_back(nextBase + (j + 1) % slices);
                indices.push_back(base + (j + 1) % slices);
            }
        }

        // Add bottom triangles
        int bottomIndex = (int)vertices.size() - 1;
        int lastRingStart = bottomIndex - slices;
        for (int i = 0; i < slices; i++) {
            indices.push_back(bottomIndex);
            indices.push_back(lastRingStart + i);
            indices.push_back(lastRingStart + (i + 1) % slices);
        }

        // Create vertex buffer
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.ByteWidth = (UINT)(sizeof(SimpleVertex) * vertices.size());
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices.data();

        HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_sphereVertexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createSolidColorSphere: Failed to create vertex buffer");
            return;
        }

        // Create index buffer
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.ByteWidth = (UINT)(sizeof(UINT) * indices.size());
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indices.data();

        hr = _pLastDevice->CreateBuffer(&ibDesc, &ibData, &_sphereIndexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createSolidColorSphere: Failed to create index buffer");
            return;
        }

        _sphereIndexCount = (UINT)indices.size();
        //MessageHandler::logDebug("D3DHook::createSolidColorSphere: Created solid color sphere with %d vertices, %d indices",
        //    vertices.size(), indices.size());
    }

    //==================================================================================================
	// Arrow Geometry and Rendering
	//==================================================================================================
    void D3DHook::createArrowGeometry() {
        if (!_pLastDevice || !_pLastContext) {
            return;
        }

        // Release existing resources if they exist
        if (_arrowHeadVertexBuffer) { _arrowHeadVertexBuffer->Release(); _arrowHeadVertexBuffer = nullptr; }
        if (_arrowHeadIndexBuffer) { _arrowHeadIndexBuffer->Release(); _arrowHeadIndexBuffer = nullptr; }
        if (_arrowShaftVertexBuffer) { _arrowShaftVertexBuffer->Release(); _arrowShaftVertexBuffer = nullptr; }
        if (_arrowShaftIndexBuffer) { _arrowShaftIndexBuffer->Release(); _arrowShaftIndexBuffer = nullptr; }

        // Create vertices and indices for the arrow head (cone)
        std::vector<SimpleVertex> arrowHeadVertices;
        std::vector<UINT> arrowHeadIndices;

        // Arrow geometry should consistently point along +Z
        // Add apex vertex for the cone (arrow head)
        SimpleVertex apexVertex{};
        apexVertex.position = DirectX::XMFLOAT3(0.0f, 0.0f, _arrowHeadLength);  // Tip at positive Z
        apexVertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        arrowHeadVertices.push_back(apexVertex);

        // Create base vertices for the cone at Z=0
        for (int i = 0; i < _arrowSegments; i++) {
            float angle = DirectX::XM_2PI * i / _arrowSegments;
            float x = _arrowHeadRadius * cosf(angle);
            float y = _arrowHeadRadius * sinf(angle);

            SimpleVertex baseVertex{};
            baseVertex.position = DirectX::XMFLOAT3(x, y, 0.0f);
            baseVertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            arrowHeadVertices.push_back(baseVertex);
        }

        // Create triangles for the cone
        for (int i = 0; i < _arrowSegments; i++) {
            arrowHeadIndices.push_back(0); // Apex
            arrowHeadIndices.push_back(1 + i);
            arrowHeadIndices.push_back(1 + (i + 1) % _arrowSegments);
        }

        // Create base cap triangles
        SimpleVertex centerVertex;
        centerVertex.position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        centerVertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        arrowHeadVertices.push_back(centerVertex);

        int centerIndex = static_cast<int>(arrowHeadVertices.size()) - 1;

        for (int i = 0; i < _arrowSegments; i++) {
            arrowHeadIndices.push_back(centerIndex);
            arrowHeadIndices.push_back(1 + (i + 1) % _arrowSegments);
            arrowHeadIndices.push_back(1 + i);
        }

        // Create vertex buffer for arrow head
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(SimpleVertex) * arrowHeadVertices.size());
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = arrowHeadVertices.data();

        HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_arrowHeadVertexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createArrowGeometry: Failed to create arrow head vertex buffer");
            return;
        }

        // Create index buffer for arrow head
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * arrowHeadIndices.size());
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = arrowHeadIndices.data();

        hr = _pLastDevice->CreateBuffer(&ibDesc, &ibData, &_arrowHeadIndexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createArrowGeometry: Failed to create arrow head index buffer");
            return;
        }

        _arrowHeadIndexCount = static_cast<UINT>(arrowHeadIndices.size());

        // IMPORTANT: Create shaft that extends in +Z direction instead of -Z
        std::vector<SimpleVertex> arrowShaftVertices;
        std::vector<UINT> arrowShaftIndices;

        // Create vertices for the cylinder (front and back rings)
        for (int i = 0; i < _arrowSegments; i++) {
            float angle = DirectX::XM_2PI * i / _arrowSegments;
            float x = _arrowShaftRadius * cosf(angle);
            float y = _arrowShaftRadius * sinf(angle);

            // CHANGE: Back vertex at z=-1 (shaft start)
            SimpleVertex backVertex;
            backVertex.position = DirectX::XMFLOAT3(x, y, -1.0f);
            backVertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            arrowShaftVertices.push_back(backVertex);

            // CHANGE: Front vertex at z=0 (shaft end/head start)
            SimpleVertex frontVertex;
            frontVertex.position = DirectX::XMFLOAT3(x, y, 0.0f);
            frontVertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            arrowShaftVertices.push_back(frontVertex);
        }

        // Create triangles for the cylinder body
        for (int i = 0; i < _arrowSegments; i++) {
            int i0 = i * 2;                                 // Current back
            int i1 = i * 2 + 1;                             // Current front
            int i2 = ((i + 1) % _arrowSegments) * 2;        // Next back
            int i3 = ((i + 1) % _arrowSegments) * 2 + 1;    // Next front

            // First triangle - ensure correct winding order
            arrowShaftIndices.push_back(i0);
            arrowShaftIndices.push_back(i2);
            arrowShaftIndices.push_back(i1);

            // Second triangle - ensure correct winding order
            arrowShaftIndices.push_back(i1);
            arrowShaftIndices.push_back(i2);
            arrowShaftIndices.push_back(i3);
        }

        // Create vertex buffer for arrow shaft
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(SimpleVertex) * arrowShaftVertices.size());
        vbData.pSysMem = arrowShaftVertices.data();

        hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_arrowShaftVertexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createArrowGeometry: Failed to create arrow shaft vertex buffer");
            return;
        }

        // Create index buffer for arrow shaft
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * arrowShaftIndices.size());
        ibData.pSysMem = arrowShaftIndices.data();

        hr = _pLastDevice->CreateBuffer(&ibDesc, &ibData, &_arrowShaftIndexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createArrowGeometry: Failed to create arrow shaft index buffer");
            return;
        }

        _arrowShaftIndexCount = static_cast<UINT>(arrowShaftIndices.size());

       // MessageHandler::logDebug("D3DHook::createArrowGeometry: Successfully created arrow geometry");
    }

    void D3DHook::renderArrow(const DirectX::XMFLOAT3& position, const DirectX::XMVECTOR& direction,
        float length, const DirectX::XMFLOAT4& color) {
        if (!_pLastContext || !_arrowHeadVertexBuffer || !_arrowHeadIndexBuffer ||
            !_arrowShaftVertexBuffer || !_arrowShaftIndexBuffer) {
            return;
        }

        // Normalize direction
        DirectX::XMVECTOR normalizedDir = DirectX::XMVector3Normalize(direction);
        DirectX::XMVECTOR defaultDir = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // +Z axis

        // Calculate rotation quaternion from default to target direction
        DirectX::XMVECTOR rotationQuat;

        // Handle special cases for numerical stability
        DirectX::XMVECTOR dot = DirectX::XMVector3Dot(normalizedDir, defaultDir);
        float dotValue = DirectX::XMVectorGetX(dot);

        if (dotValue > 0.9999f) {
            // Nearly same direction - no rotation needed
            rotationQuat = DirectX::XMQuaternionIdentity();
        }
        else if (dotValue < -0.9999f) {
            // Nearly opposite direction - rotate 180° around Y axis
            rotationQuat = DirectX::XMQuaternionRotationAxis(
                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), DirectX::XM_PI);
        }
        else {
            // General case
            DirectX::XMVECTOR rotAxis = DirectX::XMVector3Cross(defaultDir, normalizedDir);
            rotAxis = DirectX::XMVector3Normalize(rotAxis);
            float angle = acosf(dotValue);
            rotationQuat = DirectX::XMQuaternionRotationAxis(rotAxis, angle);
        }

        // Create rotation matrix
        DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationQuaternion(rotationQuat);

        // Get camera matrices for view/projection
        //DirectX::XMFLOAT3 camPos = GameSpecific::CameraManipulator::getCurrentCameraCoords();
        //float* matrixInMemory = reinterpret_cast<float*>(GameSpecific::CameraManipulator::getCameraStruct() + MATRIX_IN_STRUCT_OFFSET);
        //DirectX::XMMATRIX cameraWorldMatrix;
        //memcpy(&cameraWorldMatrix, matrixInMemory, sizeof(DirectX::XMMATRIX));
        //DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixInverse(nullptr, cameraWorldMatrix);

        //// Get projection matrix
        //float fov = GameSpecific::CameraManipulator::getCurrentFoV();
        //D3D11_VIEWPORT viewport;
        //UINT numViewports = 1;
        //_pLastContext->RSGetViewports(&numViewports, &viewport);
        //float aspectRatio = viewport.Width / viewport.Height;
        //DirectX::XMMATRIX projMatrix = DirectX::XMMatrixPerspectiveFovLH(
        //    fov, aspectRatio, 0.08f, 10000.0f);

    	D3D11_VIEWPORT viewport;
        UINT numViewports = 1;
        _pLastContext->RSGetViewports(&numViewports, &viewport);
		XMMATRIX viewMatrix, projMatrix;
        setupCameraMatrices(viewport, viewMatrix, projMatrix);

        // Determine lengths - we want cone to be ~10% of total length
        float headLength = _arrowHeadLength;
        float shaftLength = length - headLength;

        // Set up rendering pipeline
        _pLastContext->IASetInputLayout(_simpleInputLayout);
        _pLastContext->VSSetShader(_simpleVertexShader, nullptr, 0);
        _pLastContext->PSSetShader(_coloredPixelShader, nullptr, 0);
        _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Update color buffer with the provided color
        D3D11_MAPPED_SUBRESOURCE colorMappedResource;
        if (SUCCEEDED(_pLastContext->Map(_colorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &colorMappedResource))) {
            DirectX::XMFLOAT4* cb = (DirectX::XMFLOAT4*)colorMappedResource.pData;
            *cb = color;  // Set the color
            _pLastContext->Unmap(_colorBuffer, 0);
        }

        // Bind the color buffer to pixel shader
        _pLastContext->PSSetConstantBuffers(0, 1, &_colorBuffer);

        // IMPORTANT: In our model, the shaft geometry is defined from z=-1 to z=0
        // and the cone is from z=0 to z=headLength
        // 1. Render shaft - position it exactly at the start point
        {
            // Create world matrix for shaft
            DirectX::XMMATRIX worldMatrix =
                // First translate along Z by 1 unit to make shaft start at 0 (model is from -1 to 0)
                DirectX::XMMatrixTranslation(0.0f, 0.0f, 1.0f) *
                // Scale to proper length
                DirectX::XMMatrixScaling(1.0f, 1.0f, shaftLength) *
                // Apply rotation to align with direction
                rotationMatrix *
                // Translate to starting position
                DirectX::XMMatrixTranslation(position.x, position.y, position.z);

            // Generate final matrix for vertex shader
            DirectX::XMMATRIX finalMatrix = DirectX::XMMatrixTranspose(
                worldMatrix * viewMatrix * projMatrix);

            // Update constant buffer
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                SimpleConstantBuffer* cb = (SimpleConstantBuffer*)mappedResource.pData;
                cb->worldViewProjection = finalMatrix;
                _pLastContext->Unmap(_constantBuffer, 0);
            }

            _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

            // Draw the shaft
            UINT stride = sizeof(SimpleVertex);
            UINT offset = 0;
            _pLastContext->IASetVertexBuffers(0, 1, &_arrowShaftVertexBuffer, &stride, &offset);
            _pLastContext->IASetIndexBuffer(_arrowShaftIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            _pLastContext->DrawIndexed(_arrowShaftIndexCount, 0, 0);
        }

        // 2. Render head (cone) - position it at the end of the shaft
        {
            // Calculate position of head (at the end of the shaft)
            DirectX::XMVECTOR headPosition = DirectX::XMVectorAdd(
                DirectX::XMLoadFloat3(&position),
                DirectX::XMVectorScale(normalizedDir, shaftLength)
            );

            // Convert to float3 for transformation
            DirectX::XMFLOAT3 headPos;
            DirectX::XMStoreFloat3(&headPos, headPosition);

            // Create world matrix for head
            DirectX::XMMATRIX worldMatrix =
                // Apply rotation to align with direction
                rotationMatrix *
                // Translate to head position
                DirectX::XMMatrixTranslation(headPos.x, headPos.y, headPos.z);

            // Generate final matrix for vertex shader
            DirectX::XMMATRIX finalMatrix = DirectX::XMMatrixTranspose(
                worldMatrix * viewMatrix * projMatrix);

            // Update constant buffer
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                SimpleConstantBuffer* cb = (SimpleConstantBuffer*)mappedResource.pData;
                cb->worldViewProjection = finalMatrix;
                _pLastContext->Unmap(_constantBuffer, 0);
            }

            _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

            // Draw the head
            UINT stride = sizeof(SimpleVertex);
            UINT offset = 0;
            _pLastContext->IASetVertexBuffers(0, 1, &_arrowHeadVertexBuffer, &stride, &offset);
            _pLastContext->IASetIndexBuffer(_arrowHeadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            _pLastContext->DrawIndexed(_arrowHeadIndexCount, 0, 0);
        }
    }

    //==================================================================================================
    // Tube Mesh Creation
    //==================================================================================================
    void D3DHook::createTubeMesh(std::vector<SimpleVertex>& tubeVertices, std::vector<UINT>& tubeIndices,
        const DirectX::XMFLOAT3& startPoint, const DirectX::XMFLOAT3& endPoint,
        const DirectX::XMFLOAT4& color, float radius, int segments) {
        // Calculate direction vector from start to end
        DirectX::XMVECTOR start = DirectX::XMLoadFloat3(&startPoint);
        DirectX::XMVECTOR end = DirectX::XMLoadFloat3(&endPoint);
        DirectX::XMVECTOR direction = DirectX::XMVectorSubtract(end, start);

        // Skip if points are too close
        if (DirectX::XMVector3LengthSq(direction).m128_f32[0] < 0.0001f) {
            return;
        }

        direction = DirectX::XMVector3Normalize(direction);

        // Find perpendicular vectors to create circle around tube
        DirectX::XMVECTOR right, up;

        // Try to use world up as reference
        DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(direction, worldUp));

        // If direction is nearly parallel to world up, use a different reference
        if (fabs(dot) > 0.99f) {
            worldUp = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // Calculate perpendicular basis
        right = DirectX::XMVector3Cross(worldUp, direction);
        right = DirectX::XMVector3Normalize(right);
        up = DirectX::XMVector3Cross(direction, right);
        up = DirectX::XMVector3Normalize(up);

        // Store the base index for connecting triangles
        UINT baseIndex = static_cast<UINT>(tubeVertices.size());

        // Create circle vertices at start and end points
        for (int i = 0; i <= segments; i++) {
            float angle = static_cast<float>(i) / segments * DirectX::XM_2PI;
            float cosA = cosf(angle);
            float sinA = sinf(angle);

            // Calculate offset from center line
            DirectX::XMVECTOR offset = DirectX::XMVectorAdd(
                DirectX::XMVectorScale(right, cosA * radius),
                DirectX::XMVectorScale(up, sinA * radius)
            );

            // Create vertex at start point
            SimpleVertex startVertex;
            DirectX::XMStoreFloat3(&startVertex.position, DirectX::XMVectorAdd(start, offset));
            startVertex.color = color;
            tubeVertices.push_back(startVertex);

            // Create vertex at end point
            SimpleVertex endVertex;
            DirectX::XMStoreFloat3(&endVertex.position, DirectX::XMVectorAdd(end, offset));
            endVertex.color = color;
            tubeVertices.push_back(endVertex);
        }

        // Create triangles connecting the circles
        for (int i = 0; i < segments; i++) {
            // Each segment has two vertices (start and end)
            int i0 = baseIndex + i * 2;
            int i1 = baseIndex + i * 2 + 1;
            int i2 = baseIndex + (i + 1) * 2;
            int i3 = baseIndex + (i + 1) * 2 + 1;

            // Triangle 1
            tubeIndices.push_back(i0);
            tubeIndices.push_back(i1);
            tubeIndices.push_back(i2);

            // Triangle 2
            tubeIndices.push_back(i2);
            tubeIndices.push_back(i1);
            tubeIndices.push_back(i3);
        }
    }

    //==================================================================================================
// Path Visualization Implementation
//==================================================================================================
    void D3DHook::createPathVisualization() {
        if (!_pLastDevice || !_pLastContext) {
            MessageHandler::logError("D3DHook::createPathVisualization: Device or Context is null");
            return;
        }

        // If we're in the middle of changing modes, wait until it's safe
        if (_isChangingMode.load(std::memory_order_acquire)) {
            MessageHandler::logDebug("D3DHook::createPathVisualization: Mode change in progress, deferring resource creation");
            _resourcesNeedUpdate.store(true, std::memory_order_release);
            return;
        }

        // Release existing resources
        auto releaseBuffer = [](ID3D11Buffer*& buffer) {
            if (buffer) {
                buffer->Release();
                buffer = nullptr;
            }
            };

        releaseBuffer(_pathVertexBuffer);
        releaseBuffer(_nodeVertexBuffer);
        releaseBuffer(_interpolatedPathVertexBuffer);
        releaseBuffer(_directionIndicatorVertexBuffer);
        releaseBuffer(_pathTubeVertexBuffer);
        releaseBuffer(_pathTubeIndexBuffer);
        releaseBuffer(_interpolatedDirectionsVertexBuffer);

        _pathResourcesCreated = false;
        _pathInfos.clear();
        _nodePositions.clear();

        // Get access to the path manager
        if (!&CameraPathManager::instance()) {
            MessageHandler::logError("D3DHook::createPathVisualization: Failed to get path manager");
            return;
        }

        // Get all the paths
        const std::unordered_map<std::string, CameraPath>& paths = CameraPathManager::instance().getPaths();
        if (paths.empty()) {
            //// Log occasionally
            //static int frameCount = 0;
            //frameCount++;
            //if (frameCount % 500 == 0) {
            //    MessageHandler::logDebug("D3DHook::createPathVisualization: No paths found to visualize");
            //}
            return;
        }

        // Prepare vertex collections
        std::vector<SimpleVertex> nodeVertices;          // For node markers
        std::vector<SimpleVertex> directionIndicatorVertices; // For camera orientation at nodes
        std::vector<SimpleVertex> interpolatedDirectionVertices; // For direction indicators along path

        // For tube geometry
        std::vector<SimpleVertex> tubeMeshVertices;
        std::vector<UINT> tubeMeshIndices;

        // Set base color for nodes
        DirectX::XMFLOAT4 nodeColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for nodes
        DirectX::XMFLOAT4 directionColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // Red for direction indicators

        std::vector<TubeInfo> pathTubeInfos;

        // Process each path separately
        processAllPaths(paths, nodeVertices, directionIndicatorVertices, interpolatedDirectionVertices,
            tubeMeshVertices, tubeMeshIndices, pathTubeInfos, nodeColor, directionColor);

        // Store total vertex and index counts
        _nodeVertexCount = static_cast<UINT>(nodeVertices.size());
        _directionIndicatorVertexCount = static_cast<UINT>(directionIndicatorVertices.size());
        _pathTubeVertexCount = static_cast<UINT>(tubeMeshVertices.size());
        _pathTubeIndexCount = static_cast<UINT>(tubeMeshIndices.size());
        _interpolatedDirectionsVertexCount = static_cast<UINT>(interpolatedDirectionVertices.size());

        // Create vertex buffers if we have data
        createPathBuffers(nodeVertices, directionIndicatorVertices,
            interpolatedDirectionVertices, tubeMeshVertices, tubeMeshIndices,
            pathTubeInfos);

        _pathResourcesCreated = true;
        //MessageHandler::logDebug("D3DHook::createPathVisualization: Successfully created path visualization resources");
        /*MessageHandler::logDebug("D3DHook::createPathVisualization: Paths: %zu, Nodes: %u, Tube vertices: %u, Tube indices: %u, Interpolated directions: %u",
            _pathInfos.size(), _nodePositions.size(), _pathTubeVertexCount, _pathTubeIndexCount, _interpolatedDirectionsVertexCount);*/
    }

    // Helper method to process all camera paths
    void D3DHook::processAllPaths(const std::unordered_map<std::string, CameraPath>& paths,
        std::vector<SimpleVertex>& nodeVertices,
        std::vector<SimpleVertex>& directionIndicatorVertices,
        std::vector<SimpleVertex>& interpolatedDirectionVertices,
        std::vector<SimpleVertex>& tubeMeshVertices,
        std::vector<UINT>& tubeMeshIndices,
        std::vector<TubeInfo>& pathTubeInfos,
        const DirectX::XMFLOAT4& nodeColor,
        const DirectX::XMFLOAT4& directionColor) {
        int pathIndex = 0;
        for (const auto& pathPair : paths) {
            const std::string& pathName = pathPair.first;
            const CameraPath& path = pathPair.second;
            const std::vector<CameraNode>* nodes = path.getNodes();
            size_t nodeCount = path.GetNodeCount();

            if (nodeCount < 2) // Need at least 2 nodes to form a path
                continue;

            //MessageHandler::logDebug("D3DHook::processAllPaths: Processing path '%s' with %zu nodes",
            //    pathName.c_str(), nodeCount);

            // Generate a unique color for this path based on golden ratio
            float hue = (pathIndex * 137.5f) / 360.0f; // Golden ratio to distribute colors
            float saturation = 0.8f;
            float value = 0.9f;

            // Convert HSV to RGB for path color
            DirectX::XMFLOAT4 pathColor = convertHSVtoRGB(hue, saturation, value, 0.9f);

            // Create path info record for this path
            PathInfo pathInfo;
            pathInfo.name = pathName;
            pathInfo.nodeStartIndex = static_cast<UINT>(_nodePositions.size());
            pathInfo.directionStartIndex = static_cast<UINT>(directionIndicatorVertices.size());
            pathInfo.pathColor = pathColor;

            // Start tracking tube indices for this path
            TubeInfo tubeInfo;
            tubeInfo.startIndex = static_cast<UINT>(tubeMeshIndices.size());
            tubeInfo.color = pathColor;

            // Process the nodes for this path
            processPathNodes(path, nodeCount, nodeVertices, directionIndicatorVertices, nodeColor, directionColor);

            // Generate interpolated samples for this path
            auto interpSamples = generateInterpolatedSamples(path, nodeCount);

            // Create interpolated direction indicators
            size_t interpDirStart = interpolatedDirectionVertices.size();
            createInterpolatedDirections(interpSamples, interpolatedDirectionVertices);

            // Calculate how many interpolated direction vertices were added for this path
            size_t interpDirCount = interpolatedDirectionVertices.size() - interpDirStart;

            // Store this path's interpolated directions
            storePathDirections(pathInfo, interpolatedDirectionVertices, interpDirStart, interpDirCount);

            // Create interpolated path points from the samples
            std::vector<DirectX::XMFLOAT3> interpPoints;
            for (const auto& sample : interpSamples) {
                interpPoints.push_back(sample.position);
            }

            // Create tube geometry between interpolated points
            createPathTubes(interpPoints, tubeMeshVertices, tubeMeshIndices, pathColor);

            // Update path info with final counts
            pathInfo.nodeCount = static_cast<UINT>(_nodePositions.size() - pathInfo.nodeStartIndex);
            pathInfo.directionVertexCount = static_cast<UINT>(directionIndicatorVertices.size() - pathInfo.directionStartIndex);

            // Update tube info
            tubeInfo.indexCount = static_cast<UINT>(tubeMeshIndices.size() - tubeInfo.startIndex);
            pathTubeInfos.push_back(tubeInfo);

            // Add path info to collection
            _pathInfos.push_back(pathInfo);

            pathIndex++;
        }
    }

    // Helper method to process nodes for a path
    void D3DHook::processPathNodes(const CameraPath& path, size_t nodeCount,
        std::vector<SimpleVertex>& nodeVertices,
        std::vector<SimpleVertex>& directionIndicatorVertices,
        const DirectX::XMFLOAT4& nodeColor,
        const DirectX::XMFLOAT4& directionColor) {
        for (size_t i = 0; i < nodeCount; i++) {
            DirectX::XMFLOAT3 pos = path.getNodePositionFloat(i);

            // Store node position for rendering spheres
            _nodePositions.push_back(pos);

            // Create vertex for node marker
            SimpleVertex nodeVertex;
            nodeVertex.position = pos;
            nodeVertex.color = nodeColor;
            nodeVertices.push_back(nodeVertex);

            // Create direction indicator
            DirectX::XMVECTOR rotation = path.getNodeRotation(i);
            DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
                DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                rotation // Rotate by the camera's rotation
            );

            // Start and end points for direction line
            SimpleVertex dirStart, dirEnd;
            dirStart.position = pos;
            dirStart.color = directionColor;

            // Calculate end point by extending in the forward direction
            DirectX::XMFLOAT3 forwardFloat;
            DirectX::XMStoreFloat3(&forwardFloat, forward);

            dirEnd.position.x = pos.x + forwardFloat.x * _directionLength;
            dirEnd.position.y = pos.y + forwardFloat.y * _directionLength;
            dirEnd.position.z = pos.z + forwardFloat.z * _directionLength;
            dirEnd.color = directionColor;

            directionIndicatorVertices.push_back(dirStart);
            directionIndicatorVertices.push_back(dirEnd);
        }
    }

    // Helper to convert HSV color to RGB
    DirectX::XMFLOAT4 D3DHook::convertHSVtoRGB(float hue, float saturation, float value, float alpha) {
        float chroma = value * saturation;
        float huePrime = hue * 6.0f;
        float x = chroma * (1.0f - std::abs(std::fmod(huePrime, 2.0f) - 1.0f));

        float r = 0.0f, g = 0.0f, b = 0.0f;
        if (huePrime < 1.0f) { r = chroma; g = x; b = 0.0f; }
        else if (huePrime < 2.0f) { r = x; g = chroma; b = 0.0f; }
        else if (huePrime < 3.0f) { r = 0.0f; g = chroma; b = x; }
        else if (huePrime < 4.0f) { r = 0.0f; g = x; b = chroma; }
        else if (huePrime < 5.0f) { r = x; g = 0.0f; b = chroma; }
        else { r = chroma; g = 0.0f; b = x; }

        float m = value - chroma;
        r += m; g += m; b += m;

        return DirectX::XMFLOAT4(r, g, b, alpha);
    }

    // Helper to generate interpolated samples along a path
    std::vector<D3DHook::InterpSample> D3DHook::generateInterpolatedSamples(const CameraPath& path, size_t nodeCount) {
        std::vector<InterpSample> interpSamples;
        const std::vector<CameraNode>* nodes = path.getNodes();
        const std::vector<float>* knotVector = path.getknotVector();
        const std::vector<DirectX::XMVECTOR>* controlPointsPos = path.getControlPointsPos();
        const std::vector<float>* controlPointsFoV = path.getControlPointsFOV();
        const std::vector<DirectX::XMVECTOR>* controlPointsRot = path.getControlPointsRot();

        // Generate samples for each path segment
        for (size_t i = 0; i < nodeCount - 1; i++) {
            for (int step = 0; step <= _interpolationSteps; step++) {
                float t = static_cast<float>(step) / static_cast<float>(_interpolationSteps);
                float globalT = static_cast<float>(i) + t;

                DirectX::XMVECTOR interpPoint;
                DirectX::XMVECTOR interpRotation;
                float interpFOV;

                // Position interpolation based on current mode
                switch (static_cast<InterpolationMode>(Globals::instance().settings().positionInterpMode)) {
                case InterpolationMode::CatmullRom:
                    interpPoint = CatmullRomPositionInterpolation(globalT, *nodes, path);
                    interpFOV = CatmullRomFoV(globalT, *nodes, path);
                    break;
                case InterpolationMode::Centripetal:
                    interpPoint = CentripetalPositionInterpolation(globalT, *nodes, path);
                    interpFOV = CatmullRomFoV(globalT, *nodes, path);
                    break;
                case InterpolationMode::Bezier:
                    interpPoint = BezierPositionInterpolation(globalT, *nodes);
                    interpFOV = BezierFoV(globalT, *nodes);
                    break;
                case InterpolationMode::BSpline:
                    interpPoint = BSplinePositionInterpolation(globalT, *knotVector, *controlPointsPos);
                    interpFOV = BSplineFoV(globalT, *knotVector, *controlPointsFoV);
                    break;
                case InterpolationMode::Cubic:
                    interpPoint = CubicPositionInterpolation_Smooth(globalT, *nodes, path);
                    interpFOV = CubicFoV_Smooth(globalT, *nodes, path);
                    break;
                }

                // Rotation interpolation based on rotation mode
                switch (static_cast<RotationMode>(Globals::instance().settings()._rotationMode)) {
                case RotationMode::Standard:
                    switch (static_cast<InterpolationMode>(Globals::instance().settings().positionInterpMode)) {
                    case InterpolationMode::CatmullRom:
                        interpRotation = CatmullRomRotationInterpolation(globalT, *nodes, path);
                        break;
                    case InterpolationMode::Centripetal:
                        interpRotation = CentripetalRotationInterpolation(globalT, *nodes, path);
                        break;
                    case InterpolationMode::Bezier:
                        interpRotation = BezierRotationInterpolation(globalT, *nodes);
                        break;
                    case InterpolationMode::BSpline:
                        interpRotation = BSplineRotationInterpolation(globalT, *knotVector, *controlPointsRot);
                        break;
                    case InterpolationMode::Cubic:
                        interpRotation = CubicRotationInterpolation(globalT, *nodes, path);
                        break;
                    }
                    break;
                case RotationMode::SQUAD:
                    interpRotation = PathUtils::SquadRotationInterpolation(globalT, *nodes);
                    break;
                case RotationMode::RiemannCubic:
                    interpRotation = RiemannCubicRotationMonotonic(globalT, *nodes, path);
                    break;
                }

                // Store the sample
                InterpSample sample;
                DirectX::XMStoreFloat3(&sample.position, interpPoint);
                sample.rotation = interpRotation;
                sample.fov = interpFOV;
                interpSamples.push_back(sample);
            }
        }

        return interpSamples;
    }

    // Helper to create interpolated direction indicators
    void D3DHook::createInterpolatedDirections(const std::vector<InterpSample>& interpSamples,
        std::vector<SimpleVertex>& interpolatedDirectionVertices) {
        for (size_t i = 0; i < interpSamples.size(); i += _rotationSampleFrequency) {
            // Only create indicators at every Nth sample to avoid cluttering
            const InterpSample& sample = interpSamples[i];

            // Calculate direction indicator length based on FOV
            // Inverse relationship: lower FOV = longer line (more zoomed in)
            float fovScaleFactor = _defaultFOV / sample.fov;
            float scaledLength = _baseDirectionLength * fovScaleFactor;

            // Calculate forward vector from rotation quaternion
            DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
                DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                sample.rotation // Apply rotation
            );

            // Create indicator vertices
            SimpleVertex dirStart, dirEnd;
            dirStart.position = sample.position;
            dirStart.color = DirectX::XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f); // Orange for path indicators

            // Calculate end point
            DirectX::XMFLOAT3 forwardFloat;
            DirectX::XMStoreFloat3(&forwardFloat, forward);

            dirEnd.position.x = sample.position.x + forwardFloat.x * scaledLength;
            dirEnd.position.y = sample.position.y + forwardFloat.y * scaledLength;
            dirEnd.position.z = sample.position.z + forwardFloat.z * scaledLength;
            dirEnd.color = DirectX::XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f);

            interpolatedDirectionVertices.push_back(dirStart);
            interpolatedDirectionVertices.push_back(dirEnd);
        }
    }

    // Helper to store path direction data
    void D3DHook::storePathDirections(PathInfo& pathInfo,
        const std::vector<SimpleVertex>& interpolatedDirectionVertices,
        size_t startIndex, size_t count) {
        pathInfo.pathDirections.clear();
        for (size_t i = startIndex; i < startIndex + count; i += 2) {
            pathInfo.pathDirections.push_back(std::make_pair(
                interpolatedDirectionVertices[i].position,
                interpolatedDirectionVertices[i + 1].position
            ));
        }
    }

    // Helper to create tubes for path visualization
    void D3DHook::createPathTubes(const std::vector<DirectX::XMFLOAT3>& points,
        std::vector<SimpleVertex>& tubeMeshVertices,
        std::vector<UINT>& tubeMeshIndices,
        const DirectX::XMFLOAT4& color) {
        for (size_t i = 0; i < points.size() - 1; i++) {
            createTubeMesh(tubeMeshVertices, tubeMeshIndices,
                points[i], points[i + 1],
                color, _tubeDiameter / 2.0f, _tubeSegments);
        }
    }

    // Helper to create Direct3D vertex and index buffers
    void D3DHook::createPathBuffers(const std::vector<SimpleVertex>& nodeVertices,
        const std::vector<SimpleVertex>& directionIndicatorVertices,
        const std::vector<SimpleVertex>& interpolatedDirectionVertices,
        const std::vector<SimpleVertex>& tubeMeshVertices,
        const std::vector<UINT>& tubeMeshIndices,
        const std::vector<TubeInfo>& pathTubeInfos) {
        // Create node vertex buffer
        if (_nodeVertexCount > 0) {
            D3D11_BUFFER_DESC vbDesc = {};
            vbDesc.Usage = D3D11_USAGE_DEFAULT;
            vbDesc.ByteWidth = sizeof(SimpleVertex) * _nodeVertexCount;
            vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vbData = {};
            vbData.pSysMem = nodeVertices.data();

            HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_nodeVertexBuffer);
            if (FAILED(hr)) {
                MessageHandler::logError("D3DHook::createPathBuffers: Failed to create node vertex buffer");
                return;
            }
        }

        // Create direction indicator vertex buffer
        if (_directionIndicatorVertexCount > 0) {
            D3D11_BUFFER_DESC vbDesc = {};
            vbDesc.Usage = D3D11_USAGE_DEFAULT;
            vbDesc.ByteWidth = sizeof(SimpleVertex) * _directionIndicatorVertexCount;
            vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vbData = {};
            vbData.pSysMem = directionIndicatorVertices.data();

            HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_directionIndicatorVertexBuffer);
            if (FAILED(hr)) {
                MessageHandler::logError("D3DHook::createPathBuffers: Failed to create direction indicator vertex buffer");
                return;
            }
        }

        // Create interpolated direction buffer
        if (_interpolatedDirectionsVertexCount > 0) {
            D3D11_BUFFER_DESC vbDesc = {};
            vbDesc.Usage = D3D11_USAGE_DEFAULT;
            vbDesc.ByteWidth = sizeof(SimpleVertex) * _interpolatedDirectionsVertexCount;
            vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vbData = {};
            vbData.pSysMem = interpolatedDirectionVertices.data();

            HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_interpolatedDirectionsVertexBuffer);
            if (FAILED(hr)) {
                MessageHandler::logError("D3DHook::createPathBuffers: Failed to create interpolated directions buffer");
            }
        }

        // Create tube mesh buffers
        if (_pathTubeVertexCount > 0 && _pathTubeIndexCount > 0) {
            // Vertex buffer
            D3D11_BUFFER_DESC vbDesc = {};
            vbDesc.Usage = D3D11_USAGE_DEFAULT;
            vbDesc.ByteWidth = sizeof(SimpleVertex) * _pathTubeVertexCount;
            vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vbData = {};
            vbData.pSysMem = tubeMeshVertices.data();

            HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_pathTubeVertexBuffer);
            if (FAILED(hr)) {
                MessageHandler::logError("D3DHook::createPathBuffers: Failed to create tube vertex buffer");
                return;
            }

            // Index buffer
            D3D11_BUFFER_DESC ibDesc = {};
            ibDesc.Usage = D3D11_USAGE_DEFAULT;
            ibDesc.ByteWidth = sizeof(UINT) * _pathTubeIndexCount;
            ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA ibData = {};
            ibData.pSysMem = tubeMeshIndices.data();

            hr = _pLastDevice->CreateBuffer(&ibDesc, &ibData, &_pathTubeIndexBuffer);
            if (FAILED(hr)) {
                MessageHandler::logError("D3DHook::createPathBuffers: Failed to create tube index buffer");
                return;
            }

            // Store tube infos with paths
            for (size_t i = 0; i < pathTubeInfos.size() && i < _pathInfos.size(); i++) {
                _pathInfos[i].interpStartIndex = pathTubeInfos[i].startIndex;
                _pathInfos[i].interpVertexCount = pathTubeInfos[i].indexCount;
            }
        }
    }

    //==================================================================================================
    // Path Rendering Implementation
    //==================================================================================================
    void D3DHook::renderPaths() {
        if (!_pLastContext || !_visualizationEnabled) {
            return;
        }

        // Don't render if we're in the middle of changing modes
        if (_isChangingMode.load(std::memory_order_acquire)) {
            return;
        }

        // Check if resources need update
        if (_resourcesNeedUpdate.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(_resourceMutex);
            if (_resourcesNeedUpdate.load(std::memory_order_acquire)) { // double-check after acquiring lock
                createPathVisualization();
                _resourcesNeedUpdate.store(false, std::memory_order_release);
            }
        }

        // Create path resources if they don't exist
        if (!_pathResourcesCreated) {
            std::lock_guard<std::mutex> lock(_resourceMutex);
            if (!_pathResourcesCreated) { // double-check after acquiring lock
                createPathVisualization();
                if (!_pathResourcesCreated) // If creation failed
                    return;
            }
        }

        // Safety check for device resources
        if (!_pLastDevice || !_pLastRTV) {
            return;
        }

        // Save and setup rendering state
        ID3D11RenderTargetView* pCurrentRTV = nullptr;
        ID3D11DepthStencilView* pCurrentDSV = nullptr;
        ID3D11BlendState* pCurrentBlendState = nullptr;
        ID3D11DepthStencilState* pCurrentDSState = nullptr;
        D3D11_VIEWPORT currentViewport;
        FLOAT currentBlendFactor[4];
        UINT currentSampleMask;
        UINT currentStencilRef;
        UINT numViewports = 1;

        // Save the current state
        saveRenderState(pCurrentRTV, pCurrentDSV, pCurrentBlendState, pCurrentDSState,
            currentViewport, currentBlendFactor, currentSampleMask, currentStencilRef,
            numViewports);

        // Setup depth buffer for proper occlusion
        ID3D11DepthStencilView* pDepthView = setupDepthBuffer();
        ID3D11DepthStencilState* pDepthState = createDepthState(pDepthView);

        // Set our render target with the selected depth buffer
        _pLastContext->OMSetRenderTargets(1, &_pLastRTV, pDepthView);
        _pLastContext->RSSetViewports(1, &currentViewport);

        // Get camera matrices for rendering
        DirectX::XMMATRIX viewMatrix, projMatrix;
        setupCameraMatrices(currentViewport, viewMatrix, projMatrix);

        // Create blend state for transparent rendering
        ID3D11BlendState* pBlendState = createBlendState();
        _pLastContext->OMSetBlendState(pBlendState, nullptr, 0xffffffff);

        // Setup shaders and pipeline state
        _pLastContext->IASetInputLayout(_simpleInputLayout);
        _pLastContext->VSSetShader(_simpleVertexShader, nullptr, 0);
        _pLastContext->PSSetShader(_simplePixelShader, nullptr, 0);

        // Render each component in order
        renderPathTubes(viewMatrix, projMatrix);
        renderDirectionArrows(viewMatrix, projMatrix);
        renderNodeSpheres(viewMatrix, projMatrix);

        //debug viz
        //renderDebugMarkers();

        // Release the blend state
        if (pBlendState) pBlendState->Release();
        if (pDepthState) pDepthState->Release();

        // Restore previous state
        restoreRenderState(pCurrentRTV, pCurrentDSV, pCurrentBlendState, pCurrentDSState,
            currentViewport, currentBlendFactor, currentSampleMask, currentStencilRef);
    }

    // Helper to save the current render state
    void D3DHook::saveRenderState(ID3D11RenderTargetView*& pCurrentRTV,
        ID3D11DepthStencilView*& pCurrentDSV,
        ID3D11BlendState*& pCurrentBlendState,
        ID3D11DepthStencilState*& pCurrentDSState,
        D3D11_VIEWPORT& currentViewport,
        FLOAT currentBlendFactor[4],
        UINT& currentSampleMask,
        UINT& currentStencilRef,
        UINT& numViewports) {
        _pLastContext->OMGetRenderTargets(1, &pCurrentRTV, &pCurrentDSV);
        _pLastContext->OMGetBlendState(&pCurrentBlendState, currentBlendFactor, &currentSampleMask);
        _pLastContext->OMGetDepthStencilState(&pCurrentDSState, &currentStencilRef);
        _pLastContext->RSGetViewports(&numViewports, &currentViewport);
    }

    // Helper to setup depth buffer for rendering
    ID3D11DepthStencilView* D3DHook::setupDepthBuffer() {
        ID3D11DepthStencilView* pDepthView = nullptr;
        if (_useDetectedDepthBuffer && !_detectedDepthStencilViews.empty() &&
            _currentDepthBufferIndex < _detectedDepthStencilViews.size()) {
            pDepthView = _detectedDepthStencilViews[_currentDepthBufferIndex];
        }
        return pDepthView;
    }

    // Helper to create depth state for rendering
    ID3D11DepthStencilState* D3DHook::createDepthState(ID3D11DepthStencilView* pDepthView) {
        ID3D11DepthStencilState* pDepthState = nullptr;
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};

        if (pDepthView) {
            // Enable depth testing when we have a depth buffer
            depthDesc.DepthEnable = TRUE;
            depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Don't write to depth
            depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Use LESS_EQUAL for better results
        }
        else {
            // Disable depth testing when no depth buffer
            depthDesc.DepthEnable = FALSE;
        }

        HRESULT hr = _pLastDevice->CreateDepthStencilState(&depthDesc, &pDepthState);
        if (SUCCEEDED(hr) && pDepthState) {
            _pLastContext->OMSetDepthStencilState(pDepthState, 0);
        }

        return pDepthState;
    }

    // Helper to setup camera matrices for rendering
    //void D3DHook::setupCameraMatrices(const D3D11_VIEWPORT& viewport,
    //    DirectX::XMMATRIX& viewMatrix,
    //    DirectX::XMMATRIX& projMatrix) {
    //    // Get camera position and matrix from the game
    //    DirectX::XMFLOAT3 camPos = GameSpecific::CameraManipulator::getCurrentCameraCoords();

    //    float* matrixInMemory = reinterpret_cast<float*>(
    //        GameSpecific::CameraManipulator::getCameraStruct() + MATRIX_IN_STRUCT_OFFSET);

    //    DirectX::XMMATRIX cameraWorldMatrix;
    //    memcpy(&cameraWorldMatrix, matrixInMemory, sizeof(DirectX::XMMATRIX));

    //    // Convert camera world matrix to view matrix by inverting it
    //    viewMatrix = DirectX::XMMatrixInverse(nullptr, cameraWorldMatrix);

    //    // Create projection matrix
    //    float fov = GameSpecific::CameraManipulator::getCurrentFoV();
    //    float aspectRatio = viewport.Width / viewport.Height;
    //    projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, 0.08f, 10000.0f);
    //}

    void D3DHook::setupCameraMatrices(const D3D11_VIEWPORT& viewport,
        DirectX::XMMATRIX& viewMatrix,
        DirectX::XMMATRIX& projMatrix) {

         //Get camera position from game memory
        DirectX::XMFLOAT3 camPos = GameSpecific::CameraManipulator::getCurrentCameraCoords();

        // Get the camera matrix from game memory
        float* matrixInMemory = reinterpret_cast<float*>(
            GameSpecific::CameraManipulator::getCameraStruct() + MATRIX_IN_STRUCT_OFFSET);

        DirectX::XMMATRIX cameraWorldMatrix;
        memcpy(&cameraWorldMatrix, matrixInMemory, sizeof(XMMATRIX));

        XMVECTOR dt = XMMatrixDeterminant(cameraWorldMatrix);

        // Important: Use the same exact matrix for rendering paths as what's used for the game camera
        // This ensures perfect alignment regardless of interpolation status
        viewMatrix = DirectX::XMMatrixInverse(&dt, cameraWorldMatrix);
		

        // Use the exact same FOV as the game is currently using
        float fov = GameSpecific::CameraManipulator::getCurrentFoV();
        // Ensure FOV is within safe range
        fov = max(0.01f, min(fov, 3.0f));
        float aspectRatio = viewport.Width / viewport.Height;
        projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, 0.08f, 10000.0f);
    }

    // Helper to create blend state for transparent rendering
    ID3D11BlendState* D3DHook::createBlendState() {
        ID3D11BlendState* pBlendState = nullptr;
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        _pLastDevice->CreateBlendState(&blendDesc, &pBlendState);
        return pBlendState;
    }


    // Helper to render tube paths
    void D3DHook::renderPathTubes(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
        if (!_pathTubeVertexBuffer || !_pathTubeIndexBuffer || _pathTubeIndexCount == 0) {
            return;
        }

        // Update constant buffer
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            SimpleConstantBuffer* cb = (SimpleConstantBuffer*)mappedResource.pData;

            DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();
            cb->worldViewProjection = DirectX::XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

            _pLastContext->Unmap(_constantBuffer, 0);
        }

        _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

        // Set up for indexed drawing
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        _pLastContext->IASetVertexBuffers(0, 1, &_pathTubeVertexBuffer, &stride, &offset);
        _pLastContext->IASetIndexBuffer(_pathTubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Draw each path's tube segment
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != CameraPathManager::instance().getSelectedPath()) {
                continue;
            }

            if (pathInfo.interpVertexCount > 0) {
                _pLastContext->DrawIndexed(pathInfo.interpVertexCount, pathInfo.interpStartIndex, 0);
            }
        }
    }

    // Helper to render direction arrows
    void D3DHook::renderDirectionArrows(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
        // Get all paths for arrow rendering
        const std::unordered_map<std::string, CameraPath>& paths = CameraPathManager::instance().getPaths();

        // Render arrows for interpolated direction indicators
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != CameraPathManager::instance().getSelectedPath()) {
                continue;
            }

            // Render this path's direction arrows
            for (const auto& dirPair : pathInfo.pathDirections) {
                // First value is the position
                const DirectX::XMFLOAT3& position = dirPair.first;

                // Second value is the end position (not a direction)
                // Calculate direction vector from position to end position
                DirectX::XMVECTOR direction = DirectX::XMVectorSubtract(
                    DirectX::XMLoadFloat3(&dirPair.second),
                    DirectX::XMLoadFloat3(&position)
                );

                // Calculate length
                float length = DirectX::XMVectorGetX(DirectX::XMVector3Length(direction));

                // Use orange color for interpolated path indicators
                DirectX::XMFLOAT4 arrowColor = DirectX::XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f);

                // Render arrow
                renderArrow(position, direction, length, arrowColor);
            }
        }

        // Render arrows for node direction indicators
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != CameraPathManager::instance().getSelectedPath()) {
                continue;
            }

            // For each node in this path, render an arrow instead of a line
            for (UINT i = 0; i < pathInfo.nodeCount; i++) {
                UINT nodeIndex = pathInfo.nodeStartIndex + i;
                if (nodeIndex >= _nodePositions.size()) {
                    continue;
                }

                // Find the path by name to get rotation data
                auto pathIter = paths.find(pathInfo.name);
                if (pathIter != paths.end()) {
                    const CameraPath& path = pathIter->second;

                    // Get position and direction data
                    DirectX::XMFLOAT3 nodePos = _nodePositions[nodeIndex];
                    DirectX::XMVECTOR rotation = path.getNodeRotation(i);
                    DirectX::XMVECTOR direction = DirectX::XMVector3Rotate(
                        DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                        rotation // Rotate by the camera's rotation
                    );

                    // Get FOV for this node and calculate scaled length
                    float nodeFov = path.getNodeFOV(i);
                    // Inverse relationship: lower FOV = longer line (more zoomed in)
                    float fovScaleFactor = _defaultFOV / nodeFov;
                    float scaledLength = _baseDirectionLength * fovScaleFactor;

                    // Red color for node direction indicators
                    DirectX::XMFLOAT4 arrowColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

                    // Render a 3D arrow instead of a line
                    renderArrow(nodePos, direction, scaledLength, arrowColor);
                }
            }
        }
    }

    // Helper to render node spheres
    void D3DHook::renderNodeSpheres(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) {
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != CameraPathManager::instance().getSelectedPath()) {
                continue;
            }

            if (_sphereVertexBuffer && _sphereIndexBuffer && pathInfo.nodeCount > 0) {
                // For each node in this path, render a sphere at its position
                for (UINT i = 0; i < pathInfo.nodeCount; i++) {
                    UINT nodeIndex = pathInfo.nodeStartIndex + i;
                    if (nodeIndex >= _nodePositions.size()) {
                        continue;
                    }

                    // Get the node position
                    DirectX::XMVECTOR nodePosition = XMLoadFloat3(&_nodePositions[nodeIndex]);

                    // Update constant buffer for this node's sphere
                    D3D11_MAPPED_SUBRESOURCE mappedResource;
                    if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                        SimpleConstantBuffer* cb = (SimpleConstantBuffer*)mappedResource.pData;

                        // Create world matrix for the sphere at this node position
                        DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixScaling(_nodeSize, _nodeSize, _nodeSize) *
                            DirectX::XMMatrixTranslationFromVector(nodePosition);

                        // Combine matrices
                        cb->worldViewProjection = DirectX::XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

                        _pLastContext->Unmap(_constantBuffer, 0);
                    }

                    _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

                    // Set up the sphere rendering
                    UINT stride = sizeof(SimpleVertex);
                    UINT offset = 0;
                    _pLastContext->IASetVertexBuffers(0, 1, &_sphereVertexBuffer, &stride, &offset);
                    _pLastContext->IASetIndexBuffer(_sphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
                    _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                    // Explicitly use the simple pixel shader that uses vertex colors
                    _pLastContext->PSSetShader(_simplePixelShader, nullptr, 0);

                    // Draw the sphere
                    _pLastContext->DrawIndexed(_sphereIndexCount, 0, 0);
                }
            }
        }
    }

    // Helper to restore the render state
    void D3DHook::restoreRenderState(ID3D11RenderTargetView* pCurrentRTV,
        ID3D11DepthStencilView* pCurrentDSV,
        ID3D11BlendState* pCurrentBlendState,
        ID3D11DepthStencilState* pCurrentDSState,
        const D3D11_VIEWPORT& currentViewport,
        const FLOAT currentBlendFactor[4],
        UINT currentSampleMask,
        UINT currentStencilRef) {
        _pLastContext->OMSetRenderTargets(1, &pCurrentRTV, pCurrentDSV);
        _pLastContext->OMSetBlendState(pCurrentBlendState, currentBlendFactor, currentSampleMask);
        _pLastContext->RSSetViewports(1, &currentViewport);
        _pLastContext->OMSetDepthStencilState(pCurrentDSState, currentStencilRef);

        // Release references
        if (pCurrentRTV) pCurrentRTV->Release();
        if (pCurrentDSV) pCurrentDSV->Release();
        if (pCurrentBlendState) pCurrentBlendState->Release();
        if (pCurrentDSState) pCurrentDSState->Release();
    }

} // namespace IGCS


