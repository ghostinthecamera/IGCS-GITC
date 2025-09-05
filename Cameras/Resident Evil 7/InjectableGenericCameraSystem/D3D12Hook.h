#pragma once
#include "stdafx.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "GameConstants.h"
#include "CameraPath.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Forward declarations
class CameraPath;
class CameraNode;

namespace IGCS {

    //==================================================================================================
    // Data Structures
    //==================================================================================================

    /**
     * @brief D3D12 vertex structure with position and color
     */
    struct D3D12SimpleVertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    /**
     * @brief D3D12 constant buffer for shader transformation matrix
     */
    struct D3D12SimpleConstantBuffer {
        DirectX::XMMATRIX worldViewProjection;
    };

    /**
     * @brief Information about a depth buffer resource
     */
    struct DepthResourceInfo {
        ID3D12Resource* resource = nullptr;
        D3D12_RESOURCE_DESC desc{};
    };

    //==================================================================================================
    // D3D12Hook Class
    //==================================================================================================

    class D3D12Hook {
    public:
        //==============================================================================================
        // Type Definitions
        //==============================================================================================
        typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
        typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height,
            DXGI_FORMAT NewFormat, UINT SwapChainFlags);
        typedef HRESULT(WINAPI* CreateSwapChain_t)(IDXGIFactory4* pFactory, IUnknown* pDevice, HWND hWnd,
            const DXGI_SWAP_CHAIN_DESC* pDesc,
            const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
            IDXGIOutput* pRestrictToOutput, IDXGISwapChain** ppSwapChain);
        typedef void(WINAPI* ExecuteCommandLists_t)(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists,
            ID3D12CommandList* const* ppCommandLists);
        typedef void(WINAPI* OMSetRenderTargets_t)(ID3D12GraphicsCommandList* pCommandList, UINT NumRenderTargetDescriptors,
            const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors,
            BOOL RTsSingleHandleToDescriptorRange,
            const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor);
        typedef void(WINAPI* CreateDepthStencilView_t)(ID3D12Device* pDevice, ID3D12Resource* pResource,
            const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
            D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);
        typedef void(WINAPI* ClearDepthStencilView_t)(ID3D12GraphicsCommandList* pCommandList,
            D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
            D3D12_CLEAR_FLAGS ClearFlags, FLOAT Depth, UINT8 Stencil,
            UINT NumRects, const D3D12_RECT* pRects);

        //==============================================================================================
        // Singleton Pattern
        //==============================================================================================
        static D3D12Hook& instance();

        //==============================================================================================
        // Core Lifecycle Methods
        //==============================================================================================
        bool initialise();
        void cleanUp();
        bool unhook();
        [[nodiscard]] bool isHooked() const { return _hooked; }
        [[nodiscard]] bool isFullyInitialised() const;
        [[nodiscard]] bool needsInitialisation() const { return _needsInitialisation; }

        //==============================================================================================
        // Initialization Management
        //==============================================================================================
        void queueInitialisation();
        void performQueuedInitialisation();
        void markResourcesForUpdate();

        //==============================================================================================
        // Visualization Control
        //==============================================================================================
        void setVisualisation(bool enabled);
        [[nodiscard]] bool isVisualisationEnabled() const;
        void setRenderPathOnly(bool renderPathOnly);
        [[nodiscard]] bool isRenderPathOnly() const;

        //==============================================================================================
        // Depth Buffer Management
        //==============================================================================================
        void cycleDepthBuffer();
        [[nodiscard]] bool isUsingDepthBuffer() const;
        void toggleDepthBufferUsage();
        DepthResourceInfo getDepthResourceInfo(D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

        //==============================================================================================
        // Path Visualization
        //==============================================================================================
        void safeInterpolationModeChange();
        void createPathVisualisation();
        void cleanupAllResources();

        //==============================================================================================
        // Look-at Target Visualization
        //==============================================================================================
        void createLookAtTargetSphere(float radius = 0.5f,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.8f));
        void renderFreeCameraLookAtTarget(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void renderPathLookAtTarget(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);

        //==============================================================================================
        // Hooked Function Implementations (Static)
        //==============================================================================================
        static HRESULT WINAPI hookedPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT WINAPI hookedResizeBuffers(IDXGISwapChain3* pSwapChain, UINT BufferCount,
            UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
        static HRESULT WINAPI hookedCreateSwapChain(IDXGIFactory4* pFactory, IUnknown* pDevice, HWND hWnd,
            const DXGI_SWAP_CHAIN_DESC* pDesc,
            const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
            IDXGIOutput* pRestrictToOutput, IDXGISwapChain** ppSwapChain);
        static void WINAPI hookedExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists,
            ID3D12CommandList* const* ppCommandLists);
        static void WINAPI hookedOMSetRenderTargets(ID3D12GraphicsCommandList* pCommandList, UINT NumRenderTargetDescriptors,
            const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors,
            BOOL RTsSingleHandleToDescriptorRange,
            const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor);
        static void WINAPI hookedCreateDepthStencilView(ID3D12Device* pDevice, ID3D12Resource* pResource,
            const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc,
            D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);
        static void WINAPI hookedClearDepthStencilView(ID3D12GraphicsCommandList* pCommandList,
            D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
            D3D12_CLEAR_FLAGS ClearFlags, FLOAT Depth, UINT8 Stencil,
            UINT NumRects, const D3D12_RECT* pRects);

        //==============================================================================================
        // State Management
        //==============================================================================================
        void resetState();

    private:
        //==============================================================================================
        // Constructor & Destructor (Private for Singleton)
        //==============================================================================================
        D3D12Hook();
        ~D3D12Hook();
        D3D12Hook(const D3D12Hook&) = delete;
        D3D12Hook& operator=(const D3D12Hook&) = delete;

        //==============================================================================================
        // Internal Data Structures
        //==============================================================================================

        struct PathInfo {
            std::string name;
            DirectX::XMFLOAT4 color;
            UINT interpStartIndex;
            UINT interpVertexCount;
            UINT nodeStartIndex{};
            UINT nodeCount{};
            UINT directionStartIndex{};
            UINT directionVertexCount{};
            std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>> pathDirections{};
        };

        struct TubeInfo {
            UINT startIndex;
            UINT indexCount;
            DirectX::XMFLOAT4 color;
        };

        struct InterpSample {
            DirectX::XMVECTOR rotation;
            DirectX::XMFLOAT3 position;
            float fov;
        };

        struct FrameContext {
            ID3D12CommandAllocator* commandAllocator = nullptr;
            ID3D12Resource* renderTarget = nullptr;
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
            UINT64 fenceValue = 0; // Add this line
        };

        struct PerFrameConstantBuffer {
            ID3D12Resource* buffer = nullptr;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
            UINT8* mappedData = nullptr;
        };

        struct FrameTempResources {
            std::vector<ID3D12Resource*> constantBuffers;
            UINT64 fenceValue = 0;
        };

        struct DepthBufferInfo {
            ID3D12Resource* resource{ nullptr };
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};
            D3D12_RESOURCE_DESC desc{};
            bool isActive{ false };
        };

        class RenderingScope {
        public:
            RenderingScope(D3D12Hook& hook) : _hook(hook) { _hook._inOurRendering = true; }
            ~RenderingScope() { _hook._inOurRendering = false; }
        private:
            D3D12Hook& _hook;
        };

        //==============================================================================================
        // Initialization & Setup
        //==============================================================================================
        static bool initialiseRenderingResources();
        void initialiseFrameContexts(IDXGISwapChain3* pSwapChain);
        bool createRootSignature();
        bool createShadersAndPipelineState();
        static ID3DBlob* compileShader(const std::string& shaderCode, const std::string& target,
            const std::string& entryPoint);

        //==============================================================================================
        // Hook Management
        //==============================================================================================
        void hookDeviceMethods();
        static void hookCommandListMethods(ID3D12GraphicsCommandList* pCommandList);

        //==============================================================================================
        // Rendering Pipeline
        //==============================================================================================
        void renderWithResourceBarriers(IDXGISwapChain3* pSwapChain);
        void renderPaths(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void setupCameraMatrices();
        D3D12_VIEWPORT getFullViewport();
        ID3D12PipelineState* selectPipelineState(bool wireframe, bool useDepth) const;
        void updateConstantBuffer(const DirectX::XMMATRIX& worldViewProj);
        void updateConstantBuffer(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);

        //==============================================================================================
        // Path Rendering
        //==============================================================================================
        void renderPathTubes(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void renderDirectionArrows(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void renderNodeSpheres(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void renderArrow(const XMFLOAT3& position,
            const XMVECTOR& direction,
            const float length,
            const XMFLOAT4& color,
            const XMMATRIX& viewMatrix,
            const XMMATRIX& projMatrix);

        //==============================================================================================
        // Path Processing
        //==============================================================================================
        void processAllPaths(const std::unordered_map<std::string, CameraPath>& paths,
            std::vector<D3D12SimpleVertex>& directionIndicatorVertices,
            std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
            std::vector<D3D12SimpleVertex>& tubeMeshVertices,
            std::vector<UINT>& tubeMeshIndices,
            std::vector<TubeInfo>& pathTubeInfos,
            const DirectX::XMFLOAT4& nodeColor,
            const DirectX::XMFLOAT4& directionColor);

        void processPathNodes(const CameraPath& path, size_t nodeCount,
            std::vector<D3D12SimpleVertex>& directionIndicatorVertices,
            const DirectX::XMFLOAT4& nodeColor,
            const DirectX::XMFLOAT4& directionColor);

        std::vector<InterpSample> generateInterpolatedSamples(const CameraPath& path, size_t nodeCount);

        void createInterpolatedDirections(const std::vector<InterpSample>& interpSamples,
            std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
            UINT nodeStartIndex, UINT nodeCount) const;

        void storePathDirections(PathInfo& pathInfo,
            const std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
            size_t startIndex, size_t count);

        void createPathTubes(const std::vector<DirectX::XMFLOAT3>& points,
            std::vector<D3D12SimpleVertex>& tubeMeshVertices,
            std::vector<UINT>& tubeMeshIndices,
            const DirectX::XMFLOAT4& color) const;

        void createPathBuffers(const std::vector<D3D12SimpleVertex>& directionIndicatorVertices,
            const std::vector<D3D12SimpleVertex>& interpolatedDirectionVertices,
            const std::vector<D3D12SimpleVertex>& tubeMeshVertices,
            const std::vector<UINT>& tubeMeshIndices,
            const std::vector<TubeInfo>& pathTubeInfos);

        //==============================================================================================
        // Geometry Creation
        //==============================================================================================
        void createNodeSphere(float radius, const DirectX::XMFLOAT4& color);
        void createSolidColorSphere(float radius, const DirectX::XMFLOAT4& color);
        void createArrowGeometry();
        void createArrowHead();
        void createArrowShaft();
        static std::vector<TubeInfo> generateTubeMesh(const std::vector<DirectX::XMFLOAT3>& pathPoints,
            float radius, int segments,
            std::vector<D3D12SimpleVertex>& outVertices,
            std::vector<UINT>& outIndices);

        //==============================================================================================
        // Buffer Management
        //==============================================================================================
        void createVertexBuffer(const std::vector<D3D12SimpleVertex>& vertices, ID3D12Resource** vertexBuffer,
            D3D12_VERTEX_BUFFER_VIEW* vertexBufferView) const;
        void createIndexBuffer(const std::vector<UINT>& indices, ID3D12Resource** indexBuffer,
            D3D12_INDEX_BUFFER_VIEW* indexBufferView) const;

        //==============================================================================================
        // Depth Buffer Management
        //==============================================================================================
        void trackDepthDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE getCurrentTrackedDepthDescriptor();
        bool hasTrackedDepthDescriptors() const { return !_trackedDepthDescriptors.empty(); }
        void cleanupDepthBuffers();

        //==============================================================================================
        // Resource Cleanup
        //==============================================================================================
        void synchroniseGPU();
        void waitForFenceValue(UINT64 value);
        void cleanupFrameContexts();
        void cleanupFrameTempResources(UINT frameIndex);

        //==============================================================================================
        // Utility Functions
        //==============================================================================================
        static DirectX::XMFLOAT4 convertHSVtoRGB(float hue, float saturation, float value, float alpha);
        static const char* formatToString(DXGI_FORMAT format);
        void clearDepthStencilLogging(ID3D12GraphicsCommandList* pCommandList,
            D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
            D3D12_CLEAR_FLAGS ClearFlags, FLOAT Depth, UINT8 Stencil,
            UINT NumRects, const D3D12_RECT* pRects);

        //==============================================================================================
        // Static Hook Function Pointers
        //==============================================================================================
        static Present_t _originalPresent;
        static ResizeBuffers_t _originalResizeBuffers;
        static CreateSwapChain_t _originalCreateSwapChain;
        static ExecuteCommandLists_t _originalExecuteCommandLists;
        static OMSetRenderTargets_t _originalOMSetRenderTargets;
        static CreateDepthStencilView_t _originalCreateDepthStencilView;
        static ClearDepthStencilView_t _originalClearDepthStencilView;

        //==============================================================================================
        // Static VTable Pointers
        //==============================================================================================
        static void** s_swapchain_vtable;
        static void** s_factory_vtable;
        static void** s_command_queue_vtable;
        static void** s_command_list_vtable;
        static UINT s_command_queue_offset;

        //==============================================================================================
        // State Flags
        //==============================================================================================
        bool _isInitialized{ false };
        bool _frameContextsInitialized{ false };
        bool _hooked{ false };
        bool _is_phase_1{ true };
        bool _deviceMethodsHooked{ false };
        bool _visualisationEnabled{ false };
        bool _renderSelectedPathOnly{ false };
        bool _useDetectedDepthBuffer{ false };
        bool _useReversedDepth{ true };
        bool _currentFrameUsingDepth{ false };
        bool _inOurRendering{ false };
        bool _isRenderingPaths{ false };
        bool _pathResourcesCreated{ false };

        std::atomic<bool> _needsInitialisation{ false };
        std::atomic<bool> _resourcesNeedUpdate{ false };
        std::atomic<bool> _isChangingMode{ false };

        //==============================================================================================
        // D3D12 Core Resources
        //==============================================================================================
        ID3D12Device* _pDevice{ nullptr };
        IDXGISwapChain3* _pSwapChain{ nullptr };
        ID3D12CommandQueue* _pCommandQueue{ nullptr };
        ID3D12CommandAllocator* _pCommandAllocator{ nullptr };
        ID3D12GraphicsCommandList* _pCommandList{ nullptr };

        //==============================================================================================
        // Render Target Resources
        //==============================================================================================
        ID3D12DescriptorHeap* _pRTVHeap{ nullptr };
        ID3D12Resource* _pRenderTargets[DXGI_MAX_SWAP_CHAIN_BUFFERS]{ nullptr };
        std::vector<FrameContext> _frameContexts;
        UINT _frameBufferCount{ 0 };
        UINT _currentBackBuffer{ 0 };
        UINT _rtvDescriptorSize{ 0 };

        //==============================================================================================
        // Pipeline State Resources
        //==============================================================================================
        ID3D12RootSignature* _pRootSignature{ nullptr };
        ID3D12PipelineState* _pPipelineState{ nullptr };
        ID3D12PipelineState* _pWireframePipelineState{ nullptr };
        ID3D12PipelineState* _pPipelineStateWithDepth{ nullptr };
        ID3D12PipelineState* _pPipelineStateWithReversedDepth{ nullptr };
        ID3D12PipelineState* _pWireframePipelineStateWithDepth{ nullptr };
        ID3D12PipelineState* _pWireframePipelineStateWithReversedDepth{ nullptr };

        //==============================================================================================
        // Shader Resources
        //==============================================================================================
        ID3DBlob* _pVertexShader{ nullptr };
        ID3DBlob* _pPixelShader{ nullptr };

        //==============================================================================================
        // Constant Buffer Resources
        //==============================================================================================
        ID3D12Resource* _pConstantBuffer{ nullptr };
        ID3D12DescriptorHeap* _pCBVHeap{ nullptr };
        UINT8* _pCbvDataBegin{ nullptr };
        std::vector<PerFrameConstantBuffer> _perFrameConstantBuffers;
        std::vector<FrameTempResources> _frameTempResources;
        UINT _currentFrameIndex{ 0 };
        UINT _cbvDescriptorSize{ 0 };
        UINT _currentCBVIndex{ 0 };

        //==============================================================================================
        // Synchronization
        //==============================================================================================
        ID3D12Fence* _pFence{ nullptr };
        UINT64 _fenceValue{ 0 };
        HANDLE _fenceEvent{ nullptr };
        std::mutex _resourceMutex;
        std::mutex _depthBufferMutex;

        //==============================================================================================
        // Path Visualization Geometry
        //==============================================================================================
        // Tube mesh
        ID3D12Resource* _pPathTubeVertexBuffer{ nullptr };
        ID3D12Resource* _pPathTubeIndexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _pathTubeVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW _pathTubeIndexBufferView{};
        UINT _pathTubeVertexCount{ 0 };
        UINT _pathTubeIndexCount{ 0 };

        // Node spheres
        ID3D12Resource* _pSphereVertexBuffer{ nullptr };
        ID3D12Resource* _pSphereIndexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _sphereVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW _sphereIndexBufferView{};
        UINT _sphereIndexCount{ 0 };

        // Direction indicators
        ID3D12Resource* _directionIndicatorVertexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _directionIndicatorVertexBufferView{};
        UINT _directionIndicatorVertexCount{ 0 };

        ID3D12Resource* _interpolatedDirectionsVertexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _interpolatedDirectionsVertexBufferView{};
        UINT _interpolatedDirectionsVertexCount{ 0 };

        // Arrow geometry
        ID3D12Resource* _pArrowHeadVertexBuffer{ nullptr };
        ID3D12Resource* _pArrowHeadIndexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _arrowHeadVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW _arrowHeadIndexBufferView{};
        UINT _arrowHeadIndexCount{ 0 };

        ID3D12Resource* _pArrowShaftVertexBuffer{ nullptr };
        ID3D12Resource* _pArrowShaftIndexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _arrowShaftVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW _arrowShaftIndexBufferView{};
        UINT _arrowShaftIndexCount{ 0 };

        // Look-at target
        ID3D12Resource* _pLookAtTargetVertexBuffer{ nullptr };
        ID3D12Resource* _pLookAtTargetIndexBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW _lookAtTargetVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW _lookAtTargetIndexBufferView{};
        UINT _lookAtTargetIndexCount{ 0 };
        float _lookAtTargetSize{ 0.5f };

        //==============================================================================================
        // Path Data
        //==============================================================================================
        std::vector<PathInfo> _pathInfos;
        std::vector<DirectX::XMFLOAT3> _nodePositions;

        //==============================================================================================
        // Depth Buffer Management
        //==============================================================================================
        std::unordered_map<UINT64, DepthResourceInfo> _depthResourceMap;
        std::vector<DepthBufferInfo> _detectedDepthBuffers;
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _trackedDepthDescriptors;
        std::unordered_set<UINT64> _seenDepthDescriptors;
        int _currentDepthDescriptorIndex{ -1 };

        //==============================================================================================
        // Rendering State
        //==============================================================================================
        DirectX::XMMATRIX _viewMatrix;
        DirectX::XMMATRIX _projMatrix;
        D3D12_VIEWPORT _viewPort;
        mutable UINT _windowWidth{ 0 };
        mutable UINT _windowHeight{ 0 };

        //==============================================================================================
        // Configuration Constants
        //==============================================================================================
        const float _nodeSize{ 0.05f };
        const float _directionLength{ 0.3f };
        const float _baseDirectionLength{ 0.3f };
        const float _defaultFOV{ DEFAULT_FOV };
        const float _nodeProximityThreshold{ 0.05f };
        const float _tubeDiameter{ 0.03f };
        const float _arrowHeadRadius{ 0.02f };
        const float _arrowHeadLength{ 0.15f };
        const float _arrowShaftRadius{ 0.006f };
        const float _arrowShaftLength{ 1.0f };
        const int _arrowSegments{ 32 };
        const int _tubeSegments{ 14 };
        const int _interpolationSteps{ 30 };
        const int _rotationSampleFrequency{ 5 };
    };

} // namespace IGCS