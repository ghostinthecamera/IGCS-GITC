#pragma once
#include "stdafx.h"
#include <d3d11.h>
#include <dxgi.h>
#include "GameConstants.h"
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <unordered_map>
#include "CameraPath.h"

// Forward declarations
class CameraPath;
class CameraNode;

namespace IGCS {

    /**
     * @brief Basic vertex structure with position and color
     */
    struct SimpleVertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    /**
     * @brief Constant buffer for shader transformation matrix
     */
    struct SimpleConstantBuffer {
        DirectX::XMMATRIX worldViewProjection;
    };

    /**
     * @brief Singleton class to hook into D3D11 rendering pipeline
     */
    class D3DHook {
    public:
        // ==== Singleton pattern ====
        static D3DHook& instance();

        // ==== Lifecycle methods ====
        bool initialize();
        void cleanUp();
        void queueInitialization();
        void performQueuedInitialization();
        bool isFullyInitialized() const;
        bool needsInitialization() const { return _needsInitialization; }

        // ==== Visualization control ====
        void toggleVisibility();
        bool isVisible() const;
        void setVisualization(bool enabled);
        bool isVisualizationEnabled() const;
        bool isPathVisualizationEnabled() const;
        void togglePathVisualization();
        void setRenderPathOnly(bool renderPathOnly);
        bool isRenderPathOnly() const;

        // ==== Depth buffer management ====
        void cycleDepthBuffer();
        bool isUsingDepthBuffer() const;
        void toggleDepthBufferUsage();

        // ==== Path visualization ====
        void safeInterpolationModeChange();
        void createPathVisualization();
        void renderPaths();

        // ==== Resource access ====
        ID3D11Device* getDevice() const;
        ID3D11DeviceContext* getContext() const;

    private:
        // ==== Constructors & assignment operators ====
        D3DHook();
        ~D3DHook();
        D3DHook(const D3DHook&) = delete;
        D3DHook& operator=(const D3DHook&) = delete;

        // ==== Initialization helpers ====
        bool createRenderTargetView();
        bool hookDXGIFunctions();
        /*bool findActiveSwapChain();*/
        static bool initializeRenderingResources();
        std::atomic<bool> _needsInitialization{ false };

        // ==== Resource management ====
        void releaseD3DResources();

        // ==== Hook function types and implementations ====
        typedef HRESULT(STDMETHODCALLTYPE* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
        typedef void(STDMETHODCALLTYPE* OMSetRenderTargets_t)(ID3D11DeviceContext* pContext, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView);

        static HRESULT STDMETHODCALLTYPE hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT STDMETHODCALLTYPE hookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
        static void STDMETHODCALLTYPE hookedOMSetRenderTargets(ID3D11DeviceContext* pContext, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView);

        // ==== Rendering helpers ====
        void createSolidColorSphere(float radius, const DirectX::XMFLOAT4& color);
        void createTubeMesh(std::vector<SimpleVertex>& tubeVertices, std::vector<UINT>& tubeIndices,
            const DirectX::XMFLOAT3& startPoint, const DirectX::XMFLOAT3& endPoint,
            const DirectX::XMFLOAT4& color, float radius, int segments);

        // ==== Arrow visualization ====
        void createArrowGeometry();
        void renderArrow(const DirectX::XMFLOAT3& position, const DirectX::XMVECTOR& direction,
            float length, const DirectX::XMFLOAT4& color);

        // ==== Depth buffer management ====
        void setupOMSetRenderTargetsHook();
        void scanForDepthBuffers();
        void selectAppropriateDepthBuffer();
        void captureActiveDepthBuffer(ID3D11DepthStencilView* pDSV);
        void addDepthStencilView(ID3D11DepthStencilView* pDSV);
        void extractTextureFromDSV(ID3D11DepthStencilView* pDSV);
        bool isDepthFormat(DXGI_FORMAT format);
        DXGI_FORMAT getDepthViewFormat(DXGI_FORMAT resourceFormat);
        void cleanupDepthBufferResources();

        // ==== Path Data Structures ====
        struct PathInfo {
	        std::string name;
            UINT nodeStartIndex{};
            UINT nodeCount{};
            UINT pathStartIndex{};
            UINT pathVertexCount{};
            UINT interpStartIndex{};
            UINT interpVertexCount{};
            UINT directionStartIndex{};
            UINT directionVertexCount{};
            DirectX::XMFLOAT4 pathColor{};
            std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>> pathDirections{};
        };

        struct InterpSample {
            DirectX::XMVECTOR rotation;
        	DirectX::XMFLOAT3 position;
            float fov;
        };

        struct TubeInfo {
            UINT startIndex;
            UINT indexCount;
            DirectX::XMFLOAT4 color;
        };

        // ==== Path Visualization Helpers ====
        void processAllPaths(const std::unordered_map<std::string, CameraPath>& paths,
            std::vector<SimpleVertex>& nodeVertices,
            std::vector<SimpleVertex>& directionIndicatorVertices,
            std::vector<SimpleVertex>& interpolatedDirectionVertices,
            std::vector<SimpleVertex>& tubeMeshVertices,
            std::vector<UINT>& tubeMeshIndices,
            std::vector<TubeInfo>& pathTubeInfos,
            const DirectX::XMFLOAT4& nodeColor,
            const DirectX::XMFLOAT4& directionColor);

        void processPathNodes(const CameraPath& path, size_t nodeCount,
            std::vector<SimpleVertex>& nodeVertices,
            std::vector<SimpleVertex>& directionIndicatorVertices,
            const DirectX::XMFLOAT4& nodeColor,
            const DirectX::XMFLOAT4& directionColor);

        DirectX::XMFLOAT4 convertHSVtoRGB(float hue, float saturation, float value, float alpha);

        std::vector<InterpSample> generateInterpolatedSamples(const CameraPath& path, size_t nodeCount);

        void createInterpolatedDirections(const std::vector<InterpSample>& interpSamples,
            std::vector<SimpleVertex>& interpolatedDirectionVertices);

        void storePathDirections(PathInfo& pathInfo,
            const std::vector<SimpleVertex>& interpolatedDirectionVertices,
            size_t startIndex, size_t count);

        void createPathTubes(const std::vector<DirectX::XMFLOAT3>& points,
            std::vector<SimpleVertex>& tubeMeshVertices,
            std::vector<UINT>& tubeMeshIndices,
            const DirectX::XMFLOAT4& color);

        void createPathBuffers(const std::vector<SimpleVertex>& nodeVertices,
            const std::vector<SimpleVertex>& directionIndicatorVertices,
            const std::vector<SimpleVertex>& interpolatedDirectionVertices,
            const std::vector<SimpleVertex>& tubeMeshVertices,
            const std::vector<UINT>& tubeMeshIndices,
            const std::vector<TubeInfo>& pathTubeInfos);

        // ==== Rendering State Management ====
        void saveRenderState(ID3D11RenderTargetView*& pCurrentRTV,
            ID3D11DepthStencilView*& pCurrentDSV,
            ID3D11BlendState*& pCurrentBlendState,
            ID3D11DepthStencilState*& pCurrentDSState,
            D3D11_VIEWPORT& currentViewport,
            FLOAT currentBlendFactor[4],
            UINT& currentSampleMask,
            UINT& currentStencilRef,
            UINT& numViewports);

        ID3D11DepthStencilView* setupDepthBuffer();
        ID3D11DepthStencilState* createDepthState(ID3D11DepthStencilView* pDepthView);

        void setupCameraMatrices(const D3D11_VIEWPORT& viewport,
            DirectX::XMMATRIX& viewMatrix,
            DirectX::XMMATRIX& projMatrix);

        ID3D11BlendState* createBlendState();

        void renderPathTubes(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void renderDirectionArrows(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);
        void renderNodeSpheres(const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projMatrix);

        void restoreRenderState(ID3D11RenderTargetView* pCurrentRTV,
            ID3D11DepthStencilView* pCurrentDSV,
            ID3D11BlendState* pCurrentBlendState,
            ID3D11DepthStencilState* pCurrentDSState,
            const D3D11_VIEWPORT& currentViewport,
            const FLOAT currentBlendFactor[4],
            UINT currentSampleMask,
            UINT currentStencilRef);

        // ==== State variables ====
        bool _isInitialized = false;
        bool _renderTargetInitialized = false;
        bool _pathResourcesCreated = false;
        bool _isVisible = true;

        // ==== D3D Core Resources ====
        IDXGISwapChain* _pSwapChain = nullptr;
        ID3D11Device* _pDevice = nullptr;
        ID3D11DeviceContext* _pContext = nullptr;
        ID3D11RenderTargetView* _pRenderTargetView = nullptr;

        // ==== Render State Objects ====
        ID3D11BlendState* _pBlendState = nullptr;
        ID3D11DepthStencilState* _pDepthStencilState = nullptr;
        ID3D11RasterizerState* _pRasterizerState = nullptr;

        // ==== Window Properties ====
        UINT _windowWidth = 0;
        UINT _windowHeight = 0;

        // ==== Path Visualization Resources ====
        ID3D11Buffer* _pathVertexBuffer = nullptr;
        ID3D11Buffer* _nodeVertexBuffer = nullptr;
        ID3D11Buffer* _interpolatedPathVertexBuffer = nullptr;
        ID3D11Buffer* _directionIndicatorVertexBuffer = nullptr;
        ID3D11Buffer* _pathTubeVertexBuffer = nullptr;
        ID3D11Buffer* _pathTubeIndexBuffer = nullptr;
        ID3D11Buffer* _interpolatedDirectionsVertexBuffer = nullptr;

        UINT _pathVertexCount = 0;
        UINT _nodeVertexCount = 0;
        UINT _interpolatedPathVertexCount = 0;
        UINT _directionIndicatorVertexCount = 0;
        UINT _pathTubeVertexCount = 0;
        UINT _pathTubeIndexCount = 0;
        UINT _interpolatedDirectionsVertexCount = 0;

        // ==== Path Data ====
        std::vector<PathInfo> _pathInfos;
        std::vector<DirectX::XMFLOAT3> _nodePositions;

        // ==== Arrow Rendering Resources ====
        ID3D11Buffer* _arrowHeadVertexBuffer = nullptr;
        ID3D11Buffer* _arrowHeadIndexBuffer = nullptr;
        ID3D11Buffer* _arrowShaftVertexBuffer = nullptr;
        ID3D11Buffer* _arrowShaftIndexBuffer = nullptr;
        UINT _arrowHeadIndexCount = 0;
        UINT _arrowShaftIndexCount = 0;

        // ==== Thread Safety ====
        std::mutex _resourceMutex;
        std::atomic<bool> _isChangingMode{ false };
        std::atomic<bool> _resourcesNeedUpdate{ false };

        // ==== Configuration Constants ====
        const int _interpolationSteps = 30;
        const float _nodeSize = 0.05f;
        const float _directionLength = 0.3f;
        const float _tubeDiameter = 0.05f;
        const int _tubeSegments = 8;
        const float _baseDirectionLength = 0.3f;
        const float _defaultFOV = DEFAULT_FOV;
        const int _rotationSampleFrequency = 3;
        float _arrowHeadLength = 0.05f;
        float _arrowHeadRadius = 0.025f;
        float _arrowShaftRadius = 0.008f;
        int _arrowSegments = 12;

        // ==== Static shared resources ====
        static Present_t _originalPresent;
        static ResizeBuffers_t _originalResizeBuffers;
        static OMSetRenderTargets_t _originalOMSetRenderTargets;

        static ID3D11Device* _pLastDevice;
        static ID3D11DeviceContext* _pLastContext;
        static ID3D11RenderTargetView* _pLastRTV;

        static ID3D11Buffer* _sphereVertexBuffer;
        static ID3D11Buffer* _sphereIndexBuffer;
        static ID3D11VertexShader* _simpleVertexShader;
        static ID3D11PixelShader* _simplePixelShader;
        static ID3D11InputLayout* _simpleInputLayout;
        static ID3D11Buffer* _constantBuffer;
        static ID3D11BlendState* _blendState;
        static UINT _sphereIndexCount;
        static DirectX::XMFLOAT3 _fixedSpherePosition;
        static ID3D11PixelShader* _coloredPixelShader;
        static ID3D11Buffer* _colorBuffer;

        static std::vector<ID3D11DepthStencilView*> _detectedDepthStencilViews;
        static std::vector<ID3D11Texture2D*> _detectedDepthTextures;
        static std::vector<D3D11_TEXTURE2D_DESC> _depthTextureDescs;
        static int _currentDepthBufferIndex;
        static int _depthScanCount;
        static bool _hookingOMSetRenderTargets;
        static bool _useDetectedDepthBuffer;
        static ID3D11DepthStencilView* _currentDepthStencilView;

        static bool _visualizationEnabled;
        static bool _pathVisualizationEnabled;
        static bool _spherePositionInitialized;
        static bool _renderSelectedPathOnly;
    };

} // namespace IGCS