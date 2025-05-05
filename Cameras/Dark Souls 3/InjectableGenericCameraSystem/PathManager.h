#pragma once
#include "stdafx.h"
#include <DirectXMath.h>
#include "CameraPath.h"
#include <unordered_map>

namespace IGCS
{
	using namespace DirectX;

    enum PathManager : bool
    {
        PathManagerDisabled = false,
        PathManagerEnabled = true,
    };

    enum PathManagerStatus : uint8_t
    {
        Idle = 0,
        PlayingPath = 1,
        GotoNode = 2,
        Scrubbing = 3,
    };

    class CameraPathManager {
    public:
        // Reshade add-on function pointers for path management
        typedef void(__stdcall* IgcsConnector_addCameraPath)();
        typedef void(__stdcall* IgcsConnector_appendStateSnapshotToPath)(int pathIndex);
        typedef void(__stdcall* IgcsConnector_appendStateSnapshotAfterSnapshotOnPath)(int pathIndex, int indexToAppendAfter);
        typedef void(__stdcall* IgcsConnector_insertStateSnapshotBeforeSnapshotOnPath)(int pathIndex, int indexToInsertBefore);
        typedef void(__stdcall* IgcsConnector_removeStateSnapshotFromPath)(int pathIndex, int stateIndex);
        typedef void(__stdcall* IgcsConnector_removeCameraPath)(int pathIndex);
        typedef void(__stdcall* IgcsConnector_clearPaths)();
        typedef void(__stdcall* IgcsConnector_setReshadeState)(int pathIndex, int stateIndex);
        typedef void(__stdcall* IgcsConnector_setReshadeStateInterpolated)(int pathIndex, int fromStateIndex, int toStateIndex, float interpolationFactor);
        typedef void(__stdcall* IgcsConnector_updateStateSnapshotOnPath)(int pathIndex, int stateIndex);

        // Singleton instance accessor
        static CameraPathManager& instance()
        {
            static CameraPathManager instance;
            return instance;
        }

        // Delete copy constructor and assignment operator
        CameraPathManager(const CameraPathManager&) = delete;
        CameraPathManager& operator=(const CameraPathManager&) = delete;
    
        CameraPath* getPath(const std::string& pathName);
        bool createPath(const std::string& pathName);
        void playPath(float deltaTime);
        void handlePlayPathMessage();
        bool handleDeletePathMessage(const std::string& pathName);
        bool handleDeletePathMessage();
        uint8_t handleAddNodeMessage();
        void handleInsertNodeBeforeMessage(uint8_t byteArray[], DWORD arrayLength);
        [[nodiscard]] const std::unordered_map<std::string, CameraPath>& getPaths() const { return _paths; }
        void sendAllCameraPathsCombined() const;
        void handleDeleteNodeMessage(uint8_t byteArray[], DWORD arrayLength);
        void handleDeleteNodeMessage(uint8_t nodeIndex);
        void goToNodeSetup(uint8_t byteArray[], DWORD arrayLength);
        void gotoNodeIndex(float deltaTime);
        void sendAllCameraPathsBinary() const;
        void initNodePayload();
        void handleAddPathMessage(uint8_t byteArray[], DWORD arrayLength);
        std::string handleAddPathMessage();
        [[nodiscard]] std::string generateDefaultPathName() const;
        void handleStopPathMessage();
        void refreshPath();
        void updateNode(uint8_t byteArray[], DWORD arrayLength);
		void setPathManagerState(PathManager state) { _pathManagerEnabled = state; }
		[[nodiscard]] PathManager getPathManagerState() const { return _pathManagerEnabled; }
		[[nodiscard]] PathManagerStatus getPathManagerFunction() const { return _pathManagerState; }

        PathManager _pathManagerEnabled = PathManagerDisabled;
        PathManagerStatus _pathManagerState = Idle;
        PathManagerStatus _preScrubbingState = Idle;

        void setSelectedPath(uint8_t buffer[], DWORD bytesRead);
        void setPathManagerStatus(uint8_t buffer[], const DWORD bytesRead);
        void setSelectedPath(const std::string& pathName);
		void setSelectedNodeIndex(uint8_t buffer[], DWORD bytesRead);
        void sendSelectedPathUpdate(const std::string& pathName) const;
        [[nodiscard]] uint8_t getSelectedNodeIndex() const { return _selectedNodeIndex; }
		[[nodiscard]] std::string getSelectedPathName() const { return _selectedPath; }
		[[nodiscard]] CameraPath* getSelectedPathPtr() { return getPath(_selectedPath); }
        [[nodiscard]] std::string getSelectedPath() const { return _selectedPath; }
        int getPathIndex(const std::string& pathName) { return pathIndexMap.at(pathName); }
        std::string getPathAtIndex(int index) { return indexPathMap.at(index); }
		int getPathCount() { return static_cast<int>(_paths.size()); }


        //reshade connection
        void connectToAddon(HMODULE hModule);
        bool reshadeAddonisConnected();
        void addReshadePath();
        void appendStateToReshadePath(const std::string& pathName);
        void appendStateAfterReshadeSnapshot(int pathIndex, int snapshotIndex);
        void insertStateBeforeReshadeSnapshot(const std::string& pathName, int snapshotIndex);
        void removeReshadeState(const std::string& pathName, int stateIndex);
        void removeReshadePath(const std::string& pathName);
        void clearAllReshadelPaths();
        void applyReshadeState(const std::string& pathName, int stateIndex);
        void applyInterpolatedReshadeState(const std::string& pathName, int fromState, int toState, float factor);
        void updateReshadeState(const std::string& pathName, int stateIndex);

        void handlePathScrubbingMessage(uint8_t byteArray[], DWORD arrayLength);
        void updatePMState(PathManagerStatus status);
        void sendPathProgress(float progress) const;
		void setScrubbingProgress(float progress) { _scrubbingProgress = progress; }
		float getScrubbingProgress() const { return _scrubbingProgress; }
        void updateScrubbingInterpolation(float deltaTime);
        void scrubPath();

        float _targetScrubbingProgress = 0.0f;
        float _currentScrubbingProgress = 0.0f;
        static constexpr float SCRUBBING_SMOOTHING = 8.0f; // Adjust for responsiveness

    private:
        CameraPathManager();
        ~CameraPathManager();

    	std::unordered_map<std::string, CameraPath> _paths;
        // Maps path names to their corresponding indices in IgcsConnector
        std::unordered_map<std::string, int> pathIndexMap;
        // Inverse map to get index of a particular path name
        std::unordered_map<int, std::string> indexPathMap;
        // Maintains the order of paths as they are in IgcsConnector
        std::vector<std::string> pathOrder;
        // Call this whenever paths might have changed
        void rebuildPathIndices();

        // Define a structure to hold the decoded payload.
        struct GoToNodePayload {
            uint8_t nodeIndex = 0;
            XMFLOAT4 currentQ = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            XMFLOAT3 currentP = XMFLOAT3(0.0f, 0.0f, 0.0f);
            float currentfov = 0.0f;
            bool interpolating = false;
            float elapsedTime = 0.0f;
            float totalDuration = 0.0f; // seconds, adjust as needed
            CameraPath* path;
        };
        GoToNodePayload nodePayload;

        //Path Controller
        std::string _selectedPath = "null";
        uint8_t _selectedNodeIndex = 254;

        bool _isScrubbing = false;
        float _scrubbingProgress = 0.0f;
    };


}
