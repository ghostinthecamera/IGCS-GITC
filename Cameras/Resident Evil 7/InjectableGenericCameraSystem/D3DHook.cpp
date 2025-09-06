// ReSharper disable CppTooWideScope
// ReSharper disable CppUnreachableCode
// ReSharper disable CppRedundantBooleanExpressionArgument
#include "stdafx.h"
#include "D3DHook.h"
#include "Globals.h"
#include "MessageHandler.h"
#include "CameraManipulator.h"
#include "MinHook.h"
#include "PathManager.h"
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "Bezier.h"
#include "CentripetalCatmullRom.h"
#include "CatmullRom.h"
#include "BSpline.h"
#include "Cubic.h"
#include "PathUtils.h"
#include "RiemannCubic.h"
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

	//==================================================================================================
	// Getters, setters and other methods
	//==================================================================================================
    void D3DHook::setVisualization(const bool enabled) {
        _visualizationEnabled = enabled;
        MessageHandler::logDebug("D3DHook::setVisualization: Visualization is now %s", _visualizationEnabled ? "ON" : "OFF");
    }

    bool D3DHook::isVisualizationEnabled() const {
        return _visualizationEnabled;
    }

    void D3DHook::setRenderPathOnly(const bool renderPathOnly) {
        _renderSelectedPathOnly = renderPathOnly;
        MessageHandler::logDebug("D3DHook::setRenderPathOnly: Render selected path only is now %s",
            _renderSelectedPathOnly ? "ON" : "OFF");
    }

    bool D3DHook::isRenderPathOnly() const {
        return _renderSelectedPathOnly;
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


    bool D3DHook::isUsingDepthBuffer() const {
        return _useDetectedDepthBuffer;
    }

    void D3DHook::toggleDepthBufferUsage() {
        _useDetectedDepthBuffer = !_useDetectedDepthBuffer;
        MessageHandler::logDebug("D3DHook::toggleDepthBufferUsage: Depth buffer usage is now %s",
            _useDetectedDepthBuffer ? "ON" : "OFF");

        // Force recreation of any depth-related resources on next frame
        if (_pDepthStencilState) {
            _pDepthStencilState->Release();
            _pDepthStencilState = nullptr;
        }
    }

    //need to update this to newer D3D12 approach
    float D3DHook::getAspectRatio() const
    {
		if (_windowHeight == 0) return 1.77778f; // Default to 16:9 if height is zero to avoid division by zero
		return static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight);
    }

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
    D3DHook::D3DHook(): _renderSelectedPathOnly(false)
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
        const HWND hWnd = Globals::instance().mainWindowHandle();
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

        const HRESULT hr = D3D11CreateDeviceAndSwapChain(
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
        void* resizeBuffersAddress = vftable[13]; // ResizeBuffers is at index 13

        // Hook Present first
        MessageHandler::logDebug("D3DHook::initialize: Hooking Present function at %p", presentAddress);

        // Create the Present hook
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

        // Enable the Present hook
        status = MH_EnableHook(presentAddress);
        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::initialize: Failed to enable Present hook, error: %d", status);
            pTempContext->Release();
            pTempDevice->Release();
            pTempSwapChain->Release();
            return false;
        }

        MessageHandler::logLine("Successfully hooked Direct3D 11 Present function");

        // Now hook ResizeBuffers
        MessageHandler::logDebug("D3DHook::initialize: Hooking ResizeBuffers function at %p", resizeBuffersAddress);

        // Create the ResizeBuffers hook
        status = MH_CreateHook(
            resizeBuffersAddress,
            &D3DHook::hookedResizeBuffers,
            reinterpret_cast<void**>(&_originalResizeBuffers)
        );

        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::initialize: Failed to create ResizeBuffers hook, error: %d", status);
            // Note: Present hook is already active at this point
            // You might want to disable it if ResizeBuffers fails
            pTempContext->Release();
            pTempDevice->Release();
            pTempSwapChain->Release();
            return false;
        }

        // Enable the ResizeBuffers hook
        status = MH_EnableHook(resizeBuffersAddress);
        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::initialize: Failed to enable ResizeBuffers hook, error: %d", status);
            // Note: Present hook is already active at this point
            // You might want to disable it if ResizeBuffers fails
            pTempContext->Release();
            pTempDevice->Release();
            pTempSwapChain->Release();
            return false;
        }

        MessageHandler::logLine("Successfully hooked Direct3D 11 ResizeBuffers function");

        // Clean up temporary objects - we don't need them anymore
        pTempContext->Release();
        pTempDevice->Release();
        pTempSwapChain->Release();

        // Initialize render target view and associated states
        //createRenderTargetView();

        _isInitialized = true;
        return true;
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

        // Create render target view and associated states
        createRenderTargetView();

        // Start scanning for depth buffers
        scanForDepthBuffers();

        // Initialize rendering resources
        if (initializeRenderingResources()) {
            MessageHandler::logDebug("D3DHook::performQueuedInitialization: Rendering resources initialized");
        }
        else {
            MessageHandler::logError("D3DHook::performQueuedInitialization: Failed to initialize rendering resources");
        }

        // Mark initialization as complete
        _needsInitialization = false;
        _isInitialized = true;
        _renderTargetInitialized = true;

        MessageHandler::logDebug("D3DHook::performQueuedInitialization: Initialization complete");
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

    //==================================================================================================
    // Resource Management Methods
    //==================================================================================================
    void D3DHook::cleanUp() {
        static bool cleanupDone = false;
        if (cleanupDone) {
            return;
        }

        if (!_isInitialized) {
            cleanupDone = true;
            return;
        }

        MessageHandler::logDebug("D3DHook::cleanUp: Starting D3D cleanup");

        // 1. First, disable all hooks
        if (_hookingOMSetRenderTargets && _originalOMSetRenderTargets && _pLastContext) {
            void** vtbl = *reinterpret_cast<void***>(_pLastContext);
            if (vtbl) {
                void* originalFunc = vtbl[33];
                MH_DisableHook(originalFunc);
                MH_RemoveHook(originalFunc);
            }
            _hookingOMSetRenderTargets = false;
        }

        // Disable Present and ResizeBuffers hooks
        if (_originalPresent) {
            MH_DisableHook(_originalPresent);
            MH_RemoveHook(_originalPresent);
        }

        if (_originalResizeBuffers) {
            MH_DisableHook(_originalResizeBuffers);
            MH_RemoveHook(_originalResizeBuffers);
        }

        // 2. Clean up all resources
        cleanupAllResources();

        // 3. Cleanup MinHook
        //MH_Uninitialize();

        // 4. Final flag reset
        _visualizationEnabled = false;
        _renderSelectedPathOnly = false;

        cleanupDone = true;

        MessageHandler::logDebug("D3DHook::cleanUp: D3D cleanup complete");
    }

	void D3DHook::cleanupAllResources() {
        std::lock_guard<std::mutex> lock(_resourceMutex);

        // Helper macro for safe release
		#define SAFE_RELEASE(obj) if(obj) { (obj)->Release(); (obj) = nullptr; }

        MessageHandler::logDebug("D3DHook::cleanupAllResources: Starting comprehensive resource cleanup");

        // 1. Release Shader Resources
        SAFE_RELEASE(_simpleVertexShader);
        SAFE_RELEASE(_simplePixelShader);
        SAFE_RELEASE(_coloredPixelShader);
        SAFE_RELEASE(_simpleInputLayout);

        // 2. Release Buffer Resources
        SAFE_RELEASE(_constantBuffer);
        SAFE_RELEASE(_colorBuffer);

        // 3. Release Sphere Geometry Resources
        SAFE_RELEASE(_sphereVertexBuffer);
        SAFE_RELEASE(_sphereIndexBuffer);

        // 4. Release Arrow Geometry Resources
        SAFE_RELEASE(_arrowHeadVertexBuffer);
        SAFE_RELEASE(_arrowHeadIndexBuffer);
        SAFE_RELEASE(_arrowShaftVertexBuffer);
        SAFE_RELEASE(_arrowShaftIndexBuffer);

        // 5. Release Path Visualization Resources
        SAFE_RELEASE(_pathVertexBuffer);
        SAFE_RELEASE(_nodeVertexBuffer);
        SAFE_RELEASE(_interpolatedPathVertexBuffer);
        SAFE_RELEASE(_directionIndicatorVertexBuffer);
        SAFE_RELEASE(_pathTubeVertexBuffer);
        SAFE_RELEASE(_pathTubeIndexBuffer);
        SAFE_RELEASE(_interpolatedDirectionsVertexBuffer);

        // 6. Release Rendering State Resources
        SAFE_RELEASE(_pBlendState);
        SAFE_RELEASE(_pDepthStencilState);
        SAFE_RELEASE(_pRasterizerState);

        // 7. Release Core D3D Instance Resources
        SAFE_RELEASE(_pRenderTargetView);
        SAFE_RELEASE(_pContext);
        SAFE_RELEASE(_pDevice);
        SAFE_RELEASE(_pSwapChain);

        // 8. Release Static D3D Resources
        SAFE_RELEASE(_pLastRTV);
        SAFE_RELEASE(_pLastContext);
        SAFE_RELEASE(_pLastDevice);

        SAFE_RELEASE(_lookAtTargetVertexBuffer); SAFE_RELEASE(_lookAtTargetIndexBuffer);
        _lookAtTargetIndexCount = 0;

        // 10. Clean up Depth Buffer Resources
        // Release all DSVs
        for (auto& dsv : _detectedDepthStencilViews) {
            SAFE_RELEASE(dsv);
        }
        _detectedDepthStencilViews.clear();

        // Release all depth textures
        for (auto& tex : _detectedDepthTextures) {
            SAFE_RELEASE(tex);
        }
        _detectedDepthTextures.clear();
        _depthTextureDescs.clear();

        // 11. Reset Variables and Flags
        _currentDepthBufferIndex = 0;
        _useDetectedDepthBuffer = false;
        _renderTargetInitialized = false;
        _pathResourcesCreated = false;
        _isInitialized = false;
        _windowWidth = 0;
        _windowHeight = 0;
        _sphereIndexCount = 0;
        _pathVertexCount = 0;
        _nodeVertexCount = 0;
        _interpolatedPathVertexCount = 0;
        _directionIndicatorVertexCount = 0;
        _pathTubeVertexCount = 0;
        _pathTubeIndexCount = 0;
        _interpolatedDirectionsVertexCount = 0;
        _arrowHeadIndexCount = 0;
        _arrowShaftIndexCount = 0;

        // 12. Clear Data Structures
        _pathInfos.clear();
        _nodePositions.clear();

        // 13. Reset Atomic Flags
        _needsInitialization.store(false, std::memory_order_release);
        _isChangingMode.store(false, std::memory_order_release);
        _resourcesNeedUpdate.store(false, std::memory_order_release);

		#undef SAFE_RELEASE

        MessageHandler::logDebug("D3DHook::cleanupAllResources: Cleanup complete");
    }

    void D3DHook::cleanupDepthBufferResources() {
        // Release all DSVs
        for (const auto& dsv : _detectedDepthStencilViews) {
            if (dsv) {
                dsv->Release();
            }
        }
        _detectedDepthStencilViews.clear();

        // Release all textures
        for (const auto& tex : _detectedDepthTextures) {
            if (tex) {
                tex->Release();
            }
        }
        _detectedDepthTextures.clear();

        _depthTextureDescs.clear();
        _currentDepthBufferIndex = 0;
        _useDetectedDepthBuffer = false;

        MessageHandler::logDebug("D3DHook::cleanupDepthBufferResources: Cleared all depth buffer resources");
    }

    //==================================================================================================
	// Initialization Methods
	//==================================================================================================
    bool D3DHook::createRenderTargetView() {
        // Use _pDevice and _pSwapChain (instance members)
        if (!_pDevice || !_pSwapChain) {
            // Try falling back to static members if instance members aren't set
            if (!_pLastDevice) {
                MessageHandler::logError("D3DHook::createRenderTargetView: No device available (both _pDevice and _pLastDevice are null)");
                return false;
            }

            MessageHandler::logDebug("D3DHook::createRenderTargetView: Using static _pLastDevice as fallback");

            // If we have _pLastDevice but not _pDevice, sync them
            if (!_pDevice) {
                _pDevice = _pLastDevice;
                _pDevice->AddRef(); // Important: add reference
            }

            if (!_pContext && _pLastContext) {
                _pContext = _pLastContext;
                _pContext->AddRef(); // Important: add reference
            }

            // If we still don't have a swap chain, we can't proceed
            if (!_pSwapChain) {
                MessageHandler::logError("D3DHook::createRenderTargetView: SwapChain is null, cannot create RTV");
                return false;
            }
        }

        // Release any existing RTV
        if (_pRenderTargetView) {
            _pRenderTargetView->Release();
            _pRenderTargetView = nullptr;
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

        // Now create the rendering states - blend, depth, rasterizer

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
        D3D11_RASTERIZER_DESC rasterizerDesc;
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;  // No culling
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthBias = 0;
        rasterizerDesc.DepthBiasClamp = 0.0f;
        rasterizerDesc.SlopeScaledDepthBias = 0.0f;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.ScissorEnable = FALSE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;

        hr = _pDevice->CreateRasterizerState(&rasterizerDesc, &_pRasterizerState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createRenderTargetView: Failed to create rasterizer state, error: 0x%X", hr);
            return false;
        }

        // Get the back buffer dimensions and store them
        if (pBackBuffer) {
            D3D11_TEXTURE2D_DESC desc;
            pBackBuffer->GetDesc(&desc);
            _windowWidth = desc.Width;
            _windowHeight = desc.Height;
            MessageHandler::logDebug("D3DHook::createRenderTargetView: Back buffer dimensions: %ux%u", _windowWidth, _windowHeight);
        }

        _renderTargetInitialized = true;
        MessageHandler::logDebug("D3DHook::createRenderTargetView: Successfully created render target view and states");
        return true;
    }

    D3D11_VIEWPORT D3DHook::getFullViewport() {
        D3D11_VIEWPORT viewport;

        // Default fallback dimensions (if we can't get actual dimensions)
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = 1280.0f;
        viewport.Height = 720.0f;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        // Method 1: Try to get dimensions from stored window dimensions
        if (_windowWidth > 0 && _windowHeight > 0) {
            viewport.Width = static_cast<float>(_windowWidth);
            viewport.Height = static_cast<float>(_windowHeight);
            return viewport;
        }

        // Method 2: Try to get dimensions from the swap chain's back buffer
        if (_pSwapChain) {
            ID3D11Texture2D* pBackBuffer = nullptr;
            if (SUCCEEDED(_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                D3D11_TEXTURE2D_DESC desc;
                pBackBuffer->GetDesc(&desc);
                viewport.Width = static_cast<float>(desc.Width);
                viewport.Height = static_cast<float>(desc.Height);

                // Store these dimensions for future use
                _windowWidth = desc.Width;
                _windowHeight = desc.Height;

                pBackBuffer->Release();

                MessageHandler::logDebug("D3DHook::getFullViewport: Got dimensions from back buffer: %.1f x %.1f",
                    viewport.Width, viewport.Height);
                return viewport;
            }
        }

        // Method 3: Try to get current viewport from context
        if (_pLastContext) {
            UINT numViewports = 1;
            D3D11_VIEWPORT currentViewport;
            _pLastContext->RSGetViewports(&numViewports, &currentViewport);

            if (numViewports > 0 && currentViewport.Width > 0 && currentViewport.Height > 0) {
                MessageHandler::logDebug("D3DHook::getFullViewport: Got dimensions from active viewport: %.1f x %.1f",
                    currentViewport.Width, currentViewport.Height);
                return currentViewport;
            }
        }

        // Method 4: Try to get dimensions from game window
        const HWND hWnd = Globals::instance().mainWindowHandle();
        if (hWnd) {
            RECT clientRect;
            if (GetClientRect(hWnd, &clientRect)) {
                viewport.Width = static_cast<float>(clientRect.right - clientRect.left);
                viewport.Height = static_cast<float>(clientRect.bottom - clientRect.top);

                // Store these dimensions for future use
                _windowWidth = static_cast<UINT>(viewport.Width);
                _windowHeight = static_cast<UINT>(viewport.Height);

                MessageHandler::logDebug("D3DHook::getFullViewport: Got dimensions from window: %.1f x %.1f",
                    viewport.Width, viewport.Height);
                return viewport;
            }
        }

        // Using default fallback dimensions if all methods fail
        MessageHandler::logDebug("D3DHook::getFullViewport: Using default fallback dimensions: %.1f x %.1f",
            viewport.Width, viewport.Height);
        return viewport;
    }

    bool D3DHook::initializeRenderingResources() {

    	if (!instance()._pLastDevice || !instance()._pLastContext) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Device or Context is null");
            return false;
        }

        // 1. Create shaders
        // Simple vertex shader
	    const auto vsCode = R"(
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
	    const auto psCode = R"(
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

	    const auto coloredPsCode = R"(
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
            BlobGuard(ID3DBlob** blobs, const int count) : _blobs(blobs), _count(count) {}
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
        hr = instance()._pLastDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &instance()._simpleVertexShader);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create vertex shader");
            return false;
        }

        hr = instance()._pLastDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &instance()._simplePixelShader);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create pixel shader");
            return false;
        }

        // Create the colored pixel shader
        hr = instance()._pLastDevice->CreatePixelShader(coloredPsBlob->GetBufferPointer(),
            coloredPsBlob->GetBufferSize(), nullptr, &instance()._coloredPixelShader);
        if (FAILED(hr)) {
            MessageHandler::logError("Failed to create colored pixel shader");
            return false;
        }

        // Create input layout
        const D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = instance()._pLastDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &instance()._simpleInputLayout);
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

        hr = instance()._pLastDevice->CreateBuffer(&cbDesc, nullptr, &instance()._constantBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create constant buffer");
            return false;
        }

        //// Create blend state for transparency
        //D3D11_BLEND_DESC blendDesc = {};
        //blendDesc.RenderTarget[0].BlendEnable = TRUE;
        //blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        //blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        //blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        //blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        //blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        //blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        //blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        //hr = instance()._pLastDevice->CreateBlendState(&blendDesc, &instance()._blendState);
        //if (FAILED(hr)) {
        //    MessageHandler::logError("D3DHook::initializeRenderingResources: Failed to create blend state");
        //    return false;
        //}

        // Create the color buffer
        D3D11_BUFFER_DESC colorBufferDesc = {};
        colorBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        colorBufferDesc.ByteWidth = sizeof(XMFLOAT4);
        colorBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        colorBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = instance()._pLastDevice->CreateBuffer(&colorBufferDesc, nullptr, &instance()._colorBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("Failed to create color buffer");
            return false;
        }

        // Create a simple sphere
        instance().createSolidColorSphere(1.0f, XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));

        //create targetoffset sphere
        instance().createLookAtTargetSphere(0.5f, XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

        // Create path visualization resources
        instance().createPathVisualization();

        // Create arrow geometry for direction indicators
        instance().createArrowGeometry();

        return true;
    }

    //==================================================================================================
    // Hook Functions
    //==================================================================================================
    HRESULT STDMETHODCALLTYPE D3DHook::hookedPresent(IDXGISwapChain* pSwapChain, const UINT SyncInterval, const UINT Flags) {
        static uint32_t frameCounter = 0;
        frameCounter = (frameCounter + 1) % 60;

        // Device acquisition - if we don't have a device yet
        if (!instance()._pLastDevice && pSwapChain) {
            bool needsInit = false;
            {
                std::lock_guard<std::mutex> lock(instance()._resourceMutex);
                if (!instance()._pLastDevice) { // Double-check after acquiring lock
                    ID3D11Device* pDevice = nullptr;
                    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice)))) {
                        // Set static members
                        instance()._pLastDevice = pDevice;
                        pDevice->GetImmediateContext(&instance()._pLastContext);

                        // Also set instance members - CRITICAL ADDITION
                        D3DHook& hook = instance();
                        hook._pDevice = pDevice; // Set instance device
                        hook._pSwapChain = pSwapChain; // Set instance swap chain
                        pSwapChain->AddRef(); // Important: add reference since we're storing it

                        pDevice->GetImmediateContext(&hook._pContext); // Set instance context

                        MessageHandler::logDebug("D3DHook::hookedPresent: Successfully captured device, context, and swap chain");
                        needsInit = true;
                    }
                }
            }

            // Signal that initialization is needed, but don't do it here
            if (needsInit) {
                instance().queueInitialization();
            }
        }

        // Always update RTV on each frame to ensure we're using the correct one
        if (instance()._pLastDevice && pSwapChain) {
            // Release old RTV if it exists
            if (instance()._pLastRTV) {
                instance()._pLastRTV->Release();
                instance()._pLastRTV = nullptr;
            }

            // Get back buffer for current frame
            ID3D11Texture2D* pBackBuffer = nullptr;
            HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
            if (SUCCEEDED(hr) && pBackBuffer) {
                hr = instance()._pLastDevice->CreateRenderTargetView(pBackBuffer, nullptr, &instance()._pLastRTV);
                if (FAILED(hr)) {
                    MessageHandler::logError("Failed to create RTV in Present hook: 0x%X", hr);
                }
                pBackBuffer->Release();
            }
        }

        // Process depth buffer detection occasionally
        if (frameCounter == 0) {
            instance().scanForDepthBuffers();
        }

        if (!instance()._detectedDepthStencilViews.empty() && !instance()._depthMatchFound) {
            instance().selectAppropriateDepthBuffer();
        }

        // Render paths if visualization is enabled AND we have a valid RTV
        if (instance()._pLastContext && instance()._pLastDevice && instance()._pLastRTV && instance().isVisualizationEnabled()) {
            instance().renderPaths();
        }

        if (instance()._pLastContext && instance()._pLastDevice && instance()._pLastRTV && Camera::instance().getCameraLookAtVisualizationEnabled()) {
            instance().renderFreeCameraLookAtTarget();
        }

        // System update if active
        if (Globals::instance().systemActive() && RUN_IN_HOOKED_PRESENT) {
            System::instance().updateFrame();
        }

        // Call the original Present function
        return _originalPresent(pSwapChain, SyncInterval, Flags);
    }

    HRESULT STDMETHODCALLTYPE D3DHook::hookedResizeBuffers(IDXGISwapChain* pSwapChain, const UINT BufferCount,
        const UINT Width, const UINT Height, const DXGI_FORMAT NewFormat, const UINT SwapChainFlags) {

        MessageHandler::logDebug("D3DHook::hookedResizeBuffers: Resizing to %ux%u", Width, Height);

        D3DHook& hook = instance();

        // Lock to prevent concurrent access during resize
        std::lock_guard<std::mutex> lock(hook._resourceMutex);

        // Step 1: Release ALL resources connected to the swap chain

        // Release RTVs
        if (instance()._pLastRTV) {
            instance()._pLastRTV->Release();
            instance()._pLastRTV = nullptr;
        }

        if (hook._pRenderTargetView) {
            hook._pRenderTargetView->Release();
            hook._pRenderTargetView = nullptr;
        }

        // Clean up depth buffer resources
        hook.cleanupDepthBufferResources();

        // Release visualization resources
        if (hook._pathVertexBuffer) { hook._pathVertexBuffer->Release(); hook._pathVertexBuffer = nullptr; }
        if (hook._nodeVertexBuffer) { hook._nodeVertexBuffer->Release(); hook._nodeVertexBuffer = nullptr; }
        if (hook._interpolatedPathVertexBuffer) { hook._interpolatedPathVertexBuffer->Release(); hook._interpolatedPathVertexBuffer = nullptr; }
        if (hook._directionIndicatorVertexBuffer) { hook._directionIndicatorVertexBuffer->Release(); hook._directionIndicatorVertexBuffer = nullptr; }
        if (hook._pathTubeVertexBuffer) { hook._pathTubeVertexBuffer->Release(); hook._pathTubeVertexBuffer = nullptr; }
        if (hook._pathTubeIndexBuffer) { hook._pathTubeIndexBuffer->Release(); hook._pathTubeIndexBuffer = nullptr; }
        if (hook._interpolatedDirectionsVertexBuffer) { hook._interpolatedDirectionsVertexBuffer->Release(); hook._interpolatedDirectionsVertexBuffer = nullptr; }

        // Reset state flags
        hook._renderTargetInitialized = false;
        hook._pathResourcesCreated = false;
		hook._depthMatchFound = false;

        // Step 2: Update dimensions
        hook._windowWidth = Width;
        hook._windowHeight = Height;

        // Step 3: Call original function to perform the actual resize
        const HRESULT result = hook._originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

        // Step 4: Handle the result and recreate resources if successful
        if (FAILED(result)) {
            MessageHandler::logError("D3DHook::hookedResizeBuffers: Failed with error 0x%X", result);
        }
        else {
            MessageHandler::logDebug("D3DHook::hookedResizeBuffers: Resize successful, recreating resources");

            // Recreate RTVs if possible
            if (instance()._pLastDevice && pSwapChain) {
                ID3D11Texture2D* pBackBuffer = nullptr;
                if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                    // Create the static RTV
                    instance()._pLastDevice->CreateRenderTargetView(pBackBuffer, nullptr, &instance()._pLastRTV);

                    // Also recreate the instance RTV if we have a device
                    if (hook._pDevice) {
                        hook._pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &hook._pRenderTargetView);
                    }

                    pBackBuffer->Release();
                    MessageHandler::logDebug("D3DHook::hookedResizeBuffers: Successfully recreated RTVs");
                }
            }

            // Signal that other resources need to be recreated
            hook._resourcesNeedUpdate.store(true, std::memory_order_release);
        }

        return result;
    }

    void STDMETHODCALLTYPE D3DHook::hookedOMSetRenderTargets(ID3D11DeviceContext* pContext, const UINT NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView* pDepthStencilView) {
        // Call original function first
        _originalOMSetRenderTargets(pContext, NumViews, ppRenderTargetViews, pDepthStencilView);

        // Only proceed if we're hooking and the depth buffer is valid
        if (!instance()._hookingOMSetRenderTargets || !pDepthStencilView) {
            return;
        }

        // Prevent recursion
        static bool inOurCode = false;
        if (inOurCode) {
            return;
        }
        inOurCode = true;

        // Capture this depth stencil view for later processing
        instance()._currentDepthStencilView = pDepthStencilView;
        instance().captureActiveDepthBuffer(pDepthStencilView);

        // Reset recursion flag
        inOurCode = false;
    }

    //==================================================================================================
	// Depth Buffer Management
	//==================================================================================================
    void D3DHook::setupOMSetRenderTargetsHook() {
        // Early return if already hooked or context is null
        if (_hookingOMSetRenderTargets || !_pLastContext) {
            return;
        }

        MessageHandler::logDebug("D3DHook::setupOMSetRenderTargetsHook: Setting up hook to capture actual game depth buffers");

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

        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::setupOMSetRenderTargetsHook: Failed to create hook, error: %d", status);
            return;
        }

        // Enable the hook
        status = MH_EnableHook(originalFunc);
        if (status != MH_OK) {
            MessageHandler::logError("D3DHook::setupOMSetRenderTargetsHook: Failed to enable hook, error: %d", status);
            return;
        }

        // Mark as hooked
        _hookingOMSetRenderTargets = true;
        MessageHandler::logDebug("D3DHook::setupOMSetRenderTargetsHook: Successfully hooked OMSetRenderTargets");
    }

    void D3DHook::scanForDepthBuffers() {
        if (!_pLastDevice) {
            return;
        }

        // Limit scan frequency
        static int scanCount = 0;
        scanCount++;
        if (scanCount % 60 != 0) {
            return;
        }

        if (D3DLOGGING && VERBOSE)
            MessageHandler::logDebug("D3DHook::scanForDepthBuffers: Scanning for actual game depth buffers...");

        // Make sure the hook for OMSetRenderTargets is set up
        if (!_hookingOMSetRenderTargets && _pLastContext) {
            setupOMSetRenderTargetsHook();
        }

        // Check the current depth buffer if one is active
        if (_currentDepthStencilView) {
            captureActiveDepthBuffer(_currentDepthStencilView);
        }
    }

    void D3DHook::captureActiveDepthBuffer(ID3D11DepthStencilView* pDSV) {
        if (!pDSV) {
            return;
        }

        // Get the resource (texture) from the view
        ID3D11Resource* pResource = nullptr;
        pDSV->GetResource(&pResource);
        if (!pResource) {
            return;
        }

        // Query for the texture interface
        ID3D11Texture2D* pTexture = nullptr;
        HRESULT hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
        pResource->Release(); // Release our reference to the resource

        if (FAILED(hr) || !pTexture) {
            return; // Not a 2D texture, can't be a depth buffer we want
        }

        // Get texture description to verify it's a depth format
        D3D11_TEXTURE2D_DESC desc;
        pTexture->GetDesc(&desc);

        // Check if this is a depth format
        if (!isDepthFormat(desc.Format)) {
            pTexture->Release();
            return;
        }

        // Check for duplicates - compare actual texture pointers
        bool isDuplicate = false;
        for (size_t i = 0; i < _detectedDepthTextures.size(); i++) {
            if (_detectedDepthTextures[i] == pTexture) {
                // We already have this texture, but we might want to update the view
                // Option 1: Keep using the first view we found (do nothing)
                // Option 2: Update to use the newest view (implemented here)

                // Release the old view and replace with the new one
                _detectedDepthStencilViews[i]->Release();
                pDSV->AddRef(); // Add reference for our storage
                _detectedDepthStencilViews[i] = pDSV;

                isDuplicate = true;
                break;
            }
        }

        // If this is a new unique texture, add it to our collections
        if (!isDuplicate) {
            // Add references for storage
            pDSV->AddRef();
            pTexture->AddRef();

            // Store in our collections
            _detectedDepthStencilViews.push_back(pDSV);
            _detectedDepthTextures.push_back(pTexture);
            _depthTextureDescs.push_back(desc);

            // If this is our first DSV, set it as current
            if (_detectedDepthStencilViews.size() == 1) {
                _currentDepthBufferIndex = 0;
                _useDetectedDepthBuffer = true;
            }

            MessageHandler::logDebug("D3DHook::captureActiveDepthBuffer: Found unique depth buffer: %dx%d, format: %d",
                desc.Width, desc.Height, desc.Format);
        }

        // Release our working reference to the texture
        pTexture->Release();
		pResource->Release(); // Release the resource reference
    }

    void D3DHook::selectAppropriateDepthBuffer() {
        if (_detectedDepthStencilViews.empty()) {
            return;
        }



        // If we already have a valid depth buffer selected, keep using it
        if (_depthMatchFound == true) {
	        if (D3DLOGGING && VERBOSE)
	        {
                static int framecount = 0;
                framecount = (framecount + 1) % 60; // Limit to once per second

	        	if (framecount == 0) {
                    MessageHandler::logDebug("D3DHook::selectAppropriateDepthBuffer: Already have a valid depth buffer selected, skipping selection");
                }
	        }

			return;
        }

        //MessageHandler::logDebug("D3DHook::selectAppropriateDepthBuffer: Selecting appropriate depth buffer...");

        // Get current viewport or backbuffer dimensions
        UINT viewportWidth = _windowWidth;
        UINT viewportHeight = _windowHeight;

        if (viewportWidth == 0 || viewportHeight == 0) {
            // Fallback to getting dimensions from the RTV
            if (_pLastRTV) {
                ID3D11Resource* pResource = nullptr;
                _pLastRTV->GetResource(&pResource);

                if (pResource) {
                    ID3D11Texture2D* pTexture = nullptr;
                    if (SUCCEEDED(pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture))) {
                        D3D11_TEXTURE2D_DESC desc;
                        pTexture->GetDesc(&desc);
                        viewportWidth = desc.Width;
                        viewportHeight = desc.Height;
                        pTexture->Release();
                    }
                    pResource->Release();
                }
            }
        }

        // Game-specific depth buffer selection
		// ------------------------------------------------------------------------
        const char* exeName = Utils::getExecutableName();

        // Look for game-specific format (using string literal for comparison since our keys and exeName are both lowercase)
        const auto it = _gameDepthFormats.find(exeName);
        if (it != _gameDepthFormats.end()) {
            const int preferredFormat = it->second.format;
            const bool shouldMatchViewport = it->second.matchViewportSize;

            for (size_t i = 0; i < _depthTextureDescs.size(); i++) {
                const auto& desc = _depthTextureDescs[i];

                // Skip invalid buffers
                if (desc.Width == 0 || desc.Height == 0) continue;

                // Match format and optionally size
                if (desc.Format == preferredFormat) {
                    if (!shouldMatchViewport ||
                        (desc.Width == viewportWidth && desc.Height == viewportHeight)) {

                        _currentDepthBufferIndex = static_cast<int>(i);
                        _useDetectedDepthBuffer = true;
                        _depthMatchFound = true;

                        MessageHandler::logDebug("Game-specific match: Selected depth buffer for %s: %dx%d format %d",
                            exeName, desc.Width, desc.Height, desc.Format);
                        return;
                    }
                }
            }

            MessageHandler::logDebug("Game-specific format %d defined for %s, but no matching buffer found",
                preferredFormat, exeName);
        }


        // Step 1: Try exact size + preferred format
        for (const auto format : PREFERRED_DEPTH_FORMATS)
        {
	        // If it's a typeless format, get the corresponding view format
            const DXGI_FORMAT viewFormat = getDepthViewFormat(format);

            for (size_t j = 0; j < _depthTextureDescs.size(); j++) {
                const auto& desc = _depthTextureDescs[j];

                // Skip if dimensions are zero (invalid buffer)
                if (desc.Width == 0 || desc.Height == 0) continue;

                // Get proper view format for this texture
                const DXGI_FORMAT bufferViewFormat = getDepthViewFormat(desc.Format);

                // Skip non-depth formats
                if (!isDepthFormat(desc.Format) || bufferViewFormat == DXGI_FORMAT_UNKNOWN) continue;

                if (desc.Width == viewportWidth && desc.Height == viewportHeight) {
                    // Check for format match (either direct or through view format)
                    if (desc.Format == format || bufferViewFormat == viewFormat) {
                        _currentDepthBufferIndex = static_cast<int>(j);

                        MessageHandler::logDebug("D3DHook::selectAppropriateDepthBuffer: Selected depth buffer %d: %dx%d format %d (Exact match)",
                            _currentDepthBufferIndex, desc.Width, desc.Height, desc.Format);

                        _depthMatchFound = true;
                        return;
                    }
                }
            }
        }

        // Step 2: Try aspect ratio match
        if (viewportWidth > 0 && viewportHeight > 0) {
            const float viewportAspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
            float bestAspectDiff = 100.0f;
            int bestIndex = -1;

            for (size_t i = 0; i < _depthTextureDescs.size(); i++) {
                const auto& desc = _depthTextureDescs[i];

                // Skip if dimensions are zero
                if (desc.Width == 0 || desc.Height == 0) continue;

                const float bufferAspect = static_cast<float>(desc.Width) / static_cast<float>(desc.Height);
                const float aspectDiff = std::abs(bufferAspect - viewportAspect);

                if (aspectDiff < bestAspectDiff) {
                    bestAspectDiff = aspectDiff;
                    bestIndex = static_cast<int>(i);
                }
            }

            if (bestIndex >= 0) {
                _currentDepthBufferIndex = bestIndex;

                const auto& desc = _depthTextureDescs[bestIndex];
                MessageHandler::logDebug("D3DHook::selectAppropriateDepthBuffer: Selected depth buffer %d: %dx%d format %d (Aspect ratio match)",
                    _currentDepthBufferIndex, desc.Width, desc.Height, desc.Format);
                _depthMatchFound = true;
                return;
            }
        }

        // Step 3: Fallback to first available buffer
        if (!_detectedDepthStencilViews.empty()) {
            _currentDepthBufferIndex = 0;

            const auto& desc = _depthTextureDescs[0];
            MessageHandler::logDebug("D3DHook::selectAppropriateDepthBuffer: Selected depth buffer 0: %dx%d format %d (Fallback)",
                desc.Width, desc.Height, desc.Format);
            _depthMatchFound = true;
        }
    }


    bool D3DHook::isDepthFormat(const DXGI_FORMAT format) {
        switch (format) {
            // Standard depth-stencil formats
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:

            // Typeless formats that can be used as depth
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R32G8X24_TYPELESS:

            // Special formats sometimes used with depth
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
            return true;
        default:
            return false;
        }
    }

    DXGI_FORMAT D3DHook::getDepthViewFormat(const DXGI_FORMAT resourceFormat) {
        switch (resourceFormat) {
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_D16_UNORM;
        case DXGI_FORMAT_R24G8_TYPELESS:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_D32_FLOAT;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

            // Special formats - may need special handling
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return DXGI_FORMAT_D24_UNORM_S8_UINT; // Best match
        case DXGI_FORMAT_R16G16_TYPELESS:
            return DXGI_FORMAT_D32_FLOAT; // Closest precision
        case DXGI_FORMAT_R16G16_FLOAT:
            return DXGI_FORMAT_D32_FLOAT; // Closest precision

            // Already depth formats
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return resourceFormat;

            // Resource formats can sometimes be used directly
        case DXGI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_D16_UNORM;
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    //==================================================================================================
    // Sphere Rendering
    //==================================================================================================
    void D3DHook::createSolidColorSphere(float radius, const XMFLOAT4& color) {
        // Clean up existing sphere resources
        if (_sphereVertexBuffer) { _sphereVertexBuffer->Release(); _sphereVertexBuffer = nullptr; }
        if (_sphereIndexBuffer) { _sphereIndexBuffer->Release(); _sphereIndexBuffer = nullptr; }

        // Create a simple sphere using stacks and slices
        constexpr int stacks = 10;
        constexpr int slices = 10;

        std::vector<SimpleVertex> vertices;
        std::vector<UINT> indices;

        // Add top vertex
        SimpleVertex topVertex;
        topVertex.position = XMFLOAT3(0.0f, radius, 0.0f);
        topVertex.color = color;  // Use the provided solid color
        vertices.push_back(topVertex);

        // Add middle vertices
        for (int i = 1; i < stacks; i++) {
            float phi;
            phi = static_cast<float>(i) * XM_PI / stacks;
            for (int j = 0; j < slices; j++) {
                float theta = XM_2PI * static_cast<float>(j) / slices;

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
        bottomVertex.position = XMFLOAT3(0.0f, -radius, 0.0f);
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
        int bottomIndex = static_cast<int>(vertices.size()) - 1;
        int lastRingStart = bottomIndex - slices;
        for (int i = 0; i < slices; i++) {
            indices.push_back(bottomIndex);
            indices.push_back(lastRingStart + i);
            indices.push_back(lastRingStart + (i + 1) % slices);
        }

        // Create vertex buffer
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(SimpleVertex) * vertices.size());
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
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indices.data();

        hr = _pLastDevice->CreateBuffer(&ibDesc, &ibData, &_sphereIndexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createSolidColorSphere: Failed to create index buffer");
            return;
        }

        _sphereIndexCount = static_cast<UINT>(indices.size());
        MessageHandler::logDebug("D3DHook::createSolidColorSphere: Created solid color sphere with %d vertices, %d indices",
            vertices.size(), indices.size());
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
        apexVertex.position = XMFLOAT3(0.0f, 0.0f, _arrowHeadLength);  // Tip at positive Z
        apexVertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        arrowHeadVertices.push_back(apexVertex);

        // Create base vertices for the cone at Z=0
        for (int i = 0; i < _arrowSegments; i++) {
            float angle = XM_2PI * static_cast<float>(i) / static_cast<float>(_arrowSegments);
            float x = _arrowHeadRadius * cosf(angle);
            float y = _arrowHeadRadius * sinf(angle);

            SimpleVertex baseVertex{};
            baseVertex.position = XMFLOAT3(x, y, 0.0f);
            baseVertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
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
        centerVertex.position = XMFLOAT3(0.0f, 0.0f, 0.0f);
        centerVertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
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
            float angle = XM_2PI * static_cast<float>(i) / static_cast<float>(_arrowSegments);
            float x = _arrowShaftRadius * cosf(angle);
            float y = _arrowShaftRadius * sinf(angle);

            // CHANGE: Back vertex at z=-1 (shaft start)
            SimpleVertex backVertex;
            backVertex.position = XMFLOAT3(x, y, -1.0f);
            backVertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            arrowShaftVertices.push_back(backVertex);

            // CHANGE: Front vertex at z=0 (shaft end/head start)
            SimpleVertex frontVertex;
            frontVertex.position = XMFLOAT3(x, y, 0.0f);
            frontVertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
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

       MessageHandler::logDebug("D3DHook::createArrowGeometry: Successfully created arrow geometry");
    }

    void D3DHook::renderArrow(const XMFLOAT3& position, const XMVECTOR& direction,
        float length, const XMFLOAT4& color) const
    {
        if (!_pLastContext || !_arrowHeadVertexBuffer || !_arrowHeadIndexBuffer ||
            !_arrowShaftVertexBuffer || !_arrowShaftIndexBuffer) {
            return;
        }

        // Normalize direction
        XMVECTOR normalizedDir = XMVector3Normalize(direction);
        XMVECTOR defaultDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // +Z axis

        // Calculate rotation quaternion from default to target direction
        XMVECTOR rotationQuat;

        // Handle special cases for numerical stability
        XMVECTOR dot = XMVector3Dot(normalizedDir, defaultDir);
        float dotValue = XMVectorGetX(dot);

        if (dotValue > 0.9999f) {
            // Nearly same direction - no rotation needed
            rotationQuat = XMQuaternionIdentity();
        }
        else if (dotValue < -0.9999f) {
            // Nearly opposite direction - rotate 180° around Y axis
            rotationQuat = XMQuaternionRotationAxis(
                XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XM_PI);
        }
        else {
            // General case
            XMVECTOR rotAxis = XMVector3Cross(defaultDir, normalizedDir);
            rotAxis = XMVector3Normalize(rotAxis);
            float angle = acosf(dotValue);
            rotationQuat = XMQuaternionRotationAxis(rotAxis, angle);
        }

        // Create rotation matrix
        XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotationQuat);

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
            auto cb = (XMFLOAT4*)colorMappedResource.pData;
            *cb = color;  // Set the color
            _pLastContext->Unmap(_colorBuffer, 0);
        }

        // Bind the color buffer to pixel shader
        _pLastContext->PSSetConstantBuffers(0, 1, &_colorBuffer);

        // In our model, the shaft geometry is defined from z=-1 to z=0
        // and the cone is from z=0 to z=headLength
        // 1. Render shaft - position it exactly at the start point
        {
            // Create world matrix for shaft
            XMMATRIX worldMatrix =
                // First translate along Z by 1 unit to make shaft start at 0 (model is from -1 to 0)
                XMMatrixTranslation(0.0f, 0.0f, 1.0f) *
                // Scale to proper length
                XMMatrixScaling(1.0f, 1.0f, shaftLength) *
                // Apply rotation to align with direction
                rotationMatrix *
                // Translate to starting position
                XMMatrixTranslation(position.x, position.y, position.z);

            // Generate final matrix for vertex shader
            XMMATRIX finalMatrix = XMMatrixTranspose(
                worldMatrix * viewMatrix * projMatrix);

            // Update constant buffer
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                auto cb = (SimpleConstantBuffer*)mappedResource.pData;
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
            XMVECTOR headPosition = XMVectorAdd(
                XMLoadFloat3(&position),
                XMVectorScale(normalizedDir, shaftLength)
            );

            // Convert to float3 for transformation
            XMFLOAT3 headPos;
            XMStoreFloat3(&headPos, headPosition);

            // Create world matrix for head
            XMMATRIX worldMatrix =
                // Apply rotation to align with direction
                rotationMatrix *
                // Translate to head position
                XMMatrixTranslation(headPos.x, headPos.y, headPos.z);

            // Generate final matrix for vertex shader
            XMMATRIX finalMatrix = XMMatrixTranspose(
                worldMatrix * viewMatrix * projMatrix);

            // Update constant buffer
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                auto cb = (SimpleConstantBuffer*)mappedResource.pData;
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
        const XMFLOAT3& startPoint, const XMFLOAT3& endPoint,
        const XMFLOAT4& color, float radius, int segments) {
        // Calculate direction vector from start to end
        XMVECTOR start = XMLoadFloat3(&startPoint);
        XMVECTOR end = XMLoadFloat3(&endPoint);
        XMVECTOR direction = XMVectorSubtract(end, start);

        // Skip if points are too close
        if (XMVector3LengthSq(direction).m128_f32[0] < 0.0001f) {
            return;
        }

        direction = XMVector3Normalize(direction);

        // Find perpendicular vectors to create circle around tube
        XMVECTOR right, up;

        // Try to use world up as reference
        XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        float dot = XMVectorGetX(XMVector3Dot(direction, worldUp));

        // If direction is nearly parallel to world up, use a different reference
        if (fabs(dot) > 0.99f) {
            worldUp = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // Calculate perpendicular basis
        right = XMVector3Cross(worldUp, direction);
        right = XMVector3Normalize(right);
        up = XMVector3Cross(direction, right);
        up = XMVector3Normalize(up);

        // Store the base index for connecting triangles
        UINT baseIndex = static_cast<UINT>(tubeVertices.size());

        // Create circle vertices at start and end points
        for (int i = 0; i <= segments; i++) {
            float angle = static_cast<float>(i) / static_cast<float>(segments) * XM_2PI;
            float cosA = cosf(angle);
            float sinA = sinf(angle);

            // Calculate offset from center line
            XMVECTOR offset = XMVectorAdd(
                XMVectorScale(right, cosA * radius),
                XMVectorScale(up, sinA * radius)
            );

            // Create vertex at start point
            SimpleVertex startVertex;
            XMStoreFloat3(&startVertex.position, XMVectorAdd(start, offset));
            startVertex.color = color;
            tubeVertices.push_back(startVertex);

            // Create vertex at end point
            SimpleVertex endVertex;
            XMStoreFloat3(&endVertex.position, XMVectorAdd(end, offset));
            endVertex.color = color;
            tubeVertices.push_back(endVertex);
        }

        // Create triangles connecting the circles
        for (int i = 0; i < segments; i++) {
            // Each segment has two vertices (start and end)
            int i0 = i * 2 + baseIndex;
            int i1 = i * 2 + baseIndex + 1;
            int i2 = (i + 1) * 2 + baseIndex;
            int i3 = (i + 1) * 2 + baseIndex + 1;

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
        if (!&PathManager::instance()) {
            MessageHandler::logError("D3DHook::createPathVisualization: Failed to get path manager");
            return;
        }

        // Get all the paths
        const std::unordered_map<std::string, CameraPath>& paths = PathManager::instance().getPaths();
        if (paths.empty()) {
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
        const auto nodeColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for nodes
        const auto directionColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // Red for direction indicators

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
        MessageHandler::logDebug("D3DHook::createPathVisualization: Successfully created path visualization resources");
        MessageHandler::logDebug("D3DHook::createPathVisualization: Paths: %zu, Nodes: %u, Tube vertices: %u, Tube indices: %u, Interpolated directions: %u",
            _pathInfos.size(), _nodePositions.size(), _pathTubeVertexCount, _pathTubeIndexCount, _interpolatedDirectionsVertexCount);
    }

    // Helper method to process all camera paths
    void D3DHook::processAllPaths(const std::unordered_map<std::string, CameraPath>& paths,
        std::vector<SimpleVertex>& nodeVertices,
        std::vector<SimpleVertex>& directionIndicatorVertices,
        std::vector<SimpleVertex>& interpolatedDirectionVertices,
        std::vector<SimpleVertex>& tubeMeshVertices,
        std::vector<UINT>& tubeMeshIndices,
        std::vector<TubeInfo>& pathTubeInfos,
        const XMFLOAT4& nodeColor,
        const XMFLOAT4& directionColor) {
        int pathIndex = 0;
        for (const auto& pathPair : paths) {
            const std::string& pathName = pathPair.first;
            const CameraPath& path = pathPair.second;
            //const std::vector<CameraNode>* nodes = path.getNodes();
            const size_t nodeCount = path.GetNodeCount();

            if (nodeCount < 2) // Need at least 2 nodes to form a path
                continue;

            MessageHandler::logDebug("D3DHook::processAllPaths: Processing path '%s' with %zu nodes",
                pathName.c_str(), nodeCount);

            // Generate a unique color for this path based on golden ratio
            const float hue = (pathIndex * 137.5f) / 360.0f; // Golden ratio to distribute colors
            const float saturation = 0.8f;
            const float value = 0.9f;

            // Convert HSV to RGB for path color
            XMFLOAT4 pathColor = convertHSVtoRGB(hue, saturation, value, 0.9f);

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
            const size_t interpDirStart = interpolatedDirectionVertices.size();
            createInterpolatedDirections(interpSamples, interpolatedDirectionVertices,
                pathInfo.nodeStartIndex, static_cast<UINT>(nodeCount));

            // Calculate how many interpolated direction vertices were added for this path
            const size_t interpDirCount = interpolatedDirectionVertices.size() - interpDirStart;

            // Store this path's interpolated directions
            storePathDirections(pathInfo, interpolatedDirectionVertices, interpDirStart, interpDirCount);

            // Create interpolated path points from the samples
            std::vector<XMFLOAT3> interpPoints;
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
    void D3DHook::processPathNodes(const CameraPath& path, const size_t nodeCount,
        std::vector<SimpleVertex>& nodeVertices,
        std::vector<SimpleVertex>& directionIndicatorVertices,
        const XMFLOAT4& nodeColor,
        const XMFLOAT4& directionColor) {
        for (size_t i = 0; i < nodeCount; i++) {
            XMFLOAT3 pos = path.getNodePositionFloat(i);

            // Store node position for rendering spheres
            _nodePositions.push_back(pos);

            // Create vertex for node marker
            SimpleVertex nodeVertex;
            nodeVertex.position = pos;
            nodeVertex.color = nodeColor;
            nodeVertices.push_back(nodeVertex);

            // Create direction indicator
            const XMVECTOR rotation = path.getNodeRotation(i);
            const XMVECTOR forward = XMVector3Rotate(
                XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                rotation // Rotate by the camera's rotation
            );

            // Start and end points for direction line
            SimpleVertex dirStart, dirEnd;
            dirStart.position = pos;
            dirStart.color = directionColor;

            // Calculate end point by extending in the forward direction
            XMFLOAT3 forwardFloat;
            XMStoreFloat3(&forwardFloat, forward);

            dirEnd.position.x = pos.x + forwardFloat.x * _directionLength;
            dirEnd.position.y = pos.y + forwardFloat.y * _directionLength;
            dirEnd.position.z = pos.z + forwardFloat.z * _directionLength;
            dirEnd.color = directionColor;

            directionIndicatorVertices.push_back(dirStart);
            directionIndicatorVertices.push_back(dirEnd);
        }
    }

    // Helper to convert HSV color to RGB
    XMFLOAT4 D3DHook::convertHSVtoRGB(const float hue, const float saturation, const float value, const float alpha) {
        const float chroma = value * saturation;
        const float huePrime = hue * 6.0f;
        const float x = chroma * (1.0f - std::abs(std::fmod(huePrime, 2.0f) - 1.0f));

        float r, g, b;
        if (huePrime < 1.0f) { r = chroma; g = x; b = 0.0f; }
        else if (huePrime < 2.0f) { r = x; g = chroma; b = 0.0f; }
        else if (huePrime < 3.0f) { r = 0.0f; g = chroma; b = x; }
        else if (huePrime < 4.0f) { r = 0.0f; g = x; b = chroma; }
        else if (huePrime < 5.0f) { r = x; g = 0.0f; b = chroma; }
        else { r = chroma; g = 0.0f; b = x; }

        const float m = value - chroma;
        r += m; g += m; b += m;

        return XMFLOAT4(r, g, b, alpha);
    }

    // Helper to generate interpolated samples along a path
    std::vector<D3DHook::InterpSample> D3DHook::generateInterpolatedSamples(const CameraPath& path, size_t nodeCount)
    {
        std::vector<InterpSample> interpSamples;
        const std::vector<CameraNode>* nodes = path.getNodes();
        const std::vector<float>* knotVector = path.getknotVector();
        const std::vector<XMVECTOR>* controlPointsPos = path.getControlPointsPos();
        const std::vector<float>* controlPointsFoV = path.getControlPointsFOV();
        const std::vector<XMVECTOR>* controlPointsRot = path.getControlPointsRot();

        // Generate samples for each path segment
        for (size_t i = 0; i < nodeCount - 1; i++) {
            for (int step = 0; step <= _interpolationSteps; step++) {
                float t = static_cast<float>(step) / static_cast<float>(_interpolationSteps);
                float globalT = static_cast<float>(i) + t;

                XMVECTOR interpPoint;
                XMVECTOR interpRotation;
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
                XMStoreFloat3(&sample.position, interpPoint);
                sample.rotation = interpRotation;
                sample.fov = interpFOV;
                interpSamples.push_back(sample);
            }
        }

        return interpSamples;
    }

    // Helper to create interpolated direction indicators
    void D3DHook::createInterpolatedDirections(
        const std::vector<InterpSample>& interpSamples,
        std::vector<SimpleVertex>& interpolatedDirectionVertices,
        UINT nodeStartIndex,                    // New parameter - start index of nodes for this path
        UINT nodeCount) const                   // New parameter - number of nodes for this path
    {
        // Use a configurable threshold for proximity check
        const float proximityThresholdSq = _nodeProximityThreshold * _nodeProximityThreshold;

        for (size_t i = 0; i < interpSamples.size(); i += _rotationSampleFrequency) {
            // Only create indicators at every Nth sample to avoid cluttering
            const InterpSample& sample = interpSamples[i];

            // Check proximity to nodes for this path
            bool tooCloseToNode = false;
            for (UINT j = 0; j < nodeCount; j++) {
                const XMFLOAT3& nodePos = _nodePositions[nodeStartIndex + j];

                // Calculate squared distance (faster than calculating actual distance)
                float dx = sample.position.x - nodePos.x;
                float dy = sample.position.y - nodePos.y;
                float dz = sample.position.z - nodePos.z;
                float distSquared = dx * dx + dy * dy + dz * dz;

                if (distSquared < proximityThresholdSq) {
                    tooCloseToNode = true;
                    break;
                }
            }

            // Skip this sample if it's too close to a node
            if (tooCloseToNode) {
                continue;
            }

            // Calculate direction indicator length based on FOV
            // Inverse relationship: lower FOV = longer line (more zoomed in)
            const float fovScaleFactor = _defaultFOV / sample.fov;
            const float scaledLength = _baseDirectionLength * fovScaleFactor;

            // Calculate forward vector from rotation quaternion
            const XMVECTOR forward = XMVector3Rotate(
                XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                sample.rotation // Apply rotation
            );

            // Create indicator vertices
            SimpleVertex dirStart, dirEnd;
            dirStart.position = sample.position;
            dirStart.color = XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f); // Orange for path indicators

            // Calculate end point
            XMFLOAT3 forwardFloat;
            XMStoreFloat3(&forwardFloat, forward);

            dirEnd.position.x = sample.position.x + forwardFloat.x * scaledLength;
            dirEnd.position.y = sample.position.y + forwardFloat.y * scaledLength;
            dirEnd.position.z = sample.position.z + forwardFloat.z * scaledLength;
            dirEnd.color = XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f);

            interpolatedDirectionVertices.push_back(dirStart);
            interpolatedDirectionVertices.push_back(dirEnd);
        }
    }

    // Helper to store path direction data
    void D3DHook::storePathDirections(PathInfo& pathInfo,
        const std::vector<SimpleVertex>& interpolatedDirectionVertices,
        const size_t startIndex, const size_t count) {
        pathInfo.pathDirections.clear();
        for (size_t i = startIndex; i < startIndex + count; i += 2) {
            pathInfo.pathDirections.push_back(std::make_pair(
                interpolatedDirectionVertices[i].position,
                interpolatedDirectionVertices[i + 1].position
            ));
        }
    }

    // Helper to create tubes for path visualization
    void D3DHook::createPathTubes(const std::vector<XMFLOAT3>& points,
        std::vector<SimpleVertex>& tubeMeshVertices,
        std::vector<UINT>& tubeMeshIndices,
        const XMFLOAT4& color) const
    {
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

            const HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_nodeVertexBuffer);
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

            const HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_directionIndicatorVertexBuffer);
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

            const HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_interpolatedDirectionsVertexBuffer);
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
        static uint32_t frameCounter = 0;
        frameCounter = (frameCounter + 1) % 60;

        // Early exits for when we can't render
        if (!_pLastContext || !_visualizationEnabled) {
            return;
        }

        if (_isChangingMode.load(std::memory_order_acquire)) {
            return;
        }

        if (!_pLastDevice || !_pLastRTV) {
            return;
        }

        if (PathManager::instance().getPathCount() == 0) {
            return;
        }

        // Update path resources if needed
        if (_resourcesNeedUpdate.load(std::memory_order_acquire) || !_pathResourcesCreated) {
            std::lock_guard<std::mutex> lock(_resourceMutex);
            if (_resourcesNeedUpdate.load(std::memory_order_acquire) || !_pathResourcesCreated) {
				MessageHandler::logDebug("D3DHook::renderPaths: Updating path resources");
            	createPathVisualization();
                _resourcesNeedUpdate.store(false, std::memory_order_release);

                if (!_pathResourcesCreated) {
					MessageHandler::logError("D3DHook::renderPaths: Failed to create path resources");
                    return; // Creation failed, exit
                }
            }
        }

        // Create state guard to automatically restore state on exit
        StateGuard stateGuard(_pLastContext);

        // Get a reliable viewport for rendering
        const D3D11_VIEWPORT fullViewport = getFullViewport();

        // Log viewport state
        if (D3DLOGGING && VERBOSE)
        {
            if (frameCounter == 0) {
                UINT numViewports = 1;
                D3D11_VIEWPORT currentViewport;
                _pLastContext->RSGetViewports(&numViewports, &currentViewport);

                MessageHandler::logDebug("D3DHook::renderPaths: Original viewport - width: %.1f, height: %.1f, numViewports: %u",
                    (numViewports > 0) ? currentViewport.Width : 0.0f,
                    (numViewports > 0) ? currentViewport.Height : 0.0f,
                    numViewports);

                MessageHandler::logDebug("D3DHook::renderPaths: Using viewport - width: %.1f, height: %.1f",
                    fullViewport.Width, fullViewport.Height);
            }
        }

        // Set up depth buffer
        ID3D11DepthStencilView* pDepthView = setupDepthBuffer();

        //// Clear the depth buffer before rendering path elements
        //if (pDepthView) {
        //    _pLastContext->ClearDepthStencilView(pDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        //}

        // Prepare rendering pipeline
        if (!prepareForRendering(_pLastContext, _pLastRTV, pDepthView, fullViewport)) {
            MessageHandler::logError("D3DHook::renderPaths: Failed to prepare rendering pipeline");
            return;
        }

        // Get camera matrices for rendering
        XMMATRIX viewMatrix, projMatrix;
        setupCameraMatrices(fullViewport, viewMatrix, projMatrix);

        // Render all path components in correct order
        renderPathTubes(viewMatrix, projMatrix);
        renderDirectionArrows(viewMatrix, projMatrix);
        renderNodeSpheres(viewMatrix, projMatrix);
        if (Globals::instance().settings().pathLookAtEnabled)
        {
            renderPathLookAtTarget(viewMatrix, projMatrix);
        }
        

        // State guard will automatically restore original state on destruction
    }


    ID3D11DepthStencilView* D3DHook::setupDepthBuffer() {
        // Return null if depth buffer is disabled by user
        if (!_useDetectedDepthBuffer) {
            return nullptr;
        }

        // Check if we have any depth buffers
        if (_detectedDepthStencilViews.empty()) {
            return nullptr;
        }

        // Safety check for index bounds
        if (_currentDepthBufferIndex < 0 ||
            _currentDepthBufferIndex >= static_cast<int>(_detectedDepthStencilViews.size())) {
            _currentDepthBufferIndex = 0;
        }

        // Return the selected depth buffer
        ID3D11DepthStencilView* pDepthView = _detectedDepthStencilViews[_currentDepthBufferIndex];

        // Log occasionally
        static int frameCounter = 0;
        frameCounter = (frameCounter + 1) % 60;
        if (frameCounter == 0 && pDepthView) {
            ID3D11Resource* pResource = nullptr;
            pDepthView->GetResource(&pResource);

            if (pResource) {
                ID3D11Texture2D* pTexture = nullptr;
                if (SUCCEEDED(pResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pTexture)))) {
                    D3D11_TEXTURE2D_DESC desc;
                    pTexture->GetDesc(&desc);

                    if (D3DLOGGING && VERBOSE)
                    {
                        MessageHandler::logDebug("D3DHook::setupDepthBuffer: Using depth buffer %d of %zu, dimensions: %dx%d, format: %d",
                            _currentDepthBufferIndex + 1, _detectedDepthStencilViews.size(),
                            desc.Width, desc.Height, desc.Format);
                    }


                    pTexture->Release();
                }
                pResource->Release();
            }
        }

        return pDepthView;
    }

    void D3DHook::setupCameraMatrices(const D3D11_VIEWPORT& viewport,
        XMMATRIX& viewMatrix,
        XMMATRIX& projMatrix) {

        // Get camera position and quaternion from game memory
        auto activeCamAddress = GameSpecific::CameraManipulator::getCameraStructAddress();

        const auto camPos = GameSpecific::CameraManipulator::getCurrentCameraCoords();
        const auto camQuat = reinterpret_cast<XMFLOAT4*>(activeCamAddress + QUATERNION_IN_STRUCT_OFFSET);

        // Load vectors
        XMVECTOR quatVec = XMLoadFloat4(camQuat);
        XMVECTOR posVec = XMLoadFloat3(&camPos);

        // Build view matrix from transformed basis vectors
        // Start with standard basis vectors
        XMVECTOR rightAxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR upAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR forwardAxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        // Apply the game's quaternion to get the transformed basis
        XMVECTOR gameRight = XMVector3Rotate(rightAxis, quatVec);
        XMVECTOR gameUp = XMVector3Rotate(upAxis, quatVec);
        XMVECTOR gameForward = XMVector3Rotate(forwardAxis, quatVec);

        // Apply coordinate system corrections
        // Invert axes based on the game's coordinate system 
        if (kRightSign < 0) gameRight = XMVectorNegate(gameRight);
        if (kUpSign < 0) gameUp = XMVectorNegate(gameUp);
        if (kForwardSign < 0) gameForward = XMVectorNegate(gameForward);

        // Build view matrix directly with corrected basis vectors
        // Remember: A view matrix is the inverse of the world matrix

        // Construct rotation part of view matrix (transpose of world rotation)
        XMMATRIX viewRotation;
        viewRotation.r[0] = XMVector3Normalize(gameRight);
        viewRotation.r[1] = XMVector3Normalize(gameUp);
        viewRotation.r[2] = XMVector3Normalize(gameForward);
        viewRotation.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        // Transpose to get the inverse rotation
        viewRotation = XMMatrixTranspose(viewRotation);

        // Calculate the translation component of the view matrix
        // This is -Rt * P where R is the rotation matrix and P is the position
        XMVECTOR negTranslation = XMVector3Transform(
            XMVectorNegate(posVec), viewRotation);

        // Assemble the final view matrix
        viewMatrix = viewRotation;
        viewMatrix.r[3] = XMVectorSetW(negTranslation, 1.0f);

        // Create projection matrix using the game's FOV
        float fov = GameSpecific::CameraManipulator::getCurrentFoV();
        float nearZ = GameSpecific::CameraManipulator::getNearZ();
		float farZ = GameSpecific::CameraManipulator::getFarZ();
		nearZ = max(nearZ, 0.01f); // Avoid zero division
		farZ = max(farZ, 0.1f); // Avoid zero division
        fov = max(0.01f, min(fov, 3.0f));
        float aspectRatio = viewport.Width / viewport.Height;
        projMatrix = XMMatrixPerspectiveFovLH(fov, aspectRatio, (nearZ*2.0f), 10000.0f);

        // Log matrix components for debugging
        if (D3DLOGGING && VERBOSE)
        {
            static int frameCounter = 0;
            frameCounter = (frameCounter + 1) % 1200;
            if (frameCounter == 0) {
                XMFLOAT3 rightF, upF, forwardF, transF;
                XMStoreFloat3(&rightF, viewMatrix.r[0]);
                XMStoreFloat3(&upF, viewMatrix.r[1]);
                XMStoreFloat3(&forwardF, viewMatrix.r[2]);
                XMStoreFloat3(&transF, viewMatrix.r[3]);

                MessageHandler::logDebug("View Matrix Basis - Right: (%.2f, %.2f, %.2f), Up: (%.2f, %.2f, %.2f), Forward: (%.2f, %.2f, %.2f), Trans: (%.2f, %.2f, %.2f)",
                    rightF.x, rightF.y, rightF.z,
                    upF.x, upF.y, upF.z,
                    forwardF.x, forwardF.y, forwardF.z,
                    transF.x, transF.y, transF.z);

                MessageHandler::logDebug("D3DHook::setupCameraMatrices: FOV: %.2f, NearZ: %.2f, FarZ: %.2f",
                    fov, nearZ, farZ);


                // GET CURRENT VIEWPORT DEPTH RANGE FROM CONTEXT
                if (instance()._pLastContext) {
                    UINT numViewports = 1;
                    D3D11_VIEWPORT currentViewport;
                    instance()._pLastContext->RSGetViewports(&numViewports, &currentViewport);

                    if (numViewports > 0) {
                        MessageHandler::logDebug("Game Viewport Depth Range - MinDepth: %.6f, MaxDepth: %.6f",
                            currentViewport.MinDepth, currentViewport.MaxDepth);
                        MessageHandler::logDebug("Game Viewport Dimensions - Width: %.1f, Height: %.1f",
                            currentViewport.Width, currentViewport.Height);

                        // Compare with the viewport you're using
                        MessageHandler::logDebug("Your Viewport Depth Range - MinDepth: %.6f, MaxDepth: %.6f",
                            viewport.MinDepth, viewport.MaxDepth);
                    }
                    else {
                        MessageHandler::logDebug("No active viewports found");
                    }
                }
            }
        }
    }

    // Helper to render tube paths
    void D3DHook::renderPathTubes(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix) const
    {
        if (!_pathTubeVertexBuffer || !_pathTubeIndexBuffer || _pathTubeIndexCount == 0) {
            return;
        }

        // Update constant buffer
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            const auto cb = (SimpleConstantBuffer*)mappedResource.pData;

            const XMMATRIX worldMatrix = XMMatrixIdentity();
            cb->worldViewProjection = XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

            _pLastContext->Unmap(_constantBuffer, 0);
        }

        _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

        // Set up for indexed drawing
        constexpr UINT stride = sizeof(SimpleVertex);
        constexpr UINT offset = 0;
        _pLastContext->IASetVertexBuffers(0, 1, &_pathTubeVertexBuffer, &stride, &offset);
        _pLastContext->IASetIndexBuffer(_pathTubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Draw each path's tube segment
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                continue;
            }

            if (pathInfo.interpVertexCount > 0) {
                _pLastContext->DrawIndexed(pathInfo.interpVertexCount, pathInfo.interpStartIndex, 0);
            }
        }
    }

    // Helper to render direction arrows
    void D3DHook::renderDirectionArrows(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix) {
        // Get all paths for arrow rendering
        const std::unordered_map<std::string, CameraPath>& paths = PathManager::instance().getPaths();

        // Render arrows for interpolated direction indicators
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                continue;
            }

            // Render this path's direction arrows
            for (const auto& dirPair : pathInfo.pathDirections) {
                // First value is the position
                const XMFLOAT3& position = dirPair.first;

                // Second value is the end position (not a direction)
                // Calculate direction vector from position to end position
                XMVECTOR direction = XMVectorSubtract(
                    XMLoadFloat3(&dirPair.second),
                    XMLoadFloat3(&position)
                );

                // Calculate length
                const float length = XMVectorGetX(XMVector3Length(direction));

                // Use orange color for interpolated path indicators
                auto arrowColor = XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f);

                // Render arrow
                renderArrow(position, direction, length, arrowColor);
            }
        }

        // Render arrows for node direction indicators
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                continue;
            }

            // For each node in this path, render an arrow instead of a line
            for (UINT i = 0; i < pathInfo.nodeCount; i++) {
                const UINT nodeIndex = pathInfo.nodeStartIndex + i;
                if (nodeIndex >= _nodePositions.size()) {
                    continue;
                }

                // Find the path by name to get rotation data
                auto pathIter = paths.find(pathInfo.name);
                if (pathIter != paths.end()) {
                    const CameraPath& path = pathIter->second;

                    // Get position and direction data
                    XMFLOAT3 nodePos = _nodePositions[nodeIndex];
                    const XMVECTOR rotation = path.getNodeRotation(i);
                    XMVECTOR direction = XMVector3Rotate(
                        XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                        rotation // Rotate by the camera's rotation
                    );

                    // Get FOV for this node and calculate scaled length
                    const float nodeFov = path.getNodeFOV(i);
                    // Inverse relationship: lower FOV = longer line (more zoomed in)
                    const float fovScaleFactor = _defaultFOV / nodeFov;
                    const float scaledLength = _baseDirectionLength * fovScaleFactor;

                    // Red color for node direction indicators
                    auto arrowColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

                    // Render a 3D arrow instead of a line
                    renderArrow(nodePos, direction, scaledLength, arrowColor);
                }
            }
        }
    }

    // Helper to render node spheres
    void D3DHook::renderNodeSpheres(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix) const
    {
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                continue;
            }

            if (_sphereVertexBuffer && _sphereIndexBuffer && pathInfo.nodeCount > 0) {
                // For each node in this path, render a sphere at its position
                for (UINT i = 0; i < pathInfo.nodeCount; i++) {
                    const UINT nodeIndex = pathInfo.nodeStartIndex + i;
                    if (nodeIndex >= _nodePositions.size()) {
                        continue;
                    }

                    // Get the node position
                    const XMVECTOR nodePosition = XMLoadFloat3(&_nodePositions[nodeIndex]);

                    // Update constant buffer for this node's sphere
                    D3D11_MAPPED_SUBRESOURCE mappedResource;
                    if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
                        const auto cb = (SimpleConstantBuffer*)mappedResource.pData;

                        // Create world matrix for the sphere at this node position
                        XMMATRIX worldMatrix = XMMatrixScaling(_nodeSize, _nodeSize, _nodeSize) *
                            XMMatrixTranslationFromVector(nodePosition);

                        // Combine matrices
                        cb->worldViewProjection = XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

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

    bool D3DHook::prepareForRendering(ID3D11DeviceContext* context,
        ID3D11RenderTargetView* rtv,
        ID3D11DepthStencilView* dsv,
        const D3D11_VIEWPORT& viewport) const
    {
        if (!context || !rtv) {
            return false;
        }

        // Set our render target with the depth buffer
        context->OMSetRenderTargets(1, &rtv, dsv);

        // Set the viewport
        context->RSSetViewports(1, &viewport);

        // Set up blend state
        ID3D11BlendState* pBlendState = instance().getOrCreateBlendState();
        if (pBlendState) {
            context->OMSetBlendState(pBlendState, nullptr, 0xffffffff);
            pBlendState->Release();
        }

        // Set up rasterizer state
        ID3D11RasterizerState* pRasterizerState = getOrCreateRasterizerState();
        if (pRasterizerState) {
            context->RSSetState(pRasterizerState);
            pRasterizerState->Release();
        }

        // Set up depth stencil state
        ID3D11DepthStencilState* pDepthState = createDepthStateForRendering(dsv != nullptr);
        if (pDepthState) {
            context->OMSetDepthStencilState(pDepthState, 0);
            pDepthState->Release();
        }

        // Set up pipeline
        context->IASetInputLayout(_simpleInputLayout);
        context->VSSetShader(_simpleVertexShader, nullptr, 0);
        context->PSSetShader(_simplePixelShader, nullptr, 0);

        return true;
    }

    ID3D11BlendState* D3DHook::getOrCreateBlendState() 
    {
        if (_pBlendState) {
            _pBlendState->AddRef();
            return _pBlendState;
        }

        if (!_pLastDevice) {
            return nullptr;
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

        const HRESULT hr = _pLastDevice->CreateBlendState(&blendDesc, &_pBlendState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::getOrCreateBlendState: Failed to create blend state, error: 0x%X", hr);
            return nullptr;
        }

        _pBlendState->AddRef(); // Add reference for the returned copy
        return _pBlendState;
    }

    ID3D11RasterizerState* D3DHook::getOrCreateRasterizerState() const
    {
        if (_pRasterizerState) {
            _pRasterizerState->AddRef();
            return _pRasterizerState;
        }

        if (!_pLastDevice) {
            return nullptr;
        }

        // Create rasterizer state
        D3D11_RASTERIZER_DESC rasterizerDesc;
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;  // No culling
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthBias = 0;
        rasterizerDesc.DepthBiasClamp = 0.0f;
        rasterizerDesc.SlopeScaledDepthBias = 0.0f;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.ScissorEnable = FALSE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;

        ID3D11RasterizerState* pRasterizerState = nullptr;
        const HRESULT hr = _pLastDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::getOrCreateRasterizerState: Failed to create rasterizer state, error: 0x%X", hr);
            return nullptr;
        }

        return pRasterizerState;
    }

    ID3D11DepthStencilState* D3DHook::createDepthStateForRendering(const bool depthEnabled) const
    {
        if (!_pLastDevice) {
            return nullptr;
        }

        // Create depth stencil state
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        if (depthEnabled) {
            // Enable depth testing but don't write
            depthDesc.DepthEnable = TRUE;
            depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // Don't write to depth buffer
            depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Less-equal for better results
        }
        else {
            // Disable depth testing
            depthDesc.DepthEnable = FALSE;
            depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        }

        ID3D11DepthStencilState* pDepthState = nullptr;
        const HRESULT hr = _pLastDevice->CreateDepthStencilState(&depthDesc, &pDepthState);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createDepthStateForRendering: Failed to create depth state, error: 0x%X", hr);
            return nullptr;
        }

        return pDepthState;
    }

    void D3DHook::createLookAtTargetSphere(float radius, const XMFLOAT4& color)
    {
        // Clean up existing look-at target sphere resources
        if (_lookAtTargetVertexBuffer) { _lookAtTargetVertexBuffer->Release(); _lookAtTargetVertexBuffer = nullptr; }
        if (_lookAtTargetIndexBuffer) { _lookAtTargetIndexBuffer->Release(); _lookAtTargetIndexBuffer = nullptr; }

        // Create sphere geometry (similar to existing createSolidColorSphere but for look-at target)
        constexpr int stacks = 8;  // Lower resolution for performance
        constexpr int slices = 8;

        std::vector<SimpleVertex> vertices;
        std::vector<UINT> indices;

        // Add top vertex
        SimpleVertex topVertex;
        topVertex.position = XMFLOAT3(0.0f, radius, 0.0f);
        topVertex.color = color;
        vertices.push_back(topVertex);

        // Add middle vertices
        for (int i = 1; i < stacks; i++) {
            float phi = static_cast<float>(i) * XM_PI / stacks;
            for (int j = 0; j < slices; j++) {
                float theta = static_cast<float>(j) * 2.0f * XM_PI / slices;

                SimpleVertex vertex;
                vertex.position.x = radius * sinf(phi) * cosf(theta);
                vertex.position.y = radius * cosf(phi);
                vertex.position.z = radius * sinf(phi) * sinf(theta);
                vertex.color = color;
                vertices.push_back(vertex);
            }
        }

        // Add bottom vertex
        SimpleVertex bottomVertex;
        bottomVertex.position = XMFLOAT3(0.0f, -radius, 0.0f);
        bottomVertex.color = color;
        vertices.push_back(bottomVertex);

        // Generate indices
        // Top cap
        for (int j = 0; j < slices; j++) {
            indices.push_back(0);
            indices.push_back(1 + j);
            indices.push_back(1 + (j + 1) % slices);
        }

        // Middle sections
        for (int i = 0; i < stacks - 2; i++) {
            for (int j = 0; j < slices; j++) {
                int current = 1 + i * slices + j;
                int next = 1 + i * slices + (j + 1) % slices;
                int below = 1 + (i + 1) * slices + j;
                int belowNext = 1 + (i + 1) * slices + (j + 1) % slices;

                // First triangle
                indices.push_back(current);
                indices.push_back(below);
                indices.push_back(next);

                // Second triangle
                indices.push_back(next);
                indices.push_back(below);
                indices.push_back(belowNext);
            }
        }

        // Bottom cap
        int bottomIndex = static_cast<int>(vertices.size()) - 1;
        int lastRingStart = bottomIndex - slices;
        for (int j = 0; j < slices; j++) {
            indices.push_back(bottomIndex);
            indices.push_back(lastRingStart + (j + 1) % slices);
            indices.push_back(lastRingStart + j);
        }

        _lookAtTargetIndexCount = static_cast<UINT>(indices.size());

        // Create vertex buffer
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(SimpleVertex));
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices.data();

        HRESULT hr = _pLastDevice->CreateBuffer(&vbDesc, &vbData, &_lookAtTargetVertexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createLookAtTargetSphere: Failed to create vertex buffer");
            return;
        }

        // Create index buffer
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(UINT));
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indices.data();

        hr = _pLastDevice->CreateBuffer(&ibDesc, &ibData, &_lookAtTargetIndexBuffer);
        if (FAILED(hr)) {
            MessageHandler::logError("D3DHook::createLookAtTargetSphere: Failed to create index buffer");
            _lookAtTargetVertexBuffer->Release();
            _lookAtTargetVertexBuffer = nullptr;
            return;
        }

        _lookAtTargetSize = radius;
    }

    void D3DHook::renderLookAtTarget(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) const
    {
        // Only render if visualization is enabled and we have valid resources
        if (!_visualizationEnabled || !_lookAtTargetVertexBuffer || !_lookAtTargetIndexBuffer || _lookAtTargetIndexCount == 0)
            return;

        // Only render if look-at is enabled AND we're in target offset mode
        if (!Globals::instance().settings().lookAtEnabled)
            return;

        XMFLOAT3 targetPosition;
        bool hasValidTarget = false;

        // Check if we're in path mode and path look-at is enabled
        auto& pathManager = PathManager::instance();
        if (pathManager._pathManagerState && pathManager.getSelectedPathPtr() != nullptr &&
            pathManager.getSelectedPathPtr()->getPathLookAtEnabled() &&
            pathManager.getSelectedPathPtr()->hasValidPathLookAtTarget())
        {
            // Use stored path look-at target position
            targetPosition = pathManager.getSelectedPathPtr()->getCurrentPathLookAtTargetPosition();
            hasValidTarget = true;
        }
        else if (Camera::instance().getUseTargetOffsetMode() && Camera::instance().hasValidLookAtTarget())
        {
            // Use stored regular camera look-at target position (only in target offset mode)
            targetPosition = Camera::instance().getCurrentLookAtTargetPosition();
            hasValidTarget = true;
        }

        // Only render if we have a valid target position
        if (!hasValidTarget)
            return;

        // Update constant buffer for the look-at target sphere
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            const auto cb = static_cast<SimpleConstantBuffer*>(mappedResource.pData);

            // Create world matrix for the sphere at the target position
            XMVECTOR targetPos = XMLoadFloat3(&targetPosition);
            XMMATRIX worldMatrix = XMMatrixScaling(_lookAtTargetSize, _lookAtTargetSize, _lookAtTargetSize) *
                XMMatrixTranslationFromVector(targetPos);

            // Combine matrices
            cb->worldViewProjection = XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

            _pLastContext->Unmap(_constantBuffer, 0);
        }

        _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

        // Set up the sphere rendering
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        _pLastContext->IASetVertexBuffers(0, 1, &_lookAtTargetVertexBuffer, &stride, &offset);
        _pLastContext->IASetIndexBuffer(_lookAtTargetIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Use the simple pixel shader that uses vertex colors
        _pLastContext->PSSetShader(_simplePixelShader, nullptr, 0);

        // Draw the sphere
        _pLastContext->DrawIndexed(_lookAtTargetIndexCount, 0, 0);
    }

    void D3DHook::renderFreeCameraLookAtTarget()
    {
        // Only render free camera look-at target when:
        // 1. Free camera visualization is enabled
        // 2. Look-at is enabled and in target offset mode
        // 3. NOT in path manager mode (free camera only)
        // 4. We have valid resources and target position

        auto& pathManager = PathManager::instance();
        if (!Camera::instance().getCameraLookAtVisualizationEnabled() ||
            !Globals::instance().settings().lookAtEnabled ||
            !Camera::instance().getUseTargetOffsetMode() ||
            pathManager._pathManagerState ||  // Don't show in path mode
            !Camera::instance().hasValidLookAtTarget() ||
            !_lookAtTargetVertexBuffer || !_lookAtTargetIndexBuffer || _lookAtTargetIndexCount == 0)
            return;

        // Get camera matrices
        D3D11_VIEWPORT fullViewport = getFullViewport();
        XMMATRIX viewMatrix, projMatrix;
        setupCameraMatrices(fullViewport, viewMatrix, projMatrix);

        // Set up depth buffer (same as path rendering)
        ID3D11DepthStencilView* pDepthView = setupDepthBuffer();

        // Setup rendering state
        if (!prepareForRendering(_pLastContext, _pLastRTV, pDepthView, fullViewport)) {
            return;
        }

        XMFLOAT3 targetPosition = Camera::instance().getCurrentLookAtTargetPosition();

        // Update constant buffer for the look-at target sphere
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            const auto cb = static_cast<SimpleConstantBuffer*>(mappedResource.pData);

            // Create world matrix for the sphere at the target position
            XMVECTOR targetPos = XMLoadFloat3(&targetPosition);
            XMMATRIX worldMatrix = XMMatrixScaling(_lookAtTargetSize, _lookAtTargetSize, _lookAtTargetSize) *
                XMMatrixTranslationFromVector(targetPos);

            // Combine matrices
            cb->worldViewProjection = XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

            _pLastContext->Unmap(_constantBuffer, 0);
        }

        _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

        // Set up the sphere rendering
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        _pLastContext->IASetVertexBuffers(0, 1, &_lookAtTargetVertexBuffer, &stride, &offset);
        _pLastContext->IASetIndexBuffer(_lookAtTargetIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Use the simple pixel shader that uses vertex colors
        _pLastContext->PSSetShader(_simplePixelShader, nullptr, 0);

        // Draw the sphere
        _pLastContext->DrawIndexed(_lookAtTargetIndexCount, 0, 0);
    }

    void D3DHook::renderPathLookAtTarget(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix) const
    {
        // Only render path look-at target when:
        // 1. Path visualization is enabled
        // 2. Path manager is active with a selected path
        // 3. Path look-at is enabled on the current path
        // 4. We have valid resources and target position

        if (!_visualizationEnabled ||
            !_lookAtTargetVertexBuffer || !_lookAtTargetIndexBuffer || _lookAtTargetIndexCount == 0)
            return;

        auto& pathManager = PathManager::instance();
        if (!pathManager._pathManagerState ||
            pathManager.getSelectedPathPtr() == nullptr ||
            !Globals::instance().settings().pathLookAtEnabled ||
            !pathManager.getSelectedPathPtr()->hasValidPathLookAtTarget())
            return;

        XMFLOAT3 targetPosition = pathManager.getSelectedPathPtr()->getCurrentPathLookAtTargetPosition();

        // Update constant buffer for the look-at target sphere
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(_pLastContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            const auto cb = static_cast<SimpleConstantBuffer*>(mappedResource.pData);

            // Create world matrix for the sphere at the target position
            XMVECTOR targetPos = XMLoadFloat3(&targetPosition);
            XMMATRIX worldMatrix = XMMatrixScaling(_lookAtTargetSize, _lookAtTargetSize, _lookAtTargetSize) *
                XMMatrixTranslationFromVector(targetPos);

            // Combine matrices
            cb->worldViewProjection = XMMatrixTranspose(worldMatrix * viewMatrix * projMatrix);

            _pLastContext->Unmap(_constantBuffer, 0);
        }

        _pLastContext->VSSetConstantBuffers(0, 1, &_constantBuffer);

        // Set up the sphere rendering
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        _pLastContext->IASetVertexBuffers(0, 1, &_lookAtTargetVertexBuffer, &stride, &offset);
        _pLastContext->IASetIndexBuffer(_lookAtTargetIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        _pLastContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Use the simple pixel shader that uses vertex colors
        _pLastContext->PSSetShader(_simplePixelShader, nullptr, 0);

        // Draw the sphere
        _pLastContext->DrawIndexed(_lookAtTargetIndexCount, 0, 0);
    }
}