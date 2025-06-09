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
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    /**
     * @brief Constant buffer for shader transformation matrix
     */
    struct SimpleConstantBuffer {
        XMMATRIX worldViewProjection;
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
        [[nodiscard]] bool isFullyInitialized() const;
        [[nodiscard]] bool needsInitialization() const { return _needsInitialization; }
        void markResourcesForUpdate() { _resourcesNeedUpdate.store(true, std::memory_order_release); }

        // ==== Visualization control ====
        void setVisualization(bool enabled);
        [[nodiscard]] bool isVisualizationEnabled() const;
        void setRenderPathOnly(bool renderPathOnly);
        [[nodiscard]] bool isRenderPathOnly() const;

        // ==== Depth buffer management ====
        void cycleDepthBuffer();
        [[nodiscard]] bool isUsingDepthBuffer() const;
        void toggleDepthBufferUsage();

        // ==== Path visualization ====
        void safeInterpolationModeChange();
        void createPathVisualization();
        void renderPaths();


        void cleanupAllResources();

    private:
        // ==== Constructors & assignment operators ====
        D3DHook();
        ~D3DHook();
        D3DHook(const D3DHook&) = delete;
        D3DHook& operator=(const D3DHook&) = delete;

        // ==== Initialization helpers ====
        bool createRenderTargetView();
        D3D11_VIEWPORT getFullViewport();
        /*bool findActiveSwapChain();*/
        static bool initializeRenderingResources();
        std::atomic<bool> _needsInitialization{ false };


        // ==== Hook function types and implementations ====
        typedef HRESULT(STDMETHODCALLTYPE* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
        typedef void(STDMETHODCALLTYPE* OMSetRenderTargets_t)(ID3D11DeviceContext* pContext, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView);

        static HRESULT STDMETHODCALLTYPE hookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
        static HRESULT STDMETHODCALLTYPE hookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
        static void STDMETHODCALLTYPE hookedOMSetRenderTargets(ID3D11DeviceContext* pContext, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView);

        // ==== Rendering helpers ====
        void createSolidColorSphere(float radius, const XMFLOAT4& color);
        static void createTubeMesh(std::vector<SimpleVertex>& tubeVertices, std::vector<UINT>& tubeIndices,
                                   const XMFLOAT3& startPoint, const XMFLOAT3& endPoint,
                                   const XMFLOAT4& color, float radius, int segments);

        // ==== Arrow visualization ====
        void createArrowGeometry();
        void renderArrow(const XMFLOAT3& position, const XMVECTOR& direction,
            float length, const XMFLOAT4& color) const;

        // ==== Depth buffer management ====
        void setupOMSetRenderTargetsHook();
        void scanForDepthBuffers();
        void selectAppropriateDepthBuffer();
        void captureActiveDepthBuffer(ID3D11DepthStencilView* pDSV);
        static bool isDepthFormat(DXGI_FORMAT format);
        static DXGI_FORMAT getDepthViewFormat(DXGI_FORMAT resourceFormat);
        void cleanupDepthBufferResources();

        struct GameDepthFormat {
            int format;
            bool matchViewportSize;
        };

        // Static member with inline initialization
        static inline std::unordered_map<std::string, GameDepthFormat> _gameDepthFormats{
            {"darksoulsiii.exe", {19, true}},
            {"nino2.exe", {44, true}}
        };


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
            XMFLOAT4 pathColor{};
            std::vector<std::pair<XMFLOAT3, XMFLOAT3>> pathDirections{};
        };

        struct InterpSample {
            XMVECTOR rotation;
        	XMFLOAT3 position;
            float fov;
        };

        struct TubeInfo {
            UINT startIndex;
            UINT indexCount;
            XMFLOAT4 color;
        };

        // ==== Path Visualization Helpers ====
        void processAllPaths(const std::unordered_map<std::string, CameraPath>& paths,
            std::vector<SimpleVertex>& nodeVertices,
            std::vector<SimpleVertex>& directionIndicatorVertices,
            std::vector<SimpleVertex>& interpolatedDirectionVertices,
            std::vector<SimpleVertex>& tubeMeshVertices,
            std::vector<UINT>& tubeMeshIndices,
            std::vector<TubeInfo>& pathTubeInfos,
            const XMFLOAT4& nodeColor,
            const XMFLOAT4& directionColor);

        void processPathNodes(const CameraPath& path, size_t nodeCount,
            std::vector<SimpleVertex>& nodeVertices,
            std::vector<SimpleVertex>& directionIndicatorVertices,
            const XMFLOAT4& nodeColor,
            const XMFLOAT4& directionColor);

        static XMFLOAT4 convertHSVtoRGB(float hue, float saturation, float value, float alpha);

        [[nodiscard]] std::vector<InterpSample> generateInterpolatedSamples(const CameraPath& path, size_t nodeCount);

        void createInterpolatedDirections(const std::vector<InterpSample>& interpSamples,
            std::vector<SimpleVertex>& interpolatedDirectionVertices) const;

        static void storePathDirections(PathInfo& pathInfo,
                                        const std::vector<SimpleVertex>& interpolatedDirectionVertices,
                                        size_t startIndex, size_t count);

        void createPathTubes(const std::vector<XMFLOAT3>& points,
            std::vector<SimpleVertex>& tubeMeshVertices,
            std::vector<UINT>& tubeMeshIndices,
            const XMFLOAT4& color) const;

        void createPathBuffers(const std::vector<SimpleVertex>& nodeVertices,
            const std::vector<SimpleVertex>& directionIndicatorVertices,
            const std::vector<SimpleVertex>& interpolatedDirectionVertices,
            const std::vector<SimpleVertex>& tubeMeshVertices,
            const std::vector<UINT>& tubeMeshIndices,
            const std::vector<TubeInfo>& pathTubeInfos);

        // ==== Rendering State Management ====
        ID3D11DepthStencilView* setupDepthBuffer();

        static void setupCameraMatrices(const D3D11_VIEWPORT& viewport,
                                        XMMATRIX& viewMatrix,
                                        XMMATRIX& projMatrix);

        void renderPathTubes(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix) const;
        void renderDirectionArrows(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix);
        void renderNodeSpheres(const XMMATRIX& viewMatrix, const XMMATRIX& projMatrix) const;

        [[nodiscard]] ID3D11RasterizerState* getOrCreateRasterizerState() const;
        [[nodiscard]] ID3D11DepthStencilState* createDepthStateForRendering(bool depthEnabled) const;
        bool prepareForRendering(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv,
                                 const D3D11_VIEWPORT& viewport) const;
        [[nodiscard]] ID3D11BlendState* getOrCreateBlendState();

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
        std::vector<XMFLOAT3> _nodePositions;

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
        const float _tubeDiameter = 0.03f;
        const int _tubeSegments = 14;
        const float _baseDirectionLength = 0.3f;
        const float _defaultFOV = DEFAULT_FOV;
        const int _rotationSampleFrequency = 3;
        float _arrowHeadLength = 0.05f;
        float _arrowHeadRadius = 0.02f;
        float _arrowShaftRadius = 0.006f;
        int _arrowSegments = 32;

        // ==== Static shared resources ====
        static Present_t _originalPresent;
        static ResizeBuffers_t _originalResizeBuffers;
        static OMSetRenderTargets_t _originalOMSetRenderTargets;

        ID3D11Device* _pLastDevice = nullptr;
        ID3D11DeviceContext* _pLastContext = nullptr;
        ID3D11RenderTargetView* _pLastRTV = nullptr;

        ID3D11Buffer* _sphereVertexBuffer = nullptr;
        ID3D11Buffer* _sphereIndexBuffer = nullptr;
        ID3D11VertexShader* _simpleVertexShader = nullptr;
        ID3D11PixelShader* _simplePixelShader = nullptr;
        ID3D11InputLayout* _simpleInputLayout = nullptr;
        ID3D11Buffer* _constantBuffer = nullptr;
        UINT _sphereIndexCount = 0;
        ID3D11PixelShader* _coloredPixelShader = nullptr;
        ID3D11Buffer* _colorBuffer = nullptr;

        std::vector<ID3D11DepthStencilView*> _detectedDepthStencilViews;
        std::vector<ID3D11Texture2D*> _detectedDepthTextures;
        std::vector<D3D11_TEXTURE2D_DESC> _depthTextureDescs;
        int _currentDepthBufferIndex = 0;
        int _depthScanCount = 0;
        bool _hookingOMSetRenderTargets = false;
        bool _useDetectedDepthBuffer = false;
        ID3D11DepthStencilView* _currentDepthStencilView = nullptr;

        bool _visualizationEnabled = false;
        bool _renderSelectedPathOnly;
        bool _depthMatchFound = false;



        class StateGuard {
        public:
            StateGuard(ID3D11DeviceContext* context) : _context(context) {
                if (!_context) return;

                // Save render targets
                _context->OMGetRenderTargets(1, &_state.rtv, &_state.dsv);

                // Save blend state
                _context->OMGetBlendState(&_state.blendState, _state.blendFactor, &_state.sampleMask);

                // Save depth stencil state
                _context->OMGetDepthStencilState(&_state.depthState, &_state.stencilRef);

                // Save rasterizer state
                _context->RSGetState(&_state.rasterizerState);

                // Save viewport
                UINT numViewports = 1;
                _context->RSGetViewports(&numViewports, &_state.viewport);
                _state.viewportValid = (numViewports > 0);
            }

            ~StateGuard() {
                Restore();
            }

            void Restore() {
                if (!_context || _restored) return;

                // Restore render targets
                _context->OMSetRenderTargets(1, &_state.rtv, _state.dsv);

                // Restore blend state
                _context->OMSetBlendState(_state.blendState, _state.blendFactor, _state.sampleMask);

                // Restore depth stencil state
                _context->OMSetDepthStencilState(_state.depthState, _state.stencilRef);

                // Restore rasterizer state
                _context->RSSetState(_state.rasterizerState);

                // Restore viewport if valid
                if (_state.viewportValid) {
                    _context->RSSetViewports(1, &_state.viewport);
                }

                // Release references
                SafeRelease(_state.rtv);
                SafeRelease(_state.dsv);
                SafeRelease(_state.blendState);
                SafeRelease(_state.depthState);
                SafeRelease(_state.rasterizerState);

                _restored = true;
            }

        private:
            template<typename T>
            static void SafeRelease(T*& resource) {
                if (resource) {
                    resource->Release();
                    resource = nullptr;
                }
            }

            struct RenderingState {
                ID3D11RenderTargetView* rtv = nullptr;
                ID3D11DepthStencilView* dsv = nullptr;
                ID3D11BlendState* blendState = nullptr;
                ID3D11DepthStencilState* depthState = nullptr;
                ID3D11RasterizerState* rasterizerState = nullptr;
                D3D11_VIEWPORT viewport = {};
                FLOAT blendFactor[4] = { 0 };
                UINT sampleMask = 0xffffffff;
                UINT stencilRef = 0;
                bool viewportValid = false;
            };

            ID3D11DeviceContext* _context = nullptr;
            RenderingState _state;
            bool _restored = false;
        };

        // New state management helpers
        struct RenderingState;
        class StateGuard;
    };



    // Prioritized depth buffer formats
    static constexpr DXGI_FORMAT PREFERRED_DEPTH_FORMATS[] = {
        // Most common formats
        DXGI_FORMAT_R24G8_TYPELESS,         // Most common typeless (44) -> D24_UNORM_S8_UINT view
        DXGI_FORMAT_D24_UNORM_S8_UINT,      // Common explicit format (45)
        DXGI_FORMAT_R32_TYPELESS,           // 32-bit typeless (40) -> D32_FLOAT view
        DXGI_FORMAT_D32_FLOAT,              // Explicit 32-bit depth (41)

        // Higher precision formats
        DXGI_FORMAT_R32G8X24_TYPELESS,      // Combined typeless (52) -> D32_FLOAT_S8X24_UINT view
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   // Explicit 32-bit depth + stencil (46)

        // Lower precision formats
        DXGI_FORMAT_R16_TYPELESS,           // 16-bit typeless (19) -> D16_UNORM view
        DXGI_FORMAT_D16_UNORM,              // Explicit 16-bit depth (20)

        // Partial formats (sometimes used in special cases)
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  // Depth portion of D24S8 (47)
        DXGI_FORMAT_X24_TYPELESS_G8_UINT,   // Stencil portion of D24S8 (48)
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT, // Stencil portion of D32S8X24 (53)

        // Resource formats sometimes bound as depth
        DXGI_FORMAT_R16_UNORM,              // For certain MSAA configurations (56)
        DXGI_FORMAT_R32_FLOAT,              // For shader resource views of depth (41)

        // Uncommon formats for older hardware or special use cases
        DXGI_FORMAT_R16G16_TYPELESS,        // Some engines use dual-16 formats (33)
        DXGI_FORMAT_R16G16_FLOAT           // For special depth encoding (34)
    };
    static constexpr int PREFERRED_DEPTH_FORMAT_COUNT = std::size(PREFERRED_DEPTH_FORMATS);

} // namespace IGCS