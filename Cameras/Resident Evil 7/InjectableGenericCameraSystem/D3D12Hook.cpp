#include "stdafx.h"
#include "D3D12Hook.h"
#include "Globals.h"
#include "MessageHandler.h"
#include "CameraManipulator.h"
#include "MinHook.h"
#include "PathManager.h"
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <unordered_set>
#include "Bezier.h"
#include "CentripetalCatmullRom.h"
#include "CatmullRom.h"
#include "BSpline.h"
#include "Cubic.h"
#include "PathUtils.h"
#include "RiemannCubic.h"
#include "Utils.h"
#include "Camera.h"

using namespace DirectX;

namespace IGCS {

    //==================================================================================================
    // Static Member Initialization
    //==================================================================================================
    D3D12Hook::Present_t D3D12Hook::_originalPresent = nullptr;
    D3D12Hook::ResizeBuffers_t D3D12Hook::_originalResizeBuffers = nullptr;
    D3D12Hook::CreateSwapChain_t D3D12Hook::_originalCreateSwapChain = nullptr;
    D3D12Hook::ExecuteCommandLists_t D3D12Hook::_originalExecuteCommandLists = nullptr;
    D3D12Hook::OMSetRenderTargets_t D3D12Hook::_originalOMSetRenderTargets = nullptr;
    D3D12Hook::CreateDepthStencilView_t D3D12Hook::_originalCreateDepthStencilView = nullptr;
    D3D12Hook::ClearDepthStencilView_t D3D12Hook::_originalClearDepthStencilView = nullptr;
    void** D3D12Hook::s_command_queue_vtable = nullptr;
    void** D3D12Hook::s_command_list_vtable = nullptr;
    void** D3D12Hook::s_swapchain_vtable = nullptr;
    void** D3D12Hook::s_factory_vtable = nullptr;

    // Thread-local to prevent recursion
    thread_local bool g_inside_d3d12_hook = false;
    thread_local int32_t g_present_depth = 0;

    //==================================================================================================
	// Singleton & Lifecycle
    //==================================================================================================
    D3D12Hook& D3D12Hook::instance() {
        static D3D12Hook instance;
        return instance;
    }

    D3D12Hook::D3D12Hook()
    {
    }

    D3D12Hook::~D3D12Hook() {
        cleanUp();
    }

    //==================================================================================================
	// Core Initialisation
    //==================================================================================================
    bool D3D12Hook::initialise() {
        if (_isInitialized) {
			//Unlikely to be true at this point, but just in case
            MessageHandler::logDebug("D3D12Hook::initialise: Already initialized, skipping.");
			return true;
        }

        MessageHandler::logDebug("D3D12Hook::initialise: Starting D3D12 hook initialization");

        // Create temporary D3D12 objects to get vtable pointers
        IDXGISwapChain1* swap_chain1{ nullptr };
        IDXGISwapChain3* swap_chain{ nullptr };
        ID3D12Device* device{ nullptr };
        IDXGIFactory4* factory{ nullptr };
        ID3D12CommandQueue* command_queue{ nullptr };

        // Create D3D12 device
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create D3D12 device");
            return false;
        }

        // Create command queue
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)))) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create command queue");
            device->Release();
            return false;
        }

        // Create factory
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create DXGI factory");
            command_queue->Release();
            device->Release();
            return false;
        }

        // Create temporary window
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = DefWindowProcA;
        wc.lpszClassName = "D3D12HookTempWindow";
        wc.hInstance = GetModuleHandleA(nullptr);

        if (!RegisterClassExA(&wc)) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to register temporary window class");
            factory->Release();
            command_queue->Release();
            device->Release();
            return false;
        }

        const HWND temp_hwnd = CreateWindowExA(0, wc.lpszClassName, "D3D12Hook Temp", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

        if (!temp_hwnd) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create temporary window");
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            factory->Release();
            command_queue->Release();
            device->Release();
            return false;
        }

        // Setup swap chain description
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1 = {};
        swap_chain_desc1.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swap_chain_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc1.BufferCount = 2;
        swap_chain_desc1.SampleDesc.Count = 1;
        swap_chain_desc1.SampleDesc.Quality = 0;
        swap_chain_desc1.Width = 100;
        swap_chain_desc1.Height = 100;

        // Create swap chain
        if (FAILED(factory->CreateSwapChainForHwnd(command_queue, temp_hwnd, &swap_chain_desc1, nullptr, nullptr, &swap_chain1))) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create temporary swap chain");
            DestroyWindow(temp_hwnd);
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            factory->Release();
            command_queue->Release();
            device->Release();
            return false;
        }

        if (FAILED(swap_chain1->QueryInterface(IID_PPV_ARGS(&swap_chain)))) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to query IDXGISwapChain3");
            swap_chain1->Release();
            DestroyWindow(temp_hwnd);
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            factory->Release();
            command_queue->Release();
            device->Release();
            return false;
        }

        // Store vtable pointers
        s_swapchain_vtable = *reinterpret_cast<void***>(swap_chain);
        s_factory_vtable = *reinterpret_cast<void***>(factory);
        s_command_queue_vtable = *reinterpret_cast<void***>(command_queue);

        // Get function addresses from vtables
        void* presentAddress = s_swapchain_vtable[8];          // Present
        void* resizeBuffersAddress = s_swapchain_vtable[13];   // ResizeBuffers
        void* createSwapChainAddress = s_factory_vtable[15];   // CreateSwapChainForHwnd
		void* executeCommandListsAddress = s_command_queue_vtable[10]; //ExecuteCommandLists

        // Clean up temporary objects
        swap_chain->Release();
        swap_chain1->Release();
        DestroyWindow(temp_hwnd);
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        factory->Release();
        command_queue->Release();
        device->Release();

        // Hook the functions
    	// Hook Present
        MessageHandler::logDebug("D3D12Hook::initialise: Hooking Present function at %p", presentAddress);
        MH_STATUS status = MH_CreateHook(presentAddress, &D3D12Hook::hookedPresent, reinterpret_cast<void**>(&_originalPresent));
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create Present hook, error: %d", status);
            return false;
        }

        status = MH_EnableHook(presentAddress);
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to enable Present hook, error: %d", status);
            return false;
        }


        // Hook ExecuteCommandLists
        MessageHandler::logDebug("D3D12Hook::initialise: Hooking ExecuteCommandLists at %p", executeCommandListsAddress);
        status = MH_CreateHook(executeCommandListsAddress, &D3D12Hook::hookedExecuteCommandLists, reinterpret_cast<void**>(&_originalExecuteCommandLists));
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create ExecuteCommandLists hook, error: %d", status);
        }
        else {
            status = MH_EnableHook(executeCommandListsAddress);
            if (status != MH_OK) {
                MessageHandler::logError("D3D12Hook::initialise: Failed to enable ExecuteCommandLists hook, error: %d", status);
            }
        }

        // Hook ResizeBuffers
        MessageHandler::logDebug("D3D12Hook::initialise: Hooking ResizeBuffers function at %p", resizeBuffersAddress);
        status = MH_CreateHook(resizeBuffersAddress, &D3D12Hook::hookedResizeBuffers, reinterpret_cast<void**>(&_originalResizeBuffers));
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create ResizeBuffers hook, error: %d", status);
            return false;
        }

        status = MH_EnableHook(resizeBuffersAddress);
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to enable ResizeBuffers hook, error: %d", status);
            return false;
        }

        // Hook CreateSwapChainForHwnd
        MessageHandler::logDebug("D3D12Hook::initialise: Hooking CreateSwapChainForHwnd function at %p", createSwapChainAddress);
        status = MH_CreateHook(createSwapChainAddress, &D3D12Hook::hookedCreateSwapChain, reinterpret_cast<void**>(&_originalCreateSwapChain));
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to create CreateSwapChain hook, error: %d", status);
            return false;
        }

        status = MH_EnableHook(createSwapChainAddress);
        if (status != MH_OK) {
            MessageHandler::logError("D3D12Hook::initialise: Failed to enable CreateSwapChain hook, error: %d", status);
            return false;
        }

        MessageHandler::logLine("D3D12 Hook Initialised Successfully");

        _isInitialized = true;
        _hooked = true;
        return true;
    }

    bool D3D12Hook::initialiseRenderingResources() {
        if (!instance()._pDevice) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Device is null");
            return false;
        }

        // Create command allocator and list
        HRESULT hr = instance()._pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&instance()._pCommandAllocator));
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Failed to create command allocator");
            return false;
        }
        MessageHandler::logDebug("D3D12Hook::initialiseRenderingResources: Command allocator created successfully");

        hr = instance()._pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, instance()._pCommandAllocator, nullptr, IID_PPV_ARGS(&instance()._pCommandList));
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Failed to create command list");
            return false;
        }
		MessageHandler::logDebug("D3D12Hook::initialiseRenderingResources: Command list created successfully");

        // Close command list as it's created in recording state
        hr = instance()._pCommandList->Close();
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Failed to close command list after creation");
            return false;
		}
		MessageHandler::logDebug("D3D12Hook::initialiseRenderingResources: Command list closed successfully after creation");

        // Create root signature
        if (!instance().createRootSignature()) {
            return false;
        }

        // Create shaders and pipeline state
        if (!instance().createShadersAndPipelineState()) {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.NumDescriptors = 256;  // Max 256 unique objects (spheres, arrows, etc.) per frame
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        hr = instance()._pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&instance()._pCBVHeap));
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Failed to create CBV heap");
            return false;
        }
        instance()._cbvDescriptorSize = instance()._pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Create fence for synchronization
        hr = instance()._pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&instance()._pFence));
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Failed to create fence");
            return false;
        }

        instance()._fenceValue = 1;
        instance()._fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (instance()._fenceEvent == nullptr) {
            MessageHandler::logError("D3D12Hook::initialiseRenderingResources: Failed to create fence event");
            return false;
        }

        // Create geometry
        // Create look-at target sphere
        instance().createSolidColorSphere(0.5f, XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
        // Create a single node sphere mesh that will be reused for all nodes
        instance().createNodeSphere(1.0f, XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
        // Create arrow geometry for direction indicators
        instance().createArrowGeometry();

        MessageHandler::logDebug("D3D12Hook::initialiseRenderingResources: D3D12 rendering resources initialized successfully");
        return true;
    }

    void D3D12Hook::initialiseFrameContexts(IDXGISwapChain3* pSwapChain) {
        if (!_pDevice || !pSwapChain) return;

        // Get swap chain description
        DXGI_SWAP_CHAIN_DESC desc;
        const HRESULT hr = pSwapChain->GetDesc(&desc);
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::initialiseFrameContexts: Failed to get swap chain description, error: 0x%X", hr);
            return;
        }
        _frameBufferCount = desc.BufferCount;

        // Resize frame contexts
        _frameContexts.resize(_frameBufferCount);
        _perFrameConstantBuffers.resize(_frameBufferCount);

        // Create or reuse RTV heap
        if (!_pRTVHeap) {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = _frameBufferCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            if (FAILED(_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_pRTVHeap)))) {
                MessageHandler::logError("Failed to create RTV heap");
                return;
            }

            _rtvDescriptorSize = _pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _pRTVHeap->GetCPUDescriptorHandleForHeapStart();

        // Initialize each frame's resources
        for (UINT i = 0; i < _frameBufferCount; i++) {
            // Create command allocator
            if (FAILED(_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&_frameContexts[i].commandAllocator)))) {
                MessageHandler::logError("Failed to create command allocator for frame %d", i);
                continue;
            }

            // Get render target
            if (FAILED(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&_frameContexts[i].renderTarget)))) {
                MessageHandler::logError("Failed to get buffer %d", i);
                continue;
            }
            // NO AddRef() here - we're just copying the pointer

            // Create RTV
            _pDevice->CreateRenderTargetView(_frameContexts[i].renderTarget, nullptr, rtvHandle);
            _frameContexts[i].rtvHandle = rtvHandle;

            // Move to next descriptor
            rtvHandle.ptr += _rtvDescriptorSize;

            // Create per-frame constant buffer
            constexpr UINT alignedCBSize = (sizeof(D3D12SimpleConstantBuffer) + 255) & ~255;
            constexpr UINT maxObjectsPerFrame = 256;
            constexpr UINT constantBufferSize = alignedCBSize * maxObjectsPerFrame;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = constantBufferSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            const HRESULT hresult = _pDevice->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&_perFrameConstantBuffers[i].buffer));

            if (SUCCEEDED(hresult)) {
                _perFrameConstantBuffers[i].gpuAddress = _perFrameConstantBuffers[i].buffer->GetGPUVirtualAddress();

                // Keep it mapped for the lifetime
                D3D12_RANGE readRange = { 0, 0 };
                _perFrameConstantBuffers[i].buffer->Map(0, &readRange,
                    reinterpret_cast<void**>(&_perFrameConstantBuffers[i].mappedData));
            }
        }

        // Mark render targets as initialized since we created them here
        _frameContextsInitialized = true;
    }

    void D3D12Hook::queueInitialisation() {
        _needsInitialisation = true;
        MessageHandler::logDebug("D3D12Hook::queueInitialisation: D3D12 initialization has been queued");
    }

    void D3D12Hook::performQueuedInitialisation() {
        if (!_needsInitialisation) {
            return;
        }

        MessageHandler::logDebug("D3D12Hook::performQueuedInitialisation: Performing queued initialization");

        if (initialiseRenderingResources()) {
            _needsInitialisation = false;
            MessageHandler::logDebug("D3D12Hook::performQueuedInitialisation: Initialization complete");
        }
        else {
            MessageHandler::logError("D3D12Hook::performQueuedInitialisation: Failed to initialise rendering resources");
        }
    }

    bool D3D12Hook::isFullyInitialised() const {
        return _isInitialized && !_needsInitialisation;
    }

    //==================================================================================================
    // Hook Management
    //==================================================================================================

    void D3D12Hook::hookDeviceMethods() {
        if (!_pDevice || _deviceMethodsHooked) {
            return;
        }

        // Get device vtable
        void** deviceVtable = *reinterpret_cast<void***>(_pDevice);

        //Hook CreateDepthStencilView (index 22 in ID3D12Device vtable) //or 22
        void* createDSVAddress = deviceVtable[21];

        MessageHandler::logDebug("D3D12Hook::hookDeviceMethods: Hooking CreateDepthStencilView at %p", createDSVAddress);

        MH_STATUS status = MH_CreateHook(createDSVAddress, &D3D12Hook::hookedCreateDepthStencilView,
            reinterpret_cast<void**>(&_originalCreateDepthStencilView));

        if (status == MH_OK) {
            status = MH_EnableHook(createDSVAddress);
            if (status == MH_OK) {
                _deviceMethodsHooked = true;
                MessageHandler::logDebug("D3D12Hook::hookDeviceMethods: Successfully hooked device methods");
            }
        }
    }

    void D3D12Hook::hookCommandListMethods(ID3D12GraphicsCommandList* pCommandList) {
        static unordered_set<ID3D12GraphicsCommandList*> hookedLists;
        static std::mutex hookMutex;

        std::lock_guard lock(hookMutex);

        // Check if we've already hooked this command list
        if (hookedLists.contains(pCommandList)) {
            return;
        }

        //ID3D12GraphicsCommandList1* pCommandList1 = nullptr;
        //if (SUCCEEDED(pCommandList->QueryInterface(IID_PPV_ARGS(&pCommandList1)))) {
        //    MessageHandler::logDebug("Command list supports ID3D12GraphicsCommandList1");
        //    pCommandList1->Release();
        //}

        //ID3D12GraphicsCommandList2* pCommandList2 = nullptr;
        //if (SUCCEEDED(pCommandList->QueryInterface(IID_PPV_ARGS(&pCommandList2)))) {
        //    MessageHandler::logDebug("Command list supports ID3D12GraphicsCommandList2");
        //    pCommandList2->Release();
        //}

        // Get vtable
        void** vtable = *reinterpret_cast<void***>(pCommandList);

        // Only hook once - store the vtable if we haven't
        if (!s_command_list_vtable) {
            s_command_list_vtable = vtable;

            //Hook OMSetRenderTargets (index 46 in ID3D12GraphicsCommandList vtable)
            void* omSetRenderTargetsAddress = vtable[46];

            MessageHandler::logDebug("D3D12Hook::hookCommandListMethods: Hooking OMSetRenderTargets at %p", omSetRenderTargetsAddress);

            MH_STATUS status = MH_CreateHook(omSetRenderTargetsAddress, &D3D12Hook::hookedOMSetRenderTargets,
                reinterpret_cast<void**>(&_originalOMSetRenderTargets));

            if (status == MH_OK) {
                status = MH_EnableHook(omSetRenderTargetsAddress);
                if (status != MH_OK) {
                    MessageHandler::logError("D3D12Hook::hookCommandListMethods: Failed to enable OMSetRenderTargets hook");
                }
            }
            else {
                MessageHandler::logError("D3D12Hook::hookCommandListMethods: Failed to create OMSetRenderTargets hook");
            }

            // Hook ClearDepthStencilView (index 47 in ID3D12GraphicsCommandList vtable) OR 47 OLD=25
            void* clearDepthStencilViewAddress = vtable[47];

            MessageHandler::logDebug("D3D12Hook::hookCommandListMethods: Hooking ClearDepthStencilView at %p", clearDepthStencilViewAddress);

            MH_STATUS statusC = MH_CreateHook(clearDepthStencilViewAddress, &D3D12Hook::hookedClearDepthStencilView,
                reinterpret_cast<void**>(&_originalClearDepthStencilView));

            if (statusC == MH_OK) {
                statusC = MH_EnableHook(clearDepthStencilViewAddress);
                if (statusC != MH_OK) {
                    MessageHandler::logError("D3D12Hook::hookCommandListMethods: Failed to enable ClearDepthStencilView hook");
                }
                else {
                    MessageHandler::logDebug("D3D12Hook::hookCommandListMethods: Successfully hooked ClearDepthStencilView");
                }
            }
            else {
                MessageHandler::logError("D3D12Hook::hookCommandListMethods: Failed to create ClearDepthStencilView hook");
            }

        }

        hookedLists.insert(pCommandList);
    }

    bool D3D12Hook::unhook() {
        if (!_hooked) {
            return true;
        }

        MessageHandler::logDebug("D3D12Hook::unhook: Unhooking D3D12");

        // 1. Unhook swap chain methods
        if (s_swapchain_vtable) {
            if (_originalPresent) {
                MH_DisableHook(s_swapchain_vtable[8]);  // Present
                MessageHandler::logDebug("D3D12Hook::unhook: Unhooked Present");
            }
            if (_originalResizeBuffers) {
                MH_DisableHook(s_swapchain_vtable[13]); // ResizeBuffers
                MessageHandler::logDebug("D3D12Hook::unhook: Unhooked ResizeBuffers");
            }
        }

        // 2. Unhook factory methods
        if (_originalCreateSwapChain && s_factory_vtable) {
            MH_DisableHook(s_factory_vtable[15]); // CreateSwapChainForHwnd
            MessageHandler::logDebug("D3D12Hook::unhook: Unhooked CreateSwapChainForHwnd");
        }

        // 3. Unhook command queue methods
        if (_originalExecuteCommandLists && s_command_queue_vtable) {
            MH_DisableHook(s_command_queue_vtable[10]); // ExecuteCommandLists
            MessageHandler::logDebug("D3D12Hook::unhook: Unhooked ExecuteCommandLists");
        }

        // 4. Unhook command list methods
        if (s_command_list_vtable) {
            if (_originalOMSetRenderTargets) {
                MH_DisableHook(s_command_list_vtable[46]); // OMSetRenderTargets
                MessageHandler::logDebug("D3D12Hook::unhook: Unhooked OMSetRenderTargets");
            }
            if (_originalClearDepthStencilView) {
                MH_DisableHook(s_command_list_vtable[47]); // ClearDepthStencilView
                MessageHandler::logDebug("D3D12Hook::unhook: Unhooked ClearDepthStencilView");
            }
        }

        // 5. Unhook device methods
        if (_originalCreateDepthStencilView && _pDevice) {
            if (void** deviceVtable = *reinterpret_cast<void***>(_pDevice)) {
                MH_DisableHook(deviceVtable[21]); // CreateDepthStencilView
                MessageHandler::logDebug("D3D12Hook::unhook: Unhooked CreateDepthStencilView");
            }
        }

        // Reset all original function pointers
        _originalPresent = nullptr;
        _originalResizeBuffers = nullptr;
        _originalCreateSwapChain = nullptr;
        _originalExecuteCommandLists = nullptr;
        _originalOMSetRenderTargets = nullptr;
        _originalClearDepthStencilView = nullptr;
        _originalCreateDepthStencilView = nullptr;

        // Reset vtable pointers
        s_swapchain_vtable = nullptr;
        s_factory_vtable = nullptr;
        s_command_queue_vtable = nullptr;
        s_command_list_vtable = nullptr;

        // Reset hook state
        _hooked = false;
        _deviceMethodsHooked = false;
        _is_phase_1 = true;

        MessageHandler::logDebug("D3D12Hook::unhook: All hooks disabled successfully");
        return true;
    }

    //==================================================================================================
    // Hooked Functions
    //==================================================================================================
    HRESULT WINAPI D3D12Hook::hookedPresent(IDXGISwapChain3* pSwapChain, const UINT SyncInterval, const UINT Flags) {
        // Prevent recursion
        if (g_inside_d3d12_hook) {
            return _originalPresent ? _originalPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;
        }

        struct RecursionGuard {
            RecursionGuard() {
                g_inside_d3d12_hook = true;
            	g_present_depth++;}
            ~RecursionGuard() {
                g_inside_d3d12_hook = false;
            	g_present_depth--;}
        } guard;

        auto& hook = instance();

        // One-time initialization to capture the primary device from the swap chain.
        if (hook._devicefound == DeviceCaptureState::Pending && pSwapChain)
        {
            // ACQUIRE THE LOCK FIRST to prevent race conditions.
            std::lock_guard lock(hook._resourceMutex);

            // Double-check the condition AFTER acquiring the lock.
            if (hook._devicefound == DeviceCaptureState::Pending && pSwapChain)
            {
                if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&hook._pDevice))))
                {
                    MessageHandler::logDebug("D3D12Hook::hookedPresent: Captured primary device from swap chain.");
                    // Now we have the device, we can safely do other one-time initializations.
                    hook._pSwapChain = pSwapChain;
                    hook.initialiseFrameContexts(pSwapChain);
                    hook.hookDeviceMethods(); // Hook device methods now that we have the device.
                    hook.queueInitialisation();
                    hook._viewPort = hook.getFullViewport();
                    // Signal that the device is ready.
					hook._devicefound = DeviceCaptureState::SuccessFromPresent; // indicates device found successfully
                }
                else
                {
                    MessageHandler::logError("D3D12Hook::hookedPresent: Failed to get device from swap chain, will signal for fallback");
					hook._devicefound = DeviceCaptureState::FailedInPresent; // indicates an attempt was made in hookedPresent but failed, we can try capture from ExecuteCommandLists
                }
            }
        }

        // Handle phase transition
        if (hook._is_phase_1) {
            hook._is_phase_1 = false;
            MessageHandler::logDebug("D3D12Hook::hookedPresent: Transitioned from phase 1 to phase 2");
        }

        // Initialize D3D12 resources if needed
        if (hook._needsInitialisation) {
            std::lock_guard lock(hook._resourceMutex);
            if (hook._needsInitialisation) {  // Double-check after acquiring lock
                hook.performQueuedInitialisation();
            }
        }

        // Render visualizations if enabled and resources are ready
        try {
        	hook.draw(pSwapChain);

        }
        catch (const std::exception& e) {
            MessageHandler::logError("Exception during rendering: %s", e.what());
        }

        // Call original Present
        const HRESULT result = _originalPresent ? _originalPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;

        // Log device removal only once
        if (result == DXGI_ERROR_DEVICE_REMOVED && hook._pDevice) {
            static bool deviceRemovalLogged = false;
            if (!deviceRemovalLogged) {
                const HRESULT reason = hook._pDevice->GetDeviceRemovedReason();
                MessageHandler::logError("D3D12Hook::hookedPresent: Device removed! Reason: 0x%X", reason);
                deviceRemovalLogged = true;
            }
        }

        hook._activeHandlesThisFrame.clear();

        if (Globals::instance().systemActive() && RUN_IN_HOOKED_PRESENT)
        {
            System::instance().updateFrame();
        }

        // Cleanup handled by RAII guard
        return result;
    }

    void WINAPI D3D12Hook::hookedExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, const UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
	{
        auto& hook = instance();

        // One-time capture logic for the command queue.
        if (hook._devicefound == DeviceCaptureState::SuccessFromPresent && hook._pDevice && !hook._pCommandQueue && pCommandQueue)
        {
            ID3D12Device* queueDevice = nullptr;
            if (SUCCEEDED(pCommandQueue->GetDevice(IID_PPV_ARGS(&queueDevice))))
            {
				if (queueDevice == hook._pDevice) // Ensure it's the same device we captured from the swap chain
                {
                    MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists: Device matched.");
                    D3D12_COMMAND_QUEUE_DESC queueDesc = pCommandQueue->GetDesc();

                    if (queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
                        hook._pCommandQueue = pCommandQueue;
                        pCommandQueue->AddRef(); // We are keeping this pointer, so AddRef.
                        MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists: Captured DIRECT command queue.");
                    }
                }
                else
                {
                    MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists: Queue's device does not match our primary device. Skipping capture.");
                }
                queueDevice->Release();
            }
        }
        else if (hook._devicefound == DeviceCaptureState::FailedInPresent && !hook._pDevice && !hook._pCommandQueue && pCommandQueue) //hookedPresent failed to get the device, try here as a fallback
        {
            MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists: Fallback required as hookedPresent device captured failed");
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc = pCommandQueue->GetDesc();

            if (queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
                hook._pCommandQueue = pCommandQueue;
                pCommandQueue->AddRef(); // Important: add reference
                MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists::FALLBACK: Captured DIRECT command queue");
            }

            // NOW GET THE DEVICE FROM THE COMMAND QUEUE
            if (!hook._pDevice) {
                ID3D12Device* device = nullptr;
                if (SUCCEEDED(pCommandQueue->GetDevice(IID_PPV_ARGS(&device)))) {
                    hook._pDevice = device;
                    // Don't AddRef here - GetDevice already does it
                    MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists::FALLBACK: Captured device from command queue");

                    // Hook device methods immediately
					MessageHandler::logDebug("D3D12Hook::hookedExecuteCommandLists::FALLBACK: Hooking device methods for depth buffer capture");
                    hook.hookDeviceMethods();
                }
            }
			hook._devicefound = DeviceCaptureState::SuccessFromFallback; // Mark as success to avoid retrying

        }

        // Intercept and hook every graphics command list passed by the game.
        for (UINT i = 0; i < NumCommandLists; ++i)
        {
            // We only care about graphics command lists, as they do the rendering.
            ID3D12GraphicsCommandList* pGraphicsCommandList = nullptr;
            if (SUCCEEDED(ppCommandLists[i]->QueryInterface(IID_PPV_ARGS(&pGraphicsCommandList))))
            {
                // Lock to safely access our set of hooked lists
                std::lock_guard lock(hook._pCommandListHookMutex);

                // Check if we've already hooked this specific command list instance.
                if (!hook._pHookedCommandLists.contains(pGraphicsCommandList))
                {
                    // It's a new one, so apply our hooks to its vtable.
                    hookCommandListMethods(pGraphicsCommandList);

                    // Add it to our set so we don't hook it again.
                    hook._pHookedCommandLists.insert(pGraphicsCommandList);
                    //MessageHandler::logDebug("D3D12Hook: Hooked new game command list: %p", pGraphicsCommandList);
                }

                // We are done with this temporary pointer.
                pGraphicsCommandList->Release();
            }
        }

        // Call the original function
        if (_originalExecuteCommandLists)
            _originalExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
    }

    HRESULT WINAPI D3D12Hook::hookedResizeBuffers(IDXGISwapChain3* pSwapChain, const UINT BufferCount,
        const UINT Width, const UINT Height,
        const DXGI_FORMAT NewFormat, const UINT SwapChainFlags) {

        auto& hook = instance();

        MessageHandler::logDebug("D3D12Hook::hookedResizeBuffers: Resizing to %ux%u, BufferCount: %u", Width, Height, BufferCount);

        // Reset all D3D12 state and wait for GPU
        hook.resetState();

        // Call original ResizeBuffers
        MessageHandler::logDebug("D3D12Hook::hookedResizeBuffers: Calling original ResizeBuffers");
        const HRESULT result = _originalResizeBuffers ?
            _originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags) : E_FAIL;

        if (FAILED(result)) {
            MessageHandler::logError("D3D12Hook::hookedResizeBuffers: Original ResizeBuffers failed with error 0x%X", result);
            return result;  // Don't try to recreate resources when resize fails!
        }

        // Mark that resources need to be recreated on next render
        MessageHandler::logDebug("D3D12Hook::hookedResizeBuffers: Original succeeded - resetting variables for initialisation");
        hook._resourcesNeedUpdate.store(true, std::memory_order_release);
        hook._needsInitialisation.store(true, std::memory_order_release);

        hook._viewPort = hook.getFullViewport();
        MessageHandler::logDebug("D3D12Hook::hookedResizeBuffers: Viewport updated to %.0f x %.0f", hook._viewPort.Width, hook._viewPort.Height);
        MessageHandler::logDebug("D3D12Hook::hookedResizeBuffers: Resize complete");

        return result;
    }

    HRESULT WINAPI D3D12Hook::hookedCreateSwapChain(IDXGIFactory4* pFactory, IUnknown* pDevice, const HWND hWnd,
        const DXGI_SWAP_CHAIN_DESC* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
        IDXGIOutput* pRestrictToOutput, IDXGISwapChain** ppSwapChain) {

        MessageHandler::logDebug("D3D12Hook::hookedCreateSwapChain: Called");

        // Call original
        const HRESULT result = _originalCreateSwapChain ?
            _originalCreateSwapChain(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain) : E_FAIL;

        if (SUCCEEDED(result) && ppSwapChain && *ppSwapChain) {
            MessageHandler::logDebug("D3D12Hook::hookedCreateSwapChain: New swap chain created");

            // Get D3D12 device from the IUnknown
            auto& hook = instance();
            if (pDevice && !hook._deviceMethodsHooked) {
                ID3D12Device* d3d12Device = nullptr;
                if (SUCCEEDED(pDevice->QueryInterface(IID_PPV_ARGS(&d3d12Device)))) {
                    // Store device if we don't have one
                    if (!hook._pDevice) {
                        hook._pDevice = d3d12Device;
                        hook._pDevice->AddRef();
                    }

                    MessageHandler::logDebug("D3D12Hook::hookedCreateSwapChain: D3D12 device obtained");
                    MessageHandler::logDebug("D3D12Hook::hookedCreateSwapChain: Hooking device methods for depth buffer capture");
                    // Hook device methods for depth buffer capture
                    hook.hookDeviceMethods();

                    d3d12Device->Release();
                }
            }
        }

        return result;
    }

    void WINAPI D3D12Hook::hookedOMSetRenderTargets(ID3D12GraphicsCommandList* pCommandList,
        const UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors,
        const BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor) {

        // Call original
        if (_originalOMSetRenderTargets) 
            _originalOMSetRenderTargets(pCommandList, NumRenderTargetDescriptors, pRenderTargetDescriptors, RTsSingleHandleToDescriptorRange, pDepthStencilDescriptor);
    }

    void WINAPI D3D12Hook::hookedClearDepthStencilView(
        ID3D12GraphicsCommandList* pCommandList,
        const D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
        const D3D12_CLEAR_FLAGS ClearFlags,
        const FLOAT Depth,
        const UINT8 Stencil,
        const UINT NumRects,
        const D3D12_RECT* pRects) {

        auto& hook = instance();
        std::lock_guard lock(hook._depthBufferMutex);

        hook.clearDepthStencilLogging(pCommandList, DepthStencilView, ClearFlags, Depth, Stencil, NumRects, pRects);

        // This handle is being used by the game right now, so it is valid for this frame.
    	hook._activeHandlesThisFrame.insert(DepthStencilView.ptr);

        hook.trackDepthDescriptor(DepthStencilView);

        //Look up resource info from our map ---
        if (!hook._loggedDepthDescriptors.contains(DepthStencilView.ptr)) {
            DepthResourceInfo info = hook.getDepthResourceInfo(DepthStencilView);
            if (info.resource) { // Check if we found it in the map
                MessageHandler::logDebug("ClearDepthStencilView using known resource:");
                MessageHandler::logDebug("  - Descriptor: 0x%llX", DepthStencilView.ptr);
                MessageHandler::logDebug("  - Dimensions: %llu x %u", info.desc.Width, info.desc.Height);
                hook._loggedDepthDescriptors.insert(DepthStencilView.ptr); // Log only once per descriptor
            }
        }

        // Call the original function
		if (_originalClearDepthStencilView)
        _originalClearDepthStencilView(pCommandList, DepthStencilView, ClearFlags, Depth, Stencil, NumRects, pRects);
    }

    void WINAPI D3D12Hook::hookedCreateDepthStencilView(
        ID3D12Device* pDevice,
        ID3D12Resource* pResource,
        const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
        D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {

        //instance().trackDepthDescriptor(DestDescriptor);

        if (pResource) {
            // Lock the mutex to ensure thread-safe access to the map
            std::lock_guard lock(instance()._depthBufferMutex);

            D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();

            // Create the info struct
            DepthResourceInfo info;
            info.resource = pResource;
            info.desc = resourceDesc;

            // Add it to our map
            instance()._depthResourceMap[DestDescriptor.ptr] = info;

            // Log the creation for debugging
            MessageHandler::logDebug("Mapped Depth Resource:");
            MessageHandler::logDebug("  - Descriptor Handle: 0x%llX", DestDescriptor.ptr);
            MessageHandler::logDebug("  - Resource Address: %p", pResource);
            MessageHandler::logDebug("  - Dimensions: %llu x %u", resourceDesc.Width, resourceDesc.Height);
            MessageHandler::logDebug("  - Format: %s", formatToString(resourceDesc.Format));
        }

        // Call the original function
        if (_originalCreateDepthStencilView) {
            _originalCreateDepthStencilView(pDevice, pResource, pDesc, DestDescriptor);
        }
    }

    //==================================================================================================
	// D3D12 Pipeline Setup
	//==================================================================================================
    bool D3D12Hook::createRootSignature() {
        // Define root parameter for constant buffer
        D3D12_ROOT_PARAMETER rootParameter;
        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_DESCRIPTOR_RANGE descriptorRange;
        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descriptorRange.NumDescriptors = 1;
        descriptorRange.BaseShaderRegister = 0;
        descriptorRange.RegisterSpace = 0;
        descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParameter.DescriptorTable.NumDescriptorRanges = 1;
        rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRange;

        // Create root signature
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.NumParameters = 1;
        rootSignatureDesc.pParameters = &rootParameter;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* signature = nullptr;
        ID3DBlob* error = nullptr;

        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr)) {
            if (error) {
                MessageHandler::logError("D3D12Hook::createRootSignature: Failed to serialize root signature: %s",
                    static_cast<char*>(error->GetBufferPointer()));
                error->Release();
            }
            return false;
        }

        hr = _pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&_pRootSignature));

        signature->Release();

        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::createRootSignature: Failed to create root signature");
            return false;
        }

        return true;
    }

    bool D3D12Hook::createShadersAndPipelineState() {
        // Vertex shader
        const std::string vsCode = R"(
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

        // Pixel shader
        const std::string psCode = R"(
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

        // Compile shaders
        _pVertexShader = compileShader(vsCode, "vs_5_0", "main");
        if (!_pVertexShader) {
            return false;
        }

        _pPixelShader = compileShader(psCode, "ps_5_0", "main");
        if (!_pPixelShader) {
            return false;
        }

        // Define input layout
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Create pipeline state
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { .pInputElementDescs = inputElementDescs, .NumElements = ARRAYSIZE(inputElementDescs) };
        psoDesc.pRootSignature = _pRootSignature;
        psoDesc.VS = { .pShaderBytecode = _pVertexShader->GetBufferPointer(), .BytecodeLength = _pVertexShader->GetBufferSize() };
        psoDesc.PS = { .pShaderBytecode = _pPixelShader->GetBufferPointer(), .BytecodeLength = _pPixelShader->GetBufferSize() };

        // Set up rasterizer state
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Set up blend state
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // Set up depth stencil state
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        constexpr D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
        };
        psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
        psoDesc.DepthStencilState.BackFace = defaultStencilOp;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = _pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pPipelineState));
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::createShadersAndPipelineState: Failed to create pipeline state");
            return false;
        }

        // ========== CREATE DEPTH-ENABLED PSOs ==========
        // Configure for depth testing
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // write
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        // Set depth format
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        // Create PSO with normal depth testing (LESS_EQUAL)
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        hr = _pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pPipelineStateWithDepth));
        if (FAILED(hr)) {
            _depthPSOCreationFailed = true;
            MessageHandler::logError("D3D12Hook::createShadersAndPipelineState: Failed to create PSO with depth (LESS_EQUAL)");
            // This is not fatal - we can still render without depth
        }

        // Create PSO with reversed depth testing (GREATER_EQUAL) for Grid 2019
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        hr = _pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pPipelineStateWithReversedDepth));
        if (FAILED(hr)) {
            _depthPSOCreationFailed = true;
            MessageHandler::logError("D3D12Hook::createShadersAndPipelineState: Failed to create PSO with reversed depth (GREATER_EQUAL)");
            // This is not fatal - we can still render without depth
        }

        // Optional: Create PSOs for other depth formats if needed
        // std::map<DXGI_FORMAT, ID3D12PipelineState*> _depthFormatPSOs;

        MessageHandler::logDebug("D3D12Hook::createShadersAndPipelineState: Successfully created all PSOs");
        return true;
    }

    ID3DBlob* D3D12Hook::compileShader(const std::string& shaderCode, const std::string& target, const std::string& entryPoint) {
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        UINT compileFlags = 0;
		#ifdef _DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		#endif

        const HRESULT hr = D3DCompile(shaderCode.c_str(), shaderCode.length(), nullptr, nullptr, nullptr,
            entryPoint.c_str(), target.c_str(), compileFlags, 0, &shaderBlob, &errorBlob);

        if (FAILED(hr)) {
            if (errorBlob) {
                MessageHandler::logError("D3D12Hook::compileShader: Shader compilation failed: %s",
                    static_cast<char*>(errorBlob->GetBufferPointer()));
                errorBlob->Release();
            }
            return nullptr;
        }

        if (errorBlob) {
            errorBlob->Release();
        }

        return shaderBlob;
    }

    ID3D12PipelineState* D3D12Hook::selectPipelineState(const bool useDepth) const
    {
		return useDepth ? _useReversedDepth ? _pPipelineStateWithReversedDepth : _pPipelineStateWithDepth : _pPipelineState;
    }

    //==================================================================================================
	// Rendering Core
	//==================================================================================================
    void D3D12Hook::draw(IDXGISwapChain3* pSwapChain) {
        if (_resourcesNeedUpdate.load(std::memory_order_acquire))
        {
            // We are on the render thread, so it's safe to perform this work now. Calling this from the tools thread
			// creates the potential for timing issues.
            safeInterpolationModeChange();
            // The flag is reset inside createPathVisualisation/safeInterpolationModeChange now
        }

    	if (!isFullyInitialised() || !_pCommandQueue || !_pDevice ||
            (!_visualisationEnabled && !Camera::instance().getCameraLookAtVisualizationEnabled())) {
            return; // Not ready or not enabled, so do nothing.
        }

        try {
            setupCameraMatrices();
        }
        catch (const std::exception& e) {
            MessageHandler::logError("Exception during camera matrix setup: %s", e.what());
            return;
        }

        if (_frameContexts.empty()) {
            MessageHandler::logDebug("D3D12Hook::draw: Frame contexts not initialized - running initialisation now");
            // Try to initialise them now
            initialiseFrameContexts(pSwapChain);
            if (_frameContexts.empty()) {
                MessageHandler::logError("D3D12Hook::draw: Failed to initialise frame contexts");
                return;
            }
        }

        const UINT frameIndex = pSwapChain->GetCurrentBackBufferIndex();
        if (frameIndex >= _frameContexts.size()) {
            return;
        }

        // Set current frame index for constant buffer management
        _currentFrameIndex = frameIndex;

        // Wait for the fence from the PREVIOUS frame's submission to complete.
        // This ensures the GPU is no longer using any resources from that frame,
        // including the command allocator we are about to reset.
        if (_frameContexts[frameIndex].fenceValue != 0) {
            waitForFenceValue(_frameContexts[frameIndex].fenceValue);
        }

        const FrameContext& frameContext = _frameContexts[frameIndex];
        if (!frameContext.commandAllocator || !frameContext.renderTarget) {
            return;
        }

        // DEBUGGING: Log and validate command queue before execution
        if (!_pCommandQueue) {
            MessageHandler::logError("D3D12Hook::draw: Command queue is null before ExecuteCommandLists");
            return;
        }

        // Check if command queue is valid by calling a harmless method
        try {
            D3D12_COMMAND_QUEUE_DESC queueDesc = _pCommandQueue->GetDesc();

            // Verify it's a direct command queue
            if (queueDesc.Type != D3D12_COMMAND_LIST_TYPE_DIRECT) {
                MessageHandler::logError("D3D12Hook::draw: Command queue is not a DIRECT queue (Type=%d)", queueDesc.Type);
            }
        }
        catch (...) {
            MessageHandler::logError("D3D12Hook::draw: Exception when accessing command queue - likely invalid pointer");
            return;
        }

        // Reset allocator
        HRESULT hr = frameContext.commandAllocator->Reset();
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::draw: Failed to reset command allocator: 0x%X", hr);
            return;
        }

        // Reset command list
        hr = _pCommandList->Reset(frameContext.commandAllocator, nullptr);
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::draw: Failed to reset command list: 0x%X", hr);
            return;
        }

        _currentCBVIndex = 0;

        // Transition to RENDER_TARGET
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = frameContext.renderTarget;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        _pCommandList->ResourceBarrier(1, &barrier);

        prepareCommandListForRendering(frameContext);

        // Render path components
    	renderPaths(_viewMatrix, _projMatrix);

        // Render free camera look-at target if enabled
    	renderFreeCameraLookAtTarget();

        // Transition back to PRESENT
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        _pCommandList->ResourceBarrier(1, &barrier);

        // Close and execute
        hr = _pCommandList->Close();
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::draw: Failed to close command list: 0x%X", hr);
            return;
        }
        ID3D12CommandList* ppCommandLists[] = { _pCommandList };
        _pCommandQueue->ExecuteCommandLists(ARRAYSIZE(ppCommandLists), ppCommandLists);

        // Signal and store the fence value for THIS frame's work.
        // The next time we see this frameIndex, we will wait for this value.
        const UINT64 fenceToSignal = _fenceValue++;
        if (SUCCEEDED(_pCommandQueue->Signal(_pFence, fenceToSignal)))
        {
            // Store the fence value in the frame context itself.
            _frameContexts[frameIndex].fenceValue = fenceToSignal;
        }
        else
        {
            MessageHandler::logError("D3D12Hook::draw: Failed to signal fence for frame %u", _currentFrameIndex);
        }
    }

    void D3D12Hook::prepareCommandListForRendering(const FrameContext& frameContext)
    {
        // Get depth descriptor
        const D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor = getCurrentTrackedDepthDescriptor();
        const bool useDepth = _useDetectedDepthBuffer && (depthDescriptor.ptr != 0);
        _currentFrameUsingDepth = useDepth;

        // Set render targets
        if (useDepth && !_depthPSOCreationFailed && _currentDepthDescriptorIndex != -1)
        {
            // Check if the selected handle is in the safe list for this frame.
            if (_activeHandlesThisFrame.contains(depthDescriptor.ptr))
            {
                // It's valid. Use it.
                _pCommandList->OMSetRenderTargets(1, &frameContext.rtvHandle, FALSE, &depthDescriptor);
            }
            else
            {
                // It's NOT in the safe list. The handle is stale.
                MessageHandler::logError("Stale depth descriptor 0x%llX detected. Skipping depth and removing from list.", depthDescriptor.ptr);

                // 1. Do not use the stale handle for rendering.
                _pCommandList->OMSetRenderTargets(1, &frameContext.rtvHandle, FALSE, nullptr);

                // 2. Lock the mutex to safely modify the shared descriptor list.
                std::lock_guard lock(_depthBufferMutex);

                // 3. Remove the stale handle from the master list so we don't cycle to it again.
                const UINT64 staleHandlePtr = depthDescriptor.ptr;
                std::erase_if(_trackedDepthDescriptors, [staleHandlePtr](const auto& handle) {
                    return handle.ptr == staleHandlePtr;
                    });

                // 4. Safely adjust the current index to prevent it from being out of bounds.
                if (_currentDepthDescriptorIndex >= _trackedDepthDescriptors.size()) {
                    _currentDepthDescriptorIndex = _trackedDepthDescriptors.empty() ? -1 : 0;
                }
            }
        }
        else
        {
            _pCommandList->OMSetRenderTargets(1, &frameContext.rtvHandle, 
                FALSE, nullptr);
        }

        // Set viewport and scissor
        const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(_viewPort.Width), static_cast<LONG>(_viewPort.Height) };
        _pCommandList->RSSetViewports(1, &_viewPort);
        _pCommandList->RSSetScissorRects(1, &scissorRect);

        // Set pipeline state and root signature
        _pCommandList->SetGraphicsRootSignature(_pRootSignature);
        ID3D12PipelineState* pso = selectPipelineState(_currentFrameUsingDepth); // false = solid
        _pCommandList->SetPipelineState(pso);
        _pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set descriptor heaps
        ID3D12DescriptorHeap* ppHeaps[] = { _pCBVHeap };
        _pCommandList->SetDescriptorHeaps(ARRAYSIZE(ppHeaps), ppHeaps);
        _pCommandList->SetGraphicsRootDescriptorTable(0, _pCBVHeap->GetGPUDescriptorHandleForHeapStart());
    }

    void D3D12Hook::renderPaths(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix) {
        if (!_visualisationEnabled && Camera::instance().getCameraLookAtVisualizationEnabled())
            return;

        // Check if we have any paths to render, if not, return early
        const auto& paths = PathManager::instance().getPaths();
        if (paths.empty()) {
            return;
        }

        // Update path resources if needed
        if (_resourcesNeedUpdate.load(std::memory_order_acquire) || !_pathResourcesCreated) {
            std::lock_guard lock(_resourceMutex);
            if (_resourcesNeedUpdate.load(std::memory_order_acquire) || !_pathResourcesCreated) {
                MessageHandler::logDebug("D3D12Hook::renderPaths: Updating path resources. Flag status: _resourcesNeedUpdate=%s | _pathResourcesCreated=%s", _resourcesNeedUpdate.load() ? "true" : "false", _pathResourcesCreated ? "true" : "false");
                createPathVisualisation();
                _resourcesNeedUpdate.store(false, std::memory_order_release);

                if (!_pathResourcesCreated) {
                    return;
                }
            }
        }

        // Just do the actual rendering
        renderPathTubes();
        renderDirectionArrows();
        renderNodeSpheres();
    	renderPathLookAtTarget();
    }

    void D3D12Hook::setupCameraMatrices() {

        auto* activeCamAddress = GameSpecific::CameraManipulator::getCameraStructAddress();
        if (!activeCamAddress) {
            _viewMatrix = XMMatrixIdentity();
            _projMatrix = XMMatrixIdentity();
            return;
        }
        const auto camPos = GameSpecific::CameraManipulator::getCurrentCameraCoords();
        const auto camQuat = reinterpret_cast<XMFLOAT4*>(activeCamAddress + QUATERNION_IN_STRUCT_OFFSET);

        const XMVECTOR posVec = XMLoadFloat3(&camPos);
        const XMVECTOR quatVec = XMLoadFloat4(camQuat);

        const XMMATRIX worldMatrix = XMMatrixRotationQuaternion(quatVec) * XMMatrixTranslationFromVector(posVec);

        float fov = GameSpecific::CameraManipulator::getCurrentFoV();
        float nearZ = GameSpecific::CameraManipulator::getNearZ();
        float farZ = GameSpecific::CameraManipulator::getFarZ();

        fov = max(0.01f, min(fov, 3.0f));
        nearZ = max(nearZ, 0.01f);
        farZ = max(farZ, nearZ + 0.1f);

        const float aspectRatio = _viewPort.Width / _viewPort.Height;
        const float verticalFov = 2.0f * atan(tan(fov / 2.0f) / aspectRatio);

        _viewMatrix = XMMatrixInverse(nullptr, worldMatrix);
        _projMatrix = XMMatrixPerspectiveFovRH(verticalFov, aspectRatio, nearZ, farZ);
    }

    D3D12_VIEWPORT D3D12Hook::getFullViewport()
    {
        float width = 0.0f;
        float height = 0.0f;

        // --- Step 1: Find the dimensions using the best available method ---

        // Method 1 (Primary): Get dimensions from the swap chain. This is the most reliable source.
        if (_pSwapChain)
        {
            // For D3D12, prefer using IDXGISwapChain1 and GetDesc1()
            IDXGISwapChain1* pSwapChain1 = nullptr;
            if (SUCCEEDED(_pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain1))))
            {
                DXGI_SWAP_CHAIN_DESC1 desc;
                if (SUCCEEDED(pSwapChain1->GetDesc1(&desc)))
                {
                    width = static_cast<float>(desc.Width);
                    height = static_cast<float>(desc.Height);
                }
                pSwapChain1->Release();
            }
        }

        // Method 2 (Fallback): If swap chain failed, try the window's client rect.
        if (width == 0.0f || height == 0.0f)
        {
            const HWND hWnd = Globals::instance().mainWindowHandle();
            if (hWnd)
            {
                RECT clientRect;
                if (GetClientRect(hWnd, &clientRect))
                {
                    width = static_cast<float>(clientRect.right - clientRect.left);
                    height = static_cast<float>(clientRect.bottom - clientRect.top);
                }
            }
        }

        // --- Step 2: Update cache and handle final fallbacks ---

        if (width > 0.0f && height > 0.0f)
        {
            // Dimensions found.
            MessageHandler::logDebug("D3D12Hook::getFullViewport: Determined dimensions: %.0f x %.0f", width, height);
        }
        // CHANGED: Use the existing _viewPort as the cache
        else if (_viewPort.Width > 0 && _viewPort.Height > 0)
        {
            // All methods failed, but we have old data in our main viewport struct. Use it.
            width = _viewPort.Width;
            height = _viewPort.Height;
            MessageHandler::logDebug("D3D12Hook::getFullViewport: Using cached dimensions from _viewPort: %.0f x %.0f", width, height);
        }
        else
        {
            // Absolute last resort: use a hardcoded default.
            width = 1280.0f;
            height = 720.0f;
            MessageHandler::logDebug("D3D12Hook::getFullViewport: Using default fallback dimensions: %.0f x %.0f", width, height);
        }

        // --- Step 3: Construct the viewport struct once with the final dimensions ---

        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = width;
        viewport.Height = height;

        if (_useReversedDepth)
        {
            viewport.MinDepth = 1.0f;
            viewport.MaxDepth = 0.0f;
        }
        else
        {
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
        }

        return viewport;
    }

    void D3D12Hook::updateConstantBuffer(const XMMATRIX& worldViewProj) {
        if (_currentFrameIndex >= _perFrameConstantBuffers.size() || _currentCBVIndex >= 256) {
            return; // Safety checks
        }

        // Get the large constant buffer for the current frame in flight.
        const auto& frameCB = _perFrameConstantBuffers[_currentFrameIndex];
        if (!frameCB.mappedData) {
            return;
        }

        // Calculate the size of a single constant buffer entry, aligned to 256 bytes.
        constexpr UINT alignedCBSize = (sizeof(D3D12SimpleConstantBuffer) + 255) & ~255;

        // Determine the memory location for this specific draw call's data.
        UINT8* destPtr = frameCB.mappedData + (_currentCBVIndex * alignedCBSize);

        // Prepare and copy the matrix data.
        D3D12SimpleConstantBuffer cbData;
        cbData.worldViewProjection = XMMatrixTranspose(worldViewProj);
        memcpy(destPtr, &cbData, sizeof(cbData));

        // Get the CPU handle for the descriptor heap slot we're about to use.
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _pCBVHeap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += _currentCBVIndex * _cbvDescriptorSize;

        // Create a new Constant Buffer View (CBV) that points to the specific offset in our large buffer.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = frameCB.gpuAddress + (_currentCBVIndex * alignedCBSize);
        cbvDesc.SizeInBytes = alignedCBSize;
        _pDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);

        // Get the corresponding GPU handle to bind to the root signature.
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _pCBVHeap->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += _currentCBVIndex * _cbvDescriptorSize;
        _pCommandList->SetGraphicsRootDescriptorTable(0, gpuHandle);

        // Move to the next available slot for the next draw call.
        _currentCBVIndex = (_currentCBVIndex + 1);
    }

    //==================================================================================================
	// Path Visualisation Rendering
	//==================================================================================================
    void D3D12Hook::renderPathTubes() {
        if (!_pPathTubeVertexBuffer || !_pPathTubeIndexBuffer || _pathTubeIndexCount == 0) {
            return;
        }

        // Update constant buffer
        const XMMATRIX worldMatrix = XMMatrixIdentity();
        updateConstantBuffer(worldMatrix * _viewMatrix * _projMatrix);

        // Set primitive topology
        //_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set vertex and index buffers
        _pCommandList->IASetVertexBuffers(0, 1, &_pathTubeVertexBufferView);
        _pCommandList->IASetIndexBuffer(&_pathTubeIndexBufferView);

        // Draw each path's tube segment
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                continue;
            }

            if (pathInfo.interpVertexCount > 0) {
                _pCommandList->DrawIndexedInstanced(pathInfo.interpVertexCount, 1, pathInfo.interpStartIndex, 0, 0);
            }
        }
    }

    void D3D12Hook::renderNodeSpheres() {
        static int frameCounter = 0;
        const bool shouldLog = (frameCounter++ % 60 == 0);

        if (!_pSphereVertexBuffer || !_pSphereIndexBuffer || _sphereIndexCount == 0) {
            if (shouldLog && D3DLOGGING && VERBOSE) {
                MessageHandler::logError("renderNodeSpheres: Missing sphere resources - VB:%p IB:%p Count:%d",
                    _pSphereVertexBuffer, _pSphereIndexBuffer, _sphereIndexCount);
            }
            return;
        }

        if (shouldLog && D3DLOGGING && VERBOSE) {
            MessageHandler::logDebug("renderNodeSpheres: Starting - PathInfos:%zu, NodePositions:%zu",
                _pathInfos.size(), _nodePositions.size());
        }

        // Set vertex and index buffers for the sphere
        _pCommandList->IASetVertexBuffers(0, 1, &_sphereVertexBufferView);
        _pCommandList->IASetIndexBuffer(&_sphereIndexBufferView);

        int totalSpheresRendered = 0;

        // Render spheres using path iteration (like D3DHook)
        for (const auto& pathInfo : _pathInfos) {
            if (shouldLog && D3DLOGGING && VERBOSE) {
                MessageHandler::logDebug("  Path: %s, NodeCount:%d, StartIndex:%d",
                    pathInfo.name.c_str(), pathInfo.nodeCount, pathInfo.nodeStartIndex);
            }

            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                if (shouldLog && D3DLOGGING && VERBOSE) {
                    MessageHandler::logDebug("    Skipping - not selected path");
                }
                continue;
            }

            for (UINT i = 0; i < pathInfo.nodeCount; i++) {
                const UINT nodeIndex = pathInfo.nodeStartIndex + i;
                if (nodeIndex >= _nodePositions.size()) {
                    if (shouldLog && D3DLOGGING && VERBOSE) {
                        MessageHandler::logError("    Node index out of bounds: %d >= %zu",
                            nodeIndex, _nodePositions.size());
                    }
                    continue;
                }

                const XMFLOAT3& nodePos = _nodePositions[nodeIndex];
                if (shouldLog && D3DLOGGING && VERBOSE && i < 3) { // Only log first 3 nodes per path to avoid spam
                    MessageHandler::logDebug("    Node %d: Position(%.2f, %.2f, %.2f)",
                        i, nodePos.x, nodePos.y, nodePos.z);
                }

                const XMVECTOR nodePosition = XMLoadFloat3(&nodePos);
                XMMATRIX worldMatrix = XMMatrixScaling(_nodeSize, _nodeSize, _nodeSize) *
                    XMMatrixTranslationFromVector(nodePosition);

                updateConstantBuffer(worldMatrix * _viewMatrix * _projMatrix);
                _pCommandList->DrawIndexedInstanced(_sphereIndexCount, 1, 0, 0, 0);

                totalSpheresRendered++;
            }
        }

        if (shouldLog && D3DLOGGING && VERBOSE) {
            MessageHandler::logDebug("renderNodeSpheres: Rendered %d spheres total", totalSpheresRendered);
        }
    }

    void D3D12Hook::renderDirectionArrows() {
        // Debug logging

        static int frameCounter = 0;
        frameCounter++;

        if (!_pArrowHeadVertexBuffer || !_pArrowHeadIndexBuffer ||
            !_pArrowShaftVertexBuffer || !_pArrowShaftIndexBuffer) {
            if (frameCounter % 60 == 1 && D3DLOGGING && VERBOSE) {
                MessageHandler::logDebug("renderDirectionArrows: Arrow buffers not initialized");
            }
            return;
        }


        if (frameCounter % 60 == 0 && D3DLOGGING && VERBOSE) {


            MessageHandler::logDebug("renderDirectionArrows: Starting");
            MessageHandler::logDebug("  PathInfos count: %zu", _pathInfos.size());

            for (const auto& pathInfo : _pathInfos) {
                MessageHandler::logDebug("  Path '%s': %zu direction pairs",
                    pathInfo.name.c_str(), pathInfo.pathDirections.size());
            }
        }

        // Get all paths from the path manager
        const auto& paths = PathManager::instance().getPaths();
        if (paths.empty()) {
            if (frameCounter % 60 == 1 && D3DLOGGING && VERBOSE) {
                MessageHandler::logDebug("  No paths in manager");
            }
            return;
        }

        // Calculate FOV scale factor for arrow size
        float currentFOV = GameSpecific::CameraManipulator::getCurrentFoV();
        float fovScaleFactor = currentFOV / DEFAULT_FOV;

        int arrowsRendered = 0;

        // 1. Render arrows for interpolated direction indicators
        for (const auto& pathInfo : _pathInfos) {
            if (_renderSelectedPathOnly && pathInfo.name != PathManager::instance().getSelectedPath()) {
                continue;
            }

            // Render this path's interpolated direction arrows
            for (const auto& dirPair : pathInfo.pathDirections) {
                const XMFLOAT3& startPos = dirPair.first;
                const XMFLOAT3& endPos = dirPair.second;

                // Debug first arrow
                if (frameCounter % 60 == 1 && arrowsRendered == 0 && D3DLOGGING && VERBOSE) {
                    MessageHandler::logDebug("  First interpolated arrow: start(%.2f,%.2f,%.2f) end(%.2f,%.2f,%.2f)",
                        startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);
                }

                // Calculate direction vector
                XMVECTOR start = XMLoadFloat3(&startPos);
                XMVECTOR end = XMLoadFloat3(&endPos);
                XMVECTOR direction = XMVectorSubtract(end, start);

                // Calculate length
                float length = XMVectorGetX(XMVector3Length(direction));
                if (length < 0.001f) continue;

                // Orange color for interpolated path indicators
                auto arrowColor = XMFLOAT4(1.0f, 0.5f, 0.0f, 0.8f);

                // Render arrow - now uses the D3DHook-style function
                renderArrow(startPos, direction, length, arrowColor);
                arrowsRendered++;
            }
        }

        // 2. Render arrows for node direction indicators
        for (const auto& pathPair : paths) {
            const CameraPath& path = pathPair.second;
            const std::string& pathName = pathPair.first;

            if (_renderSelectedPathOnly && pathName != PathManager::instance().getSelectedPath()) {
                continue;
            }

            if (path.getNodeSize() < 2) {
                continue;
            }

            // Debug first node
            if (frameCounter % 60 == 1 && path.getNodeSize() > 0 && D3DLOGGING && VERBOSE) {
                XMVECTOR nodePos = path.getNodePosition(0);
                XMFLOAT3 pos;
                XMStoreFloat3(&pos, nodePos);
                MessageHandler::logDebug("  First node arrow: pos(%.2f,%.2f,%.2f)", pos.x, pos.y, pos.z);
            }

            for (size_t i = 0; i < path.getNodeSize(); i++) {
                XMVECTOR nodePos = path.getNodePosition(static_cast<uint8_t>(i));
                XMFLOAT3 position;
                XMStoreFloat3(&position, nodePos);

                XMVECTOR rotation = path.getNodeRotation(static_cast<uint8_t>(i));

                // Calculate forward direction vector from rotation quaternion
                XMVECTOR forwardDir = XMVector3Rotate(
                    XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
                    rotation
                );

                const float nodeFov = path.getNodeFOV(static_cast<uint8_t>(i));
                const float nodeFovScaleFactor = _defaultFOV / nodeFov;
                const float scaledLength = _baseDirectionLength * nodeFovScaleFactor;

                auto arrowColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

                // Render arrow with direction vector and length
                renderArrow(position, forwardDir, scaledLength, arrowColor);
                arrowsRendered++;
            }
        }

        if (frameCounter % 60 == 1 && D3DLOGGING && VERBOSE) {
            MessageHandler::logDebug("  Total arrows rendered: %d", arrowsRendered);
        }
    }

    void D3D12Hook::renderArrow(const XMFLOAT3& position,
                                const XMVECTOR& direction,
                                const float length,
                                const XMFLOAT4& color) {

        if (!_pCommandList || !_pArrowHeadVertexBuffer || !_pArrowShaftVertexBuffer) {
            return;
        }

        // Normalize direction vector
        const XMVECTOR normalizedDir = XMVector3Normalize(direction);

        // Create rotation matrix to align arrow with direction
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        if (fabsf(XMVectorGetY(normalizedDir)) > 0.99f) {
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        }

        const XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, normalizedDir));
        up = XMVector3Cross(normalizedDir, right);

        XMMATRIX rotMatrix;
        rotMatrix.r[0] = right;
        rotMatrix.r[1] = up;
        rotMatrix.r[2] = normalizedDir;
        rotMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        // Calculate length scale factor - this affects only the z-axis (length)
        const float lengthScale = length / _arrowShaftLength;

        // Use non-uniform scaling - scale ONLY the Z dimension (length)
        // Keep X and Y dimensions (thickness) constant with scale 1.0
        const XMMATRIX scaleMatrix = XMMatrixScaling(1.0f, 1.0f, lengthScale);

        // The arrow geometry points along +Z, but cameras typically look along -Z
        // So we need to rotate 180 degrees around Y
        const XMMATRIX flipMatrix = XMMatrixRotationY(XM_PI);

        // Translation matrix
        const XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

        // Combine transformations: Scale -> Flip -> Rotation -> Translation
        const XMMATRIX worldMatrix = scaleMatrix * flipMatrix * rotMatrix * translationMatrix;

        // Update constant buffer
        updateConstantBuffer(worldMatrix * _viewMatrix * _projMatrix);

        // Set primitive topology
        //_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Render shaft first (so head overlaps it)
        _pCommandList->IASetVertexBuffers(0, 1, &_arrowShaftVertexBufferView);
        _pCommandList->IASetIndexBuffer(&_arrowShaftIndexBufferView);
        _pCommandList->DrawIndexedInstanced(_arrowShaftIndexCount, 1, 0, 0, 0);

        // Render head
        _pCommandList->IASetVertexBuffers(0, 1, &_arrowHeadVertexBufferView);
        _pCommandList->IASetIndexBuffer(&_arrowHeadIndexBufferView);
        _pCommandList->DrawIndexedInstanced(_arrowHeadIndexCount, 1, 0, 0, 0);
    }

    void D3D12Hook::renderFreeCameraLookAtTarget() {
    	if (!Camera::instance().getCameraLookAtVisualizationEnabled() && _visualisationEnabled)
            return;
        // Only render free camera look-at target when:
        // 1. Look-at is enabled and in target offset mode
        // 2. NOT in path manager mode (free camera only)
        // 3. We have valid resources and target position

        const auto& pathManager = PathManager::instance();
		const auto& camera = Camera::instance();
        if (!Globals::instance().settings().lookAtEnabled ||
            !camera.getUseTargetOffsetMode() ||
            pathManager._pathManagerState ||  // Don't show in path mode
            !camera.hasValidLookAtTarget() ||
            !_pLookAtTargetVertexBuffer || !_pLookAtTargetIndexBuffer || _lookAtTargetIndexCount == 0) {
            return;
        }

        // Get look-at target position
        const XMFLOAT3 targetPosition = Camera::instance().getCurrentLookAtTargetPosition();
        const XMVECTOR targetPos = XMLoadFloat3(&targetPosition);

        // Create world matrix for the look-at target sphere
        const XMMATRIX worldMatrix = XMMatrixScaling(_lookAtTargetSize, _lookAtTargetSize, _lookAtTargetSize) *
            XMMatrixTranslationFromVector(targetPos);

        // Update constant buffer
        updateConstantBuffer(worldMatrix * _viewMatrix * _projMatrix);

        // Set vertex and index buffers
        _pCommandList->IASetVertexBuffers(0, 1, &_lookAtTargetVertexBufferView);
        _pCommandList->IASetIndexBuffer(&_lookAtTargetIndexBufferView);

        // Draw the sphere
        _pCommandList->DrawIndexedInstanced(_lookAtTargetIndexCount, 1, 0, 0, 0);
    }

    void D3D12Hook::renderPathLookAtTarget() {
        if (!Globals::instance().settings().pathLookAtEnabled)
            return;
        // Only render path look-at target when:
        // 1. Path visualization is enabled
        // 2. Path manager is active with a selected path
        // 3. Path look-at is enabled on the current path
        // 4. We have valid resources and target position

        if (!_visualisationEnabled ||
            !_pLookAtTargetVertexBuffer || !_pLookAtTargetIndexBuffer || _lookAtTargetIndexCount == 0) {
            return;
        }

        auto& pathManager = PathManager::instance();
        if (!pathManager._pathManagerState ||
            pathManager.getSelectedPathPtr() == nullptr ||
            !Globals::instance().settings().pathLookAtEnabled ||
            !pathManager.getSelectedPathPtr()->hasValidPathLookAtTarget()) {
            return;
        }

        const XMFLOAT3 targetPosition = pathManager.getSelectedPathPtr()->getCurrentPathLookAtTargetPosition();

        // Update constant buffer
        const XMVECTOR targetPos = XMLoadFloat3(&targetPosition);
        const XMMATRIX worldMatrix = XMMatrixScaling(_lookAtTargetSize, _lookAtTargetSize, _lookAtTargetSize) *
            XMMatrixTranslationFromVector(targetPos);

        updateConstantBuffer(worldMatrix * _viewMatrix * _projMatrix);

        // Set primitive topology
        //_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set vertex and index buffers
        _pCommandList->IASetVertexBuffers(0, 1, &_lookAtTargetVertexBufferView);
        _pCommandList->IASetIndexBuffer(&_lookAtTargetIndexBufferView);

        // Draw the sphere
        _pCommandList->DrawIndexedInstanced(_lookAtTargetIndexCount, 1, 0, 0, 0);
    }

    //==================================================================================================
	// Path Visualisation Setup
	//==================================================================================================
    // Requires path count to be checked before calling directly. Else call safeInterpolationModeChange
    // which does all the safety checks and resource cleanup before calling this.
    void D3D12Hook::createPathVisualisation() {
        // This function assumes safeInterpolationModeChange has done all safety checks
        // and resource cleanup. It purely creates new resources.

        // Get paths
        const auto& paths = PathManager::instance().getPaths();

        // Prepare vertex collections
        std::vector<D3D12SimpleVertex> directionIndicatorVertices;
        std::vector<D3D12SimpleVertex> interpolatedDirectionVertices;
        std::vector<D3D12SimpleVertex> tubeMeshVertices;
        std::vector<UINT> tubeMeshIndices;
        std::vector<TubeInfo> pathTubeInfos;

        // Set base colors
        constexpr auto nodeColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for nodes
        constexpr auto directionColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // Red for direction indicators

        // Process all paths
        processAllPaths(paths, directionIndicatorVertices, interpolatedDirectionVertices,
            tubeMeshVertices, tubeMeshIndices,
            pathTubeInfos, nodeColor, directionColor);

        // Store total counts
        _directionIndicatorVertexCount = static_cast<UINT>(directionIndicatorVertices.size());
        _pathTubeVertexCount = static_cast<UINT>(tubeMeshVertices.size());
        _pathTubeIndexCount = static_cast<UINT>(tubeMeshIndices.size());
        _interpolatedDirectionsVertexCount = static_cast<UINT>(interpolatedDirectionVertices.size());

        // Create buffers for all the geometry
        createPathBuffers(directionIndicatorVertices, interpolatedDirectionVertices,
            tubeMeshVertices, tubeMeshIndices,
            pathTubeInfos);

        // Mark resources as created
        _pathResourcesCreated = true;

        MessageHandler::logDebug("D3D12Hook::createPathVisualisation: Successfully created path visualization resources");
        MessageHandler::logDebug("D3D12Hook::createPathVisualisation: Paths: %d, Nodes: %d, Tube vertices: %d, Tube indices: %d, Interpolated directions: %d",
            static_cast<int>(_pathInfos.size()),
            static_cast<int>(_nodePositions.size()),
            _pathTubeVertexCount,
            _pathTubeIndexCount,
            _interpolatedDirectionsVertexCount);
    }

    void D3D12Hook::processAllPaths(const std::unordered_map<std::string, CameraPath>& paths,
        std::vector<D3D12SimpleVertex>& directionIndicatorVertices,
        std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
        std::vector<D3D12SimpleVertex>& tubeMeshVertices,
        std::vector<UINT>& tubeMeshIndices,
        std::vector<TubeInfo>& pathTubeInfos,
        const XMFLOAT4& nodeColor,
        const XMFLOAT4& directionColor) {

        int pathIndex = 0;
        for (const auto& pathPair : paths) {
            const std::string& pathName = pathPair.first;
            const CameraPath& path = pathPair.second;
            const size_t nodeCount = path.getNodeSize();

            if (nodeCount < 2) // Need at least 2 nodes to form a path
                continue;

            MessageHandler::logDebug("D3D12Hook::processAllPaths: Processing path '%s' with %zu nodes",
                pathName.c_str(), nodeCount);

            // Generate a unique color for this path based on golden ratio
            const float hue = (pathIndex * 137.5f) / 360.0f;
            constexpr float saturation = 0.8f;
            constexpr float value = 0.9f;

            // Convert HSV to RGB for path color
            XMFLOAT4 pathColor = convertHSVtoRGB(hue, saturation, value, 1.0f);

            // Create path info record
            PathInfo pathInfo;
            pathInfo.name = pathName;
            pathInfo.nodeStartIndex = static_cast<UINT>(_nodePositions.size());
            pathInfo.directionStartIndex = static_cast<UINT>(directionIndicatorVertices.size());
            pathInfo.color = pathColor;

            // Start tracking tube indices
            TubeInfo tubeInfo;
            tubeInfo.startIndex = static_cast<UINT>(tubeMeshIndices.size());
            tubeInfo.color = pathColor;

            // Process the nodes for this path
            processPathNodes(path, nodeCount, directionIndicatorVertices, nodeColor, directionColor);

            // Generate interpolated samples for this path
            auto interpSamples = generateInterpolatedSamples(path, nodeCount);

            // Create interpolated direction indicators
            const size_t interpDirStart = interpolatedDirectionVertices.size();
            createInterpolatedDirections(interpSamples, interpolatedDirectionVertices,
                pathInfo.nodeStartIndex, static_cast<UINT>(nodeCount));

            // Calculate how many interpolated direction vertices were added
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

    // Process nodes for a path
    void D3D12Hook::processPathNodes(const CameraPath& path, const size_t nodeCount,
        std::vector<D3D12SimpleVertex>& directionIndicatorVertices,
        const XMFLOAT4& nodeColor,
        const XMFLOAT4& directionColor) {

        for (size_t i = 0; i < nodeCount; i++) {
            const XMVECTOR nodePos = path.getNodePosition(static_cast<uint8_t>(i));
            XMFLOAT3 pos;
            XMStoreFloat3(&pos, nodePos);

            // Store node position for rendering spheres
            _nodePositions.push_back(pos);

            // Create direction indicator
            const XMVECTOR rotation = path.getNodeRotation(static_cast<uint8_t>(i));
            const XMVECTOR forward = XMVector3Rotate(
                XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), // Forward vector (Z+)
                rotation // Rotate by the camera's rotation
            );

            // Start and end points for direction line
            D3D12SimpleVertex dirStart, dirEnd;
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

    // Generate interpolated samples along a path
    std::vector<D3D12Hook::InterpSample> D3D12Hook::generateInterpolatedSamples(const CameraPath& path, size_t nodeCount) {
        std::vector<InterpSample> interpSamples;
        const std::vector<CameraNode>* nodes = path.getNodes();
        const std::vector<float>* knotVector = path.getknotVector();
        const std::vector<XMVECTOR>* controlPointsPos = path.getControlPointsPos();
        const std::vector<float>* controlPointsFoV = path.getControlPointsFOV();
        const std::vector<XMVECTOR>* controlPointsRot = path.getControlPointsRot();

        // Get the number of steps for the path interpolation
        int sampleSize = CameraPath::getSampleSize();
        sampleSize = static_cast<int>(floor(sampleSize / 2));

        // Generate samples for each path segment
        for (size_t i = 0; i < nodeCount - 1; i++) {
            for (int step = 0; step <= sampleSize; step++) {
                float t = static_cast<float>(step) / static_cast<float>(sampleSize);
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

    // Create interpolated direction indicators
    void D3D12Hook::createInterpolatedDirections(
        const std::vector<InterpSample>& interpSamples,
        std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
        const UINT nodeStartIndex,
        const UINT nodeCount) const {

        // Use a configurable threshold for proximity check
        const float proximityThresholdSq = _nodeProximityThreshold * _nodeProximityThreshold;

        int sampleSize = CameraPath::getSampleSize();
        sampleSize = static_cast<int>(floor(sampleSize / 2));

        for (size_t i = 0; i < interpSamples.size(); i += (sampleSize / _rotationSampleFrequency)) {
            // Only create indicators at every Nth sample to avoid cluttering
            const InterpSample& sample = interpSamples[i];

            // Check proximity to nodes for this path
            bool tooCloseToNode = false;
            for (UINT j = 0; j < nodeCount; j++) {
                const XMFLOAT3& nodePos = _nodePositions[nodeStartIndex + j];

                // Calculate squared distance (faster than calculating actual distance)
                const float dx = sample.position.x - nodePos.x;
                const float dy = sample.position.y - nodePos.y;
                const float dz = sample.position.z - nodePos.z;
                const float distSquared = dx * dx + dy * dy + dz * dz;

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
            const float fovScaleFactor = _defaultFOV / sample.fov;
            const float scaledLength = _baseDirectionLength * fovScaleFactor;

            // Calculate forward vector from rotation quaternion
            const XMVECTOR forward = XMVector3Rotate(
                XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
                sample.rotation
            );

            // Create indicator vertices
            D3D12SimpleVertex dirStart, dirEnd;
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

    // Store path direction data
    void D3D12Hook::storePathDirections(PathInfo& pathInfo,
        const std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
        const size_t startIndex, const size_t count) {

        pathInfo.pathDirections.clear();
        for (size_t i = startIndex; i < startIndex + count; i += 2) {
            pathInfo.pathDirections.push_back(std::make_pair(
                interpolatedDirectionVertices[i].position,
                interpolatedDirectionVertices[i + 1].position
            ));
        }
    }

    // Create tubes for path visualization
    void D3D12Hook::createPathTubes(const std::vector<XMFLOAT3>& points,
        std::vector<D3D12SimpleVertex>& tubeMeshVertices,
        std::vector<UINT>& tubeMeshIndices,
        const XMFLOAT4& color) const {

        for (size_t i = 0; i < points.size() - 1; i++) {
            // generateTubeMesh already exists in D3D12Hook, we just need to ensure it's being called correctly
            auto tubeInfos = instance().generateTubeMesh(
                std::vector{points[i], points[i + 1]},
                _tubeDiameter / 2.0f,
                _tubeSegments,
                tubeMeshVertices,
                tubeMeshIndices
            );

            // Apply the color to the vertices that were just added
            if (!tubeInfos.empty() && tubeMeshVertices.size() > 0) {
                // Color the vertices that were just added
                const size_t startIdx = tubeMeshVertices.size() - (tubeInfos[0].indexCount / 3);
                for (size_t j = startIdx; j < tubeMeshVertices.size(); j++) {
                    tubeMeshVertices[j].color = color;
                }
            }
        }
    }

    // Create Direct3D12 vertex and index buffers
    void D3D12Hook::createPathBuffers(
        const std::vector<D3D12SimpleVertex>& directionIndicatorVertices,
        const std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
        const std::vector<D3D12SimpleVertex>& tubeMeshVertices,
        const std::vector<UINT>& tubeMeshIndices,
        const std::vector<TubeInfo>& pathTubeInfos) {

        // Create direction indicator vertex buffer
        if (_directionIndicatorVertexCount > 0) {
            createVertexBuffer(directionIndicatorVertices, &_directionIndicatorVertexBuffer, &_directionIndicatorVertexBufferView);
        }

        // Create interpolated direction buffer
        if (_interpolatedDirectionsVertexCount > 0) {
            createVertexBuffer(interpolatedDirectionVertices, &_interpolatedDirectionsVertexBuffer, &_interpolatedDirectionsVertexBufferView);
        }

        // Create tube mesh buffers (already exists in current implementation)
        if (_pathTubeVertexCount > 0 && _pathTubeIndexCount > 0) {
            createVertexBuffer(tubeMeshVertices, &_pPathTubeVertexBuffer, &_pathTubeVertexBufferView);
            createIndexBuffer(tubeMeshIndices, &_pPathTubeIndexBuffer, &_pathTubeIndexBufferView);

            // Store tube infos with paths
            for (size_t i = 0; i < pathTubeInfos.size() && i < _pathInfos.size(); i++) {
                _pathInfos[i].interpStartIndex = pathTubeInfos[i].startIndex;
                _pathInfos[i].interpVertexCount = pathTubeInfos[i].indexCount;
            }
        }
    }

    void D3D12Hook::safeInterpolationModeChange() {
        std::lock_guard lock(_resourceMutex);

        // 1. Check if we have paths
        const auto& paths = PathManager::instance().getPaths();
        if (paths.empty()) {
            // Clear all tracking data
            _pathResourcesCreated = false;
            _pathInfos.clear();
            _nodePositions.clear();
            _isChangingMode.store(false, std::memory_order_release);
			_resourcesNeedUpdate.store(false, std::memory_order_release); // No resources to update
            MessageHandler::logDebug("D3D12Hook::safeInterpolationModeChange: No paths to visualize");
            return;
        }

        // 2. Check device is ready
        if (!_pDevice || !_pCommandQueue) {
            MessageHandler::logError("D3D12Hook::safeInterpolationModeChange: Device or CommandQueue is null");
            return;
        }

        // 3. Set changing mode flag
        _isChangingMode.store(true, std::memory_order_release);

        // 4. Wait for GPU to finish with current resources
        synchroniseGPU();

        // 5. Clear ALL buffer views
        _pathTubeVertexBufferView = {};
        _pathTubeIndexBufferView = {};
        _directionIndicatorVertexBufferView = {};
        _interpolatedDirectionsVertexBufferView = {};

        // 6. Clear ALL counts
        _pathTubeVertexCount = 0;
        _pathTubeIndexCount = 0;
        _directionIndicatorVertexCount = 0;
        _interpolatedDirectionsVertexCount = 0;


        // 7. Release ALL path visualization resources
        if (_pPathTubeVertexBuffer) {
            _pPathTubeVertexBuffer->Release();
            _pPathTubeVertexBuffer = nullptr;
        }
        if (_pPathTubeIndexBuffer) {
            _pPathTubeIndexBuffer->Release();
            _pPathTubeIndexBuffer = nullptr;
        }
        if (_directionIndicatorVertexBuffer) {
            _directionIndicatorVertexBuffer->Release();
            _directionIndicatorVertexBuffer = nullptr;
        }
        if (_interpolatedDirectionsVertexBuffer) {
            _interpolatedDirectionsVertexBuffer->Release();
            _interpolatedDirectionsVertexBuffer = nullptr;
        }

        // 8. Clear path data structures
        _pathResourcesCreated = false;
        _pathInfos.clear();
        _nodePositions.clear();

        // 9. Reset changing mode flag
        _isChangingMode.store(false, std::memory_order_release);

        // 10. Now create new visualization
        createPathVisualisation();

    	_resourcesNeedUpdate.store(false, std::memory_order_release);

        MessageHandler::logDebug("D3D12Hook::safeInterpolationModeChange: Interpolation mode changed successfully");
    }

    //==================================================================================================
	// Geometry Creation
	//==================================================================================================
    void D3D12Hook::createNodeSphere(const float radius, const XMFLOAT4& color) {
        if (!_pDevice) {
            return;
        }

        // Clean up existing sphere resources if they exist
        if (_pSphereVertexBuffer) {
            _pSphereVertexBuffer->Release();
            _pSphereVertexBuffer = nullptr;
        }
        if (_pSphereIndexBuffer) {
            _pSphereIndexBuffer->Release();
            _pSphereIndexBuffer = nullptr;
        }

        std::vector<D3D12SimpleVertex> vertices;
        std::vector<UINT> indices;

        constexpr int latitudeSegments = 12;
        constexpr int longitudeSegments = 12;

        // Generate vertices
        for (int lat = 0; lat <= latitudeSegments; lat++) {
            const float theta = static_cast<float>(lat) * XM_PI / latitudeSegments;
            const float sinTheta = sinf(theta);
            const float cosTheta = cosf(theta);

            for (int lon = 0; lon <= longitudeSegments; lon++) {
                const float phi = static_cast<float>(lon) * 2 * XM_PI / longitudeSegments;
                const float sinPhi = sinf(phi);
                const float cosPhi = cosf(phi);

                D3D12SimpleVertex vertex;
                vertex.position.x = radius * sinTheta * cosPhi;
                vertex.position.y = radius * cosTheta;
                vertex.position.z = radius * sinTheta * sinPhi;
                vertex.color = color;

                vertices.push_back(vertex);
            }
        }

        // Generate indices
        for (int lat = 0; lat < latitudeSegments; lat++) {
            for (int lon = 0; lon < longitudeSegments; lon++) {
                const int first = (lat * (longitudeSegments + 1)) + lon;
                const int second = first + longitudeSegments + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        createVertexBuffer(vertices, &_pSphereVertexBuffer, &_sphereVertexBufferView);
        createIndexBuffer(indices, &_pSphereIndexBuffer, &_sphereIndexBufferView);
        _sphereIndexCount = static_cast<UINT>(indices.size());

        MessageHandler::logDebug("D3D12Hook::createNodeSphere: Created sphere with %zu vertices, %zu indices, radius:%.2f",
            vertices.size(), indices.size(), radius);
        MessageHandler::logDebug("  VertexBuffer:%p, IndexBuffer:%p", _pSphereVertexBuffer, _pSphereIndexBuffer);
    }

	void D3D12Hook::createSolidColorSphere(const float radius, const XMFLOAT4& color) {
        if (!_pDevice) {
            return;
        }

        // Clean up existing resources
        if (_pLookAtTargetVertexBuffer) {
            _pLookAtTargetVertexBuffer->Release();
            _pLookAtTargetVertexBuffer = nullptr;
        }
        if (_pLookAtTargetIndexBuffer) {
            _pLookAtTargetIndexBuffer->Release();
            _pLookAtTargetIndexBuffer = nullptr;
        }

        _lookAtTargetSize = radius;

        // Generate sphere geometry
        std::vector<D3D12SimpleVertex> vertices;
        std::vector<UINT> indices;

        constexpr int latitudeSegments = 16;
        constexpr int longitudeSegments = 16;

        // Generate vertices
        for (int lat = 0; lat <= latitudeSegments; lat++) {
            const float theta = static_cast<float>(lat) * XM_PI / latitudeSegments;
            const float sinTheta = sinf(theta);
            const float cosTheta = cosf(theta);

            for (int lon = 0; lon <= longitudeSegments; lon++) {
                const float phi = static_cast<float>(lon) * 2 * XM_PI / longitudeSegments;
                const float sinPhi = sinf(phi);
                const float cosPhi = cosf(phi);

                D3D12SimpleVertex vertex;
                vertex.position.x = radius * sinTheta * cosPhi;
                vertex.position.y = radius * cosTheta;
                vertex.position.z = radius * sinTheta * sinPhi;
                vertex.color = color;

                vertices.push_back(vertex);
            }
        }

        // Generate indices
        for (int lat = 0; lat < latitudeSegments; lat++) {
            for (int lon = 0; lon < longitudeSegments; lon++) {
                const int first = (lat * (longitudeSegments + 1)) + lon;
                const int second = first + longitudeSegments + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        // Create buffers
        createVertexBuffer(vertices, &_pLookAtTargetVertexBuffer, &_lookAtTargetVertexBufferView);
        createIndexBuffer(indices, &_pLookAtTargetIndexBuffer, &_lookAtTargetIndexBufferView);
        _lookAtTargetIndexCount = static_cast<UINT>(indices.size());

        MessageHandler::logDebug("D3D12Hook::createSolidColorSphere: Created sphere with radius %.2f", radius);
    }

	void D3D12Hook::createArrowGeometry() {
        if (!_pDevice) {
            MessageHandler::logError("D3D12Hook::createArrowGeometry: Device is null");
            return;
        }

        // Clean up existing arrow resources
        if (_pArrowHeadVertexBuffer) {
            _pArrowHeadVertexBuffer->Release();
            _pArrowHeadVertexBuffer = nullptr;
        }
        if (_pArrowHeadIndexBuffer) {
            _pArrowHeadIndexBuffer->Release();
            _pArrowHeadIndexBuffer = nullptr;
        }
        if (_pArrowShaftVertexBuffer) {
            _pArrowShaftVertexBuffer->Release();
            _pArrowShaftVertexBuffer = nullptr;
        }
        if (_pArrowShaftIndexBuffer) {
            _pArrowShaftIndexBuffer->Release();
            _pArrowShaftIndexBuffer = nullptr;
        }

        // Create head and shaft separately
        createArrowHead();
        createArrowShaft();

        MessageHandler::logDebug("D3D12Hook::createArrowGeometry: Successfully created arrow geometry");
    }

    void D3D12Hook::createArrowHead() {
        std::vector<D3D12SimpleVertex> vertices;
        std::vector<UINT> indices;

        // Arrow head is a cone pointing along +Z
        // The base is at z = _arrowShaftLength, the tip is at z = _arrowShaftLength + _arrowHeadLength

        // Add apex vertex (tip of the cone)
        D3D12SimpleVertex apex;
        apex.position = XMFLOAT3(0.0f, 0.0f, _arrowShaftLength + _arrowHeadLength);
        apex.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        vertices.push_back(apex);

        // Add base vertices at z = _arrowShaftLength
        for (int i = 0; i < _arrowSegments; i++) {
            const float angle = XM_2PI * static_cast<float>(i) / static_cast<float>(_arrowSegments);
            const float x = _arrowHeadRadius * cosf(angle);
            const float y = _arrowHeadRadius * sinf(angle);

            D3D12SimpleVertex baseVertex;
            baseVertex.position = XMFLOAT3(x, y, _arrowShaftLength);
            baseVertex.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // Red
            vertices.push_back(baseVertex);
        }

        // Create triangles for the cone
        for (int i = 0; i < _arrowSegments; i++) {
            const int baseIndex1 = i + 1;
            const int baseIndex2 = ((i + 1) % _arrowSegments) + 1;

            // Triangle from apex to base edge
            indices.push_back(0); // Apex
            indices.push_back(baseIndex1);
            indices.push_back(baseIndex2);
        }

        // Create bottom cap of the cone (optional for better visibility)
        const int centerIndex = static_cast<int>(vertices.size());
        D3D12SimpleVertex baseCenter;
        baseCenter.position = XMFLOAT3(0.0f, 0.0f, _arrowShaftLength);
        baseCenter.color = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f); // Darker red
        vertices.push_back(baseCenter);

        for (int i = 0; i < _arrowSegments; i++) {
            const int baseIndex1 = i + 1;
            const int baseIndex2 = ((i + 1) % _arrowSegments) + 1;

            // Triangle for base cap (reversed winding for inward facing)
            indices.push_back(centerIndex);
            indices.push_back(baseIndex2);
            indices.push_back(baseIndex1);
        }

        // Create buffers
        createVertexBuffer(vertices, &_pArrowHeadVertexBuffer, &_arrowHeadVertexBufferView);
        createIndexBuffer(indices, &_pArrowHeadIndexBuffer, &_arrowHeadIndexBufferView);
        _arrowHeadIndexCount = static_cast<UINT>(indices.size());

        MessageHandler::logDebug("D3D12Hook::createArrowHead: Created cone with %d vertices, %d indices",
            vertices.size(), indices.size());
    }

    void D3D12Hook::createArrowShaft() {
        std::vector<D3D12SimpleVertex> vertices;
        std::vector<UINT> indices;

        // Arrow shaft is a cylinder from z=0 to z=_arrowShaftLength

        // Create vertices for cylinder rings at both ends
        for (int ring = 0; ring <= 1; ring++) {
            const float z = static_cast<float>(ring) * _arrowShaftLength;

            for (int i = 0; i < _arrowSegments; i++) {
                const float angle = XM_2PI * static_cast<float>(i) / static_cast<float>(_arrowSegments);
                const float x = _arrowShaftRadius * cosf(angle);
                const float y = _arrowShaftRadius * sinf(angle);

                D3D12SimpleVertex vertex;
                vertex.position = XMFLOAT3(x, y, z);
                vertex.color = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f); // Slightly darker red
                vertices.push_back(vertex);
            }
        }

        // Create cylinder side triangles
        for (int i = 0; i < _arrowSegments; i++) {
            const int i0 = i;                               // Current bottom
            const int i1 = (i + 1) % _arrowSegments;       // Next bottom
            const int i2 = i + _arrowSegments;             // Current top
            const int i3 = i1 + _arrowSegments;            // Next top

            // First triangle
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            // Second triangle
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }

        // Add end caps for better visibility
        // Bottom cap center
        const int bottomCenterIndex = static_cast<int>(vertices.size());
        D3D12SimpleVertex bottomCenter;
        bottomCenter.position = XMFLOAT3(0.0f, 0.0f, 0.0f);
        bottomCenter.color = XMFLOAT4(0.6f, 0.0f, 0.0f, 1.0f);
        vertices.push_back(bottomCenter);

        // Bottom cap triangles
        for (int i = 0; i < _arrowSegments; i++) {
            const int i0 = i;
            const int i1 = (i + 1) % _arrowSegments;

            indices.push_back(bottomCenterIndex);
            indices.push_back(i1);
            indices.push_back(i0);
        }

        // Create buffers
        createVertexBuffer(vertices, &_pArrowShaftVertexBuffer, &_arrowShaftVertexBufferView);
        createIndexBuffer(indices, &_pArrowShaftIndexBuffer, &_arrowShaftIndexBufferView);
        _arrowShaftIndexCount = static_cast<UINT>(indices.size());

        MessageHandler::logDebug("D3D12Hook::createArrowShaft: Created cylinder with %d vertices, %d indices",
            vertices.size(), indices.size());
    }

    std::vector<D3D12Hook::TubeInfo> D3D12Hook::generateTubeMesh(
        const std::vector<XMFLOAT3>& pathPoints,
        float radius,
        int segments,
        std::vector<D3D12SimpleVertex>& outVertices,
        std::vector<UINT>& outIndices) {

        std::vector<TubeInfo> tubeInfos;

        if (pathPoints.size() < 2) {
            return tubeInfos;
        }

        TubeInfo info;
        info.startIndex = static_cast<UINT>(outIndices.size());

        // Store the starting vertex index before adding new vertices
        UINT baseVertexIndex = static_cast<UINT>(outVertices.size());

        // Generate vertices for each ring along the path
        for (size_t i = 0; i < pathPoints.size(); i++) {
            XMVECTOR current = XMLoadFloat3(&pathPoints[i]);
            XMVECTOR forward;

            // Calculate forward direction
            if (i == 0) {
                forward = XMVectorSubtract(XMLoadFloat3(&pathPoints[i + 1]), current);
            }
            else if (i == pathPoints.size() - 1) {
                forward = XMVectorSubtract(current, XMLoadFloat3(&pathPoints[i - 1]));
            }
            else {
                forward = XMVectorSubtract(XMLoadFloat3(&pathPoints[i + 1]), XMLoadFloat3(&pathPoints[i - 1]));
            }

            forward = XMVector3Normalize(forward);

            // Find perpendicular vectors
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            if (fabsf(XMVectorGetY(forward)) > 0.99f) {
                up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            }

            XMVECTOR right = XMVector3Cross(up, forward);
            right = XMVector3Normalize(right);
            up = XMVector3Cross(forward, right);
            up = XMVector3Normalize(up);

            // Generate ring vertices (DON'T use segments + 1, just segments)
            for (int j = 0; j < segments; j++) {
                float angle = static_cast<float>(j) * 2 * XM_PI / static_cast<float>(segments);
                XMVECTOR offset = XMVectorScale(right, radius * cosf(angle)) +
                    XMVectorScale(up, radius * sinf(angle));
                XMVECTOR vertexPos = XMVectorAdd(current, offset);

                D3D12SimpleVertex vertex;
                XMStoreFloat3(&vertex.position, vertexPos);

                // Color based on position along path
                float t = static_cast<float>(i) / (pathPoints.size() - 1);
                vertex.color = XMFLOAT4(1.0f - t, 0.5f, t, 0.8f);

                outVertices.push_back(vertex);
            }
        }

        // Generate indices connecting the rings
        for (size_t i = 0; i < pathPoints.size() - 1; i++) {
            for (int j = 0; j < segments; j++) {
                // Calculate actual vertex indices
                UINT curr = baseVertexIndex + static_cast<UINT>(i * segments + j);
                UINT next = baseVertexIndex + static_cast<UINT>((i + 1) * segments + j);
                UINT currNext = baseVertexIndex + static_cast<UINT>(i * segments + ((j + 1) % segments));
                UINT nextNext = baseVertexIndex + static_cast<UINT>((i + 1) * segments + ((j + 1) % segments));

                // First triangle
                outIndices.push_back(curr);
                outIndices.push_back(next);
                outIndices.push_back(currNext);

                // Second triangle
                outIndices.push_back(currNext);
                outIndices.push_back(next);
                outIndices.push_back(nextNext);
            }
        }

        info.indexCount = static_cast<UINT>(outIndices.size()) - info.startIndex;
        tubeInfos.push_back(info);

        return tubeInfos;
    }

    //==================================================================================================
	// Buffer Management
	//==================================================================================================
    void D3D12Hook::createVertexBuffer(const std::vector<D3D12SimpleVertex>& vertices,
        ID3D12Resource** vertexBuffer,
        D3D12_VERTEX_BUFFER_VIEW* vertexBufferView) const
    {
        if (vertices.empty() || !_pDevice) {
            return;
        }

        const UINT vertexBufferSize = static_cast<UINT>(sizeof(D3D12SimpleVertex) * vertices.size());

        // Create vertex buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = vertexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        const HRESULT hr = _pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(vertexBuffer));

        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::createVertexBuffer: Failed to create vertex buffer");
            return;
        }

        // Copy vertex data to buffer
        UINT8* pVertexDataBegin;
        constexpr D3D12_RANGE readRange = { 0, 0 };
        (*vertexBuffer)->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
        (*vertexBuffer)->Unmap(0, nullptr);

        // Initialize vertex buffer view
        vertexBufferView->BufferLocation = (*vertexBuffer)->GetGPUVirtualAddress();
        vertexBufferView->StrideInBytes = sizeof(D3D12SimpleVertex);
        vertexBufferView->SizeInBytes = vertexBufferSize;
    }

    void D3D12Hook::createIndexBuffer(const std::vector<UINT>& indices,
        ID3D12Resource** indexBuffer,
        D3D12_INDEX_BUFFER_VIEW* indexBufferView) const
    {
        if (indices.empty() || !_pDevice) {
            return;
        }

        const UINT indexBufferSize = static_cast<UINT>(sizeof(UINT) * indices.size());

        // Create index buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = indexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        const HRESULT hr = _pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(indexBuffer));

        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::createIndexBuffer: Failed to create index buffer");
            return;
        }

        // Copy index data to buffer
        UINT8* pIndexDataBegin;
        constexpr D3D12_RANGE readRange = { 0, 0 };
        (*indexBuffer)->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
        memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
        (*indexBuffer)->Unmap(0, nullptr);

        // Initialize index buffer view
        indexBufferView->BufferLocation = (*indexBuffer)->GetGPUVirtualAddress();
        indexBufferView->Format = DXGI_FORMAT_R32_UINT;
        indexBufferView->SizeInBytes = indexBufferSize;
    }

    //==================================================================================================
	// Depth Buffer Management
	//==================================================================================================
    void D3D12Hook::trackDepthDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
        std::lock_guard lock(_depthBufferMutex);

        if (!_seenDepthDescriptors.contains(descriptor.ptr)) {
            _seenDepthDescriptors.insert(descriptor.ptr);
            _trackedDepthDescriptors.push_back(descriptor);

            MessageHandler::logDebug("D3D12Hook::trackDepthDescriptor: Added descriptor #%zu: 0x%llX",
                _trackedDepthDescriptors.size(), descriptor.ptr);

            // If this is the first descriptor and we're using depth, select it
            if (_trackedDepthDescriptors.size() == 1 && _useDetectedDepthBuffer) {
                _currentDepthDescriptorIndex = 0;
            }
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12Hook::getCurrentTrackedDepthDescriptor() {
        std::lock_guard lock(_depthBufferMutex);

        if (!_useDetectedDepthBuffer || _trackedDepthDescriptors.empty() ||
            _currentDepthDescriptorIndex < 0 ||
            _currentDepthDescriptorIndex >= _trackedDepthDescriptors.size()) {
            return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
        }

        return _trackedDepthDescriptors[_currentDepthDescriptorIndex];
    }

    DepthResourceInfo D3D12Hook::getDepthResourceInfo(D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
        //std::lock_guard<std::mutex> lock(_depthBufferMutex);
        if (_depthResourceMap.contains(descriptor.ptr)) {
            return _depthResourceMap[descriptor.ptr];
        }
        return {}; // Return an empty struct if not found
    }

    void D3D12Hook::cycleDepthBuffer() {
        // Lock the mutex for safe access to tracked descriptors
        
        lock_guard lock(_depthBufferMutex);

        if (_trackedDepthDescriptors.empty()) {
            MessageHandler::logDebug("D3D12Hook::cycleDepthBuffer: No tracked depth descriptors yet!");
            MessageHandler::addNotification("No depth buffers detected yet");
            return;
        }

        // Cycle to the next available descriptor
        _currentDepthDescriptorIndex = (_currentDepthDescriptorIndex + 1) % _trackedDepthDescriptors.size();

        // Get the handle of the currently selected descriptor
        const D3D12_CPU_DESCRIPTOR_HANDLE& selectedDescriptor = _trackedDepthDescriptors[_currentDepthDescriptorIndex];

        MessageHandler::logDebug("Cycled to depth descriptor %d/%zu: 0x%llX",
            _currentDepthDescriptorIndex + 1,
            _trackedDepthDescriptors.size(),
            selectedDescriptor.ptr);

        MessageHandler::addNotification("Cycled to depth descriptor " + 
			to_string(_currentDepthDescriptorIndex + 1) + "/" +
			to_string(_trackedDepthDescriptors.size()));

        // --- Look up and print resource info ---
        DepthResourceInfo info = getDepthResourceInfo(selectedDescriptor);
        if (info.resource) {
            // If the resource was found in our map, print its details including resource address
            MessageHandler::logDebug("  -> Resource Details: %llu x %u, Format: %s, Address: %p",
                info.desc.Width,
                info.desc.Height,
                formatToString(info.desc.Format),
                info.resource);
        }
        else {
            // This can happen if the descriptor was seen in ClearDepthStencilView
            // before its creation was hooked by CreateDepthStencilView.
            MessageHandler::logDebug("  -> Resource details not yet mapped for this descriptor.");
        }

        // Add a notification for the game overlay/UI
        MessageHandler::logDebug("Depth Buffer: Tracked #%d/%d",
            _currentDepthDescriptorIndex + 1,
            static_cast<int>(_trackedDepthDescriptors.size()));
    }

    void D3D12Hook::toggleDepthBufferUsage() {
        // Lock the mutex to ensure this state change is atomic
        std::lock_guard lock(_depthBufferMutex);

        _useDetectedDepthBuffer = !_useDetectedDepthBuffer;
        MessageHandler::logDebug("D3D12Hook::toggleDepthBufferUsage: Depth buffer usage is now %s",
            _useDetectedDepthBuffer ? "ON" : "OFF");

        if (_useDetectedDepthBuffer) {
            _currentDepthDescriptorIndex = -1; // Reset index upon enabling

            if (!_trackedDepthDescriptors.empty()) {
                // Find the best-matching depth buffer (usually the one with the same resolution as the window)
                for (size_t i = 0; i < _trackedDepthDescriptors.size(); ++i) {
                    DepthResourceInfo info = getDepthResourceInfo(_trackedDepthDescriptors[i]);
                    if (info.resource && info.desc.Width == instance()._viewPort.Width && info.desc.Height == instance()._viewPort.Height) {
                        _currentDepthDescriptorIndex = static_cast<int>(i);
                        MessageHandler::logDebug("Automatically selected matching depth buffer #%d", _currentDepthDescriptorIndex + 1);
                        break;
                    }
                }
                // If no perfect match, select the first one to be safe.
                if (_currentDepthDescriptorIndex == -1) {
                    _currentDepthDescriptorIndex = 0;
                }
            }
        }
    }

    bool D3D12Hook::isUsingDepthBuffer() const {
        return _useDetectedDepthBuffer;
    }

    void D3D12Hook::cleanupDepthBuffers() {
        lock_guard lock(_depthBufferMutex);

        // Release all depth buffer resources
        _trackedDepthDescriptors.clear();
        _seenDepthDescriptors.clear();
        _currentDepthDescriptorIndex = -1;
        _useDetectedDepthBuffer = false;
        _depthResourceMap.clear();
        _loggedDepthDescriptors.clear();
        _activeHandlesThisFrame.clear();
		MessageHandler::logDebug("D3D12Hook::cleanupDepthBuffers: Cleared all tracked depth buffers");
    }

    //==================================================================================================
	// Resource Cleanup
	//==================================================================================================

    void D3D12Hook::resetState() {
        std::lock_guard lock(_resourceMutex);

        MessageHandler::logDebug("D3D12Hook::resetState: Beginning state reset");

        // ==== STEP 1: Close command list if recording ====
        if (_pCommandList) {
            _pCommandList->Close();
        }

        // ==== STEP 2: Synchronize GPU ONCE ====
        synchroniseGPU();

        //// ==== STEP 3: Clean up frame temp resources ====
        //for (auto& resources : _frameTempResources) {
        //    for (auto* buffer : resources.constantBuffers) {
        //        if (buffer) {
        //            buffer->Release();
        //        }
        //    }
        //    resources.constantBuffers.clear();
        //    resources.fenceValue = 0;
        //}

        // ==== STEP 4: Release command objects ====
        if (_pCommandList) {
            _pCommandList->Release();
            _pCommandList = nullptr;
        }

        if (_pCommandAllocator) {
            _pCommandAllocator->Release();
            _pCommandAllocator = nullptr;
        }

        // ==== STEP 5: Release per-frame constant buffers ====
        for (auto& cb : _perFrameConstantBuffers) {
            if (cb.buffer) {
                if (cb.mappedData) {
                    cb.buffer->Unmap(0, nullptr);
                    cb.mappedData = nullptr;
                }
                cb.buffer->Release();
                cb.buffer = nullptr;
            }
        }
        _perFrameConstantBuffers.clear();

        // ==== STEP 6: Release render targets - ONLY from frameContexts ====
        for (auto& context : _frameContexts) {
            if (context.commandAllocator) {
                context.commandAllocator->Release();
                context.commandAllocator = nullptr;
            }
            if (context.renderTarget) {
                context.renderTarget->Release();  // ONLY release here
                context.renderTarget = nullptr;
            }
            context.rtvHandle = {};
        }
        _frameContexts.clear();
        _frameContexts.empty() ? MessageHandler::logDebug("Frame contexts cleared") 
			: MessageHandler::logError("Frame contexts not fully cleared!");

        // ==== STEP 7: Release descriptor heaps ====
        if (_pRTVHeap) {
            _pRTVHeap->Release();
            _pRTVHeap = nullptr;
        }

        if (_pCBVHeap) {
            _pCBVHeap->Release();
            _pCBVHeap = nullptr;
        }

        // ==== STEP 9: Clear path visualization data ====
        _pathInfos.clear();
        _nodePositions.clear();
        _pathResourcesCreated = false;

        // Reset all counts
        _pathTubeIndexCount = 0;
        _pathTubeVertexCount = 0;
        _sphereIndexCount = 0;
        _arrowHeadIndexCount = 0;
        _arrowShaftIndexCount = 0;
        _directionIndicatorVertexCount = 0;
        _interpolatedDirectionsVertexCount = 0;

        // ==== STEP 10: Reset state flags ====
        _frameContextsInitialized = false;
        _currentBackBuffer = 0;
        _rtvDescriptorSize = 0;
        _currentFrameIndex = 0;
        _frameBufferCount = 0;
        _currentCBVIndex = 0;
        _cbvDescriptorSize = 0;

        // ==== STEP 11: Clear depth buffer tracking ====
		cleanupDepthBuffers();

        // ==== STEP 12: Reset initialization flags ====
        _needsInitialisation.store(false, std::memory_order_release);
        _resourcesNeedUpdate.store(false, std::memory_order_release);
        _isChangingMode.store(false, std::memory_order_release);

        // ==== STEP 13: Reset other state flags ====
        _currentFrameUsingDepth = false;
        _inOurRendering = false;
        _isRenderingPaths = false;

        MessageHandler::logDebug("D3D12Hook::resetState: State reset complete");
    }

    void D3D12Hook::cleanUp() {
        MessageHandler::logDebug("D3D12Hook::cleanUp: Starting cleanup");

        std::lock_guard lock(_resourceMutex);

        cleanupFrameContexts();

        cleanupAllResources();

        if (_hooked) {
            unhook();
        }

        _isInitialized = false;
        _frameContextsInitialized = false;
        _needsInitialisation = false;
        _is_phase_1 = true;

        MessageHandler::logDebug("D3D12Hook::cleanUp: Cleanup complete");
    }

    void D3D12Hook::cleanupAllResources() {

        // Wait for GPU to finish if we have a fence
        synchroniseGPU();

        // Instead, just null them out
        for (UINT i = 0; i < _frameBufferCount; i++) {
        }

        // Clean up descriptor heaps
        if (_pRTVHeap) {
            _pRTVHeap->Release();
            _pRTVHeap = nullptr;
        }

        // Clean up path visualization resources
        if (_pPathTubeVertexBuffer) {
            _pPathTubeVertexBuffer->Release();
            _pPathTubeVertexBuffer = nullptr;
        }
        if (_pPathTubeIndexBuffer) {
            _pPathTubeIndexBuffer->Release();
            _pPathTubeIndexBuffer = nullptr;
        }
        if (_pLookAtTargetVertexBuffer) {
            _pLookAtTargetVertexBuffer->Release();
            _pLookAtTargetVertexBuffer = nullptr;
        }
        if (_pLookAtTargetIndexBuffer) {
            _pLookAtTargetIndexBuffer->Release();
            _pLookAtTargetIndexBuffer = nullptr;
        }

        if (_pArrowHeadVertexBuffer) {
            _pArrowHeadVertexBuffer->Release();
            _pArrowHeadVertexBuffer = nullptr;
        }
        if (_pArrowHeadIndexBuffer) {
            _pArrowHeadIndexBuffer->Release();
            _pArrowHeadIndexBuffer = nullptr;
        }
        if (_pArrowShaftVertexBuffer) {
            _pArrowShaftVertexBuffer->Release();
            _pArrowShaftVertexBuffer = nullptr;
        }
        if (_pArrowShaftIndexBuffer) {
            _pArrowShaftIndexBuffer->Release();
            _pArrowShaftIndexBuffer = nullptr;
        }
        if (_pPipelineStateWithDepth) {
            _pPipelineStateWithDepth->Release();
            _pPipelineStateWithDepth = nullptr;
        }
        if (_pPipelineStateWithReversedDepth) {
            _pPipelineStateWithReversedDepth->Release();
            _pPipelineStateWithReversedDepth = nullptr;
        }
        if (_directionIndicatorVertexBuffer) {
            _directionIndicatorVertexBuffer->Release();
            _directionIndicatorVertexBuffer = nullptr;
        }
        if (_interpolatedDirectionsVertexBuffer) {
            _interpolatedDirectionsVertexBuffer->Release();
            _interpolatedDirectionsVertexBuffer = nullptr;
        }

        // Clean up synchronization objects
        if (_pFence) {
            _pFence->Release();
            _pFence = nullptr;
        }
        if (_fenceEvent) {
            CloseHandle(_fenceEvent);
            _fenceEvent = nullptr;
        }

        // Don't release these as they are owned by the game
        _pDevice = nullptr;
        _pSwapChain = nullptr;
        _pCommandQueue = nullptr;

        // Reset state
        _pathResourcesCreated = false;
        _pathInfos.clear();
        _nodePositions.clear();

        cleanupDepthBuffers();

        // Reset the counts
        _directionIndicatorVertexCount = 0;
        _interpolatedDirectionsVertexCount = 0;
    }

    void D3D12Hook::cleanupFrameContexts() {
        // Clean up per-frame constant buffers
        for (auto& cb : _perFrameConstantBuffers) {
            if (cb.buffer) {
                if (cb.mappedData) {
                    cb.buffer->Unmap(0, nullptr);
                    cb.mappedData = nullptr;
                }
                cb.buffer->Release();
                cb.buffer = nullptr;
            }
        }
        _perFrameConstantBuffers.clear();

        for (auto& context : _frameContexts) {
            if (context.commandAllocator) {
                context.commandAllocator->Release();
                context.commandAllocator = nullptr;
            }
            if (context.renderTarget) {
                context.renderTarget->Release();
                context.renderTarget = nullptr;
            }
        }
        _frameContexts.clear();
    }

    void D3D12Hook::synchroniseGPU() {
        if (!_pCommandQueue || !_pFence || !_fenceEvent) {
            return;
        }

        // Signal and increment the fence value
        const UINT64 fence = _fenceValue;
        HRESULT hr = _pCommandQueue->Signal(_pFence, fence);
        if (FAILED(hr)) {
            MessageHandler::logError("D3D12Hook::synchroniseGPU: Failed to signal fence");
            return;
        }
        _fenceValue++;

        // Wait until the previous frame is finished
        if (_pFence->GetCompletedValue() < fence) {
            hr = _pFence->SetEventOnCompletion(fence, _fenceEvent);
            if (SUCCEEDED(hr)) {
                WaitForSingleObject(_fenceEvent, INFINITE);
            }
            //MessageHandler::logDebug("D3D12Hook::hookedResizeBuffers: GPU synchronization complete");
        }
    }

    void D3D12Hook::waitForFenceValue(UINT64 value)
    {
        if (value == 0 || !_pFence || !_fenceEvent) return;
        if (_pFence->GetCompletedValue() >= value) return;
        if (SUCCEEDED(_pFence->SetEventOnCompletion(value, _fenceEvent)))
        {
            WaitForSingleObject(_fenceEvent, INFINITE);
        }
    }

    //==================================================================================================
	// State Management & Getters/Setters
	//==================================================================================================
    void D3D12Hook::setVisualisation(const bool enabled) {
        _visualisationEnabled = enabled;
        MessageHandler::logDebug("D3D12Hook::setVisualisation: Visualization is now %s",
            _visualisationEnabled ? "ON" : "OFF");
    }

    bool D3D12Hook::isVisualisationEnabled() const {
        return _visualisationEnabled;
    }

    void D3D12Hook::setRenderPathOnly(const bool renderPathOnly) {
        _renderSelectedPathOnly = renderPathOnly;
        MessageHandler::logDebug("D3D12Hook::setRenderPathOnly: Render selected path only is now %s",
            _renderSelectedPathOnly ? "ON" : "OFF");
    }

    bool D3D12Hook::isRenderPathOnly() const {
        return _renderSelectedPathOnly;
    }

    void D3D12Hook::markResourcesForUpdate() {
        // First, set the flag to indicate resources need updating 
        _resourcesNeedUpdate.store(true, std::memory_order_release);
    }

    float D3D12Hook::getAspectRatio() const {
        if (_viewPort.Width == 0) {
            return ASPECT_RATIO; // Prevent division by zero
        }
        return (_viewPort.Width / _viewPort.Height);
	}

    //==================================================================================================
	// Utility Functions
	//==================================================================================================
    XMFLOAT4 D3D12Hook::convertHSVtoRGB(const float hue, const float saturation, const float value, const float alpha) {
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
        return {r + m, g + m, b + m, alpha};
    }

    // Helper function to convert DXGI_FORMAT to a string
    const char* D3D12Hook::formatToString(DXGI_FORMAT format)
    {
        switch (format)
        {
            // --- 32-bit Depth, 8-bit Stencil ---
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return "D32_FLOAT_S8X24_UINT / R32G8X24_TYPELESS";

            // --- 32-bit Depth ---
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_TYPELESS:
            return "D32_FLOAT / R32_TYPELESS";

            // --- 24-bit Depth, 8-bit Stencil ---
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
            return "D24_UNORM_S8_UINT / R24G8_TYPELESS";

            // --- 16-bit Depth ---
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_TYPELESS:
            return "D16_UNORM / R16_TYPELESS";

            // --- Other ---
        case DXGI_FORMAT_UNKNOWN:
            return "UNKNOWN";
        default:
            return "Other Format";
        }
    }

	void D3D12Hook::clearDepthStencilLogging(ID3D12GraphicsCommandList* pCommandList,
                                             const D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
                                             const D3D12_CLEAR_FLAGS ClearFlags,
                                             const FLOAT Depth,
                                             const UINT8 Stencil,
                                             const UINT NumRects,
                                             const D3D12_RECT* pRects)
    {
        if (D3DLOGGING && VERBOSE)
        {
            static int totalCalls = 0;
            totalCalls++;

            // FIXED: Track unique descriptors on EVERY call, not just when logging
            static std::unordered_set<UINT64> uniqueDescriptors;
            uniqueDescriptors.insert(DepthStencilView.ptr);

            // OPTIONAL: Also track depth values on every call
            static std::unordered_map<float, int> depthValueCounts;
            depthValueCounts[Depth]++;

            // Log every 1000 calls with detailed parameter information
            if (totalCalls % 1000 == 0) {
                MessageHandler::logDebug("Camera dll::[DEBUG] ClearDepthStencilView call #%d:", totalCalls);
                MessageHandler::logDebug("  CommandList: %p", pCommandList);
                MessageHandler::logDebug("  Descriptor Handle: 0x%llX", DepthStencilView.ptr);
                MessageHandler::logDebug("  Clear Flags: 0x%X", ClearFlags);

                // Decode clear flags
                std::string flagsStr = "";
                if (ClearFlags & D3D12_CLEAR_FLAG_DEPTH) {
                    flagsStr += "DEPTH ";
                }
                if (ClearFlags & D3D12_CLEAR_FLAG_STENCIL) {
                    flagsStr += "STENCIL ";
                }
                if (flagsStr.empty()) {
                    flagsStr = "NONE";
                }
                MessageHandler::logDebug("  Clear Flags Decoded: %s", flagsStr.c_str());

                MessageHandler::logDebug("  Depth Value: %.6f", Depth);
                MessageHandler::logDebug("  Stencil Value: %u", Stencil);
                MessageHandler::logDebug("  NumRects: %u", NumRects);

                // Log rectangle information if provided
                if (NumRects > 0 && pRects != nullptr) {
                    MessageHandler::logDebug("  Rectangles:");
                    for (UINT i = 0; i < NumRects && i < 5; i++) { // Limit to first 5 rects
                        MessageHandler::logDebug("    Rect[%u]: left=%d, top=%d, right=%d, bottom=%d (size: %dx%d)",
                            i,
                            pRects[i].left,
                            pRects[i].top,
                            pRects[i].right,
                            pRects[i].bottom,
                            pRects[i].right - pRects[i].left,
                            pRects[i].bottom - pRects[i].top);
                    }
                    if (NumRects > 5) {
                        MessageHandler::logDebug("    ... and %u more rectangles", NumRects - 5);
                    }
                }
                else {
                    MessageHandler::logDebug("  Rectangles: Full clear (no rects specified)");
                }

                MessageHandler::logDebug("  ---");
            }

            // Create a summary every 10000 calls
            if (totalCalls % 10000 == 0) {
                MessageHandler::logDebug("Camera dll::[DEBUG] ClearDepthStencilView Summary after %d calls:", totalCalls);
                MessageHandler::logDebug("  Unique descriptors seen so far: %zu", uniqueDescriptors.size());
                MessageHandler::logDebug("  All unique descriptors:");
                for (const auto& desc : uniqueDescriptors) {
                    MessageHandler::logDebug("    - 0x%llX", desc);
                }

                MessageHandler::logDebug("  Common depth values:");
                for (const auto& [depth, count] : depthValueCounts) {
                    MessageHandler::logDebug("    Depth %.6f: %d times", depth, count);
                }
            }
        }
    }
} // namespace IGCS