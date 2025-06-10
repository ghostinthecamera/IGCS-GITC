#pragma once
#include "stdafx.h"
#include <DirectXMath.h>
#include "Utils.h"

namespace IGCS
{
	enum PathManagerStatus : uint8_t;
	class CameraPathManager;//forward declaration to avoid cyclical dependency
}


namespace IGCS
{
	using namespace DirectX;

	struct CameraNode {
		XMVECTOR position;
		XMVECTOR rotation;
		float fov;

		CameraNode(const XMVECTOR& pos, const XMVECTOR& rot, float fov)
			: position(pos), rotation(rot), fov(fov)
		{
		}

		XMFLOAT3 nodePos() {
			XMFLOAT3 pos;
			XMStoreFloat3(&pos, position);
			return pos;
		}

		XMFLOAT4 nodeRot() {
			XMFLOAT4 rot;
			XMStoreFloat4(&rot, rotation);
			return rot;
		}
	};

	enum class EasingMode : uint8_t {
		TimeEasing = 0, // ease the time fraction first, then get the uniform parameter
		ArcEasing = 1   // get the uniform parameter first, then ease that fraction
	};
	enum class InterpolationMode : uint8_t
	{
		CatmullRom = 0,       // 
		Centripetal = 1,	  // chord-len Catmull–Rom for quaternions
		Bezier = 2,
		BSpline = 3,
		Cubic = 4
	};
	enum class RotationMode : uint8_t
	{
		Standard = 0,
		SQUAD = 1,// 
		RiemannCubic = 2
	};

	enum class ParameterMode : uint8_t
	{
		UniformTime = 0,    // Use uniform parameter [0..1] in time
		ArcLengthUniformSpeed = 1, // Use arc-length table for uniform motion in space
	};

	enum class EasingType : uint8_t {
		None = 0,      // No easing (default linear motion)
		EaseIn = 1,    // Slow start, fast end
		EaseOut = 2,   // Fast start, slow end
		EaseInOut = 3  // Slow start, fast middle, slow end
	};

	enum class DeltaType : uint8_t
	{
		FPSLinked = 0,  //use deltatime calculated between previous frame and current frame
		FPSUnlinked = 1,  //use a fixed interval update, independent of frametime
	};

	class CameraPath
	{
	public:
		explicit CameraPath(std::string pathName);
		CameraPath();

		//Node Creation + Manipulation
		static CameraNode createNode();
		void addNode(const XMVECTOR position, const XMVECTOR rotation, float fov);
		uint8_t addNode();

		void insertNodeBefore(uint8_t nodeIndex);
		void deleteNode(uint8_t nodeIndex);
		void updateNode(uint8_t nodeIndex);

		//Node playback
		void interpolateNodes(float deltaTime);
		void updateReshadeStateController(const float& globalT) const;
		void getSettings();
		float getGlobalT(const float& deltaTime, PathManagerStatus currentState);
		XMVECTOR applySmoothPlayerOffset(XMVECTOR position, float deltaTime);// Apply smoothed player offset

		//Utility/helpers
		
		void printNodeDetails(size_t i) const;
		void resetPath();
		void setTotalDuration(float totalDuration);
		void generateArcLengthTables();
		void getnodeSequence(int& i0, int& i1, int& i2, int& i3, float globalT) const;
		void setArcLengthTables();
		void performPositionInterpolation(float globalT);
		void performRotationInterpolation(float globalT);
		static float getGlobalTForDistance(float distance, const std::vector<float>& arcTable, const std::vector<float>& paramTable);
		void applyShakeEffect();

		void continuityCheck();

		//Node setters/getters
		[[nodiscard]] float getNodeFOV(const size_t i) const { return _nodes.at(i).fov; }
		[[nodiscard]] double getDuration() const { return _totalDuration; }
		[[nodiscard]] XMVECTOR getNodePosition(const size_t i) const { return _nodes.at(i).position; }
		[[nodiscard]] int getsegmentIndex(float globalT) const;
		[[nodiscard]] XMVECTOR getNodeRotation(const size_t i) const { return _nodes.at(i).rotation; }
		[[nodiscard]] XMFLOAT3 getNodePositionFloat(size_t i) const;
		[[nodiscard]] size_t GetNodeCount() const { return _nodes.size(); }
		void setInterpolationMode(const InterpolationMode mode) { _interpolationMode = mode; }
		[[nodiscard]] const InterpolationMode getInterpolationMode() const { return  _interpolationMode; }
		void setRotationMode(const RotationMode mode) { _rotationMode = mode; }
		[[nodiscard]] const RotationMode getRotationMode() const { return  _rotationMode; }
		void setParameterMode(const ParameterMode pm) { _parameterMode = pm; }
		[[nodiscard]] ParameterMode getParameterMode() const { return _parameterMode; }
		void setEasingType(const EasingType type) { _easingType = type; }
		[[nodiscard]] EasingType getEasingType() const { return _easingType; }
		[[nodiscard]] bool isRelativeToPlayerEnabled() const { return _relativeToPlayerEnabled; }
		void setPlayerPosition(const XMVECTOR& position) { _playerPosition = position; }
		[[nodiscard]] XMVECTOR getPlayerPosition() const { return _playerPosition; }
		void setOffsetSmoothingFactor(const float factor) { _offsetSmoothingFactor = max(0.1f, factor); }
		[[nodiscard]] float getOffsetSmoothingFactor() const { return _offsetSmoothingFactor; }
		[[nodiscard]] const std::vector<CameraNode>* getNodes() const { return &_nodes; } 
		[[nodiscard]] const std::vector<float>* getknotVector() const { return &_knotVector; }
		[[nodiscard]] const std::vector<float>* getControlPointsFOV() const { return &_controlPointsFOV; }
		[[nodiscard]] const std::vector<XMVECTOR>* getControlPointsPos() const { return &_controlPointsPos; }
		[[nodiscard]] const std::vector<XMVECTOR>* getControlPointsRot() const { return &_controlPointsRot; }
		void setRelativeToPlayerEnabled(const bool enabled) {
			_relativeToPlayerEnabled = enabled;
			if (enabled) initializeRelativeTracking();
		}
		[[nodiscard]] uint8_t getNodeSize() const { return static_cast<uint8_t>(_nodes.size()); }
		void setProgressPosition(float progress);
		float getProgress() const;


	private:
		XMVECTOR interpolatedRotation = XMQuaternionIdentity();
		static InterpolationMode _interpolationMode;
		static ParameterMode _parameterMode;
		static RotationMode _rotationMode;
		static EasingType _easingType;
		static int _samplesize; // number of samples for interpolation


		
		
		// Camera physics simulation variables
		XMVECTOR interpolatedPosition = XMVectorZero();
		XMVECTOR _handheldPositionVelocity = XMVectorZero();  // Current velocity for position
		XMVECTOR _handheldRotationVelocity = XMVectorZero();  // Current velocity for rotation
		XMVECTOR _playerPosition = XMVectorZero();   // Current player position
		XMVECTOR _initialPlayerPosition = XMVectorZero(); // Player position when path started
		XMVECTOR _currentPlayerOffset = XMVectorZero(); // Current smoothed offset value
		

		std::string _pathName; // Name of the path

		std::vector<CameraNode> _nodes;
		size_t _currentNode = 0;
		double _currentTime = 0.0; //for UniformTime mode
		double _totalDuration = 0.0; // Duration per node in seconds
		float _currentArcLength = 0.0f; // how far along the path we are
		float _currentProgress = 0.0f;
		

		float interpolatedFOV = 0.0f;
		float totalDist = 0.0f;

		const std::vector<float>* arcTable = nullptr;
		const std::vector<float>* paramTable = nullptr;

		//CatmullRom
		std::vector<float> _arcLengthTable;  // cumulative distance at each sample
		std::vector<float> _paramTable;      // corresponding parameter (segment + t)	

		///Centripetal
		std::vector<float> _arcLengthTableCentripetal;
		std::vector<float> _paramTableCentripetal;

		/////Bezier
		std::vector<float> _arcLengthTableBezier;
		std::vector<float> _paramTableBezier;
	
		/////BSPLINE
		std::vector<float> _arcLengthTableBspline;      // NEW
		std::vector<float> _paramTableBspline;          // NEW
		std::vector<XMVECTOR> _controlPointsPos; // Control points for positions
		std::vector<XMVECTOR> _controlPointsRot; // Control points for rotations
		std::vector<float> _knotVector;         // Knot vector for B-spline
		std::vector<float> _controlPointsFOV;

		//CUBIC	
		std::vector<float> _arcLengthTableCubic;
		std::vector<float> _paramTableCubic;

		bool _applyShake = false; // Flag to toggle camera shake
		float _shakeAmplitude = 0.0f;  // Strength of the shake
		float _shakeFrequency = 0.0f;   // How fast the shake changes (Hz)
		float _shakeTime = 0.0f;        // Keeps track of time for smooth noise

		// Handheld camera settings
		bool _enableHandheldCamera = false;        // Toggle for handheld camera effect
		float _handheldIntensity = 1.0f;          // Overall intensity multiplier 
		float _handheldDriftIntensity = 1.0f;     // Low frequency motion intensity
		float _handheldJitterIntensity = 1.0f;    // High frequency jitter intensity
		float _handheldBreathingIntensity = 0.0f; // Breathing motion intensity
		float _handheldBreathingRate = 0.0f;      // Breathing cycle rate in Hz
		float _handheldDriftSpeed = 0.05f;        // Drift speed in Hz (how quickly drift changes)
		float _handheldRotationDriftSpeed = 0.03f; // Drift speed for rotation
		bool _handheldRotationEnabled = true;     // Enable rotation component
		bool _handheldPositionEnabled = true;     // Enable position component
		float _handheldTimeAccumulator = 0.0f;    // Time accumulator for handheld simulation



		float _handheldDrift = 0.0f;   // Low frequency motion component
		float _handheldJitter = 0.0f;  // High frequency motion component
		float _breathingCycle = 0.0f;  // Medium frequency motion for breathing effect
		void applyHandheldCameraEffect(const float deltaTime);

		// Initialize relative tracking with current player position
		void initializeRelativeTracking() {
			_initialPlayerPosition = _playerPosition;
			_isFirstFrame = false;
			/*MessageHandler::logDebug("CameraPath::initializeRelativeTracking::Player position stored for relative mode: (%f, %f, %f)",
				XMVectorGetX(_initialPlayerPosition), XMVectorGetY(_initialPlayerPosition), XMVectorGetZ(_initialPlayerPosition));*/
		}

		bool _relativeToPlayerEnabled = false;       // Toggle for relative-to-player mode

		bool _isFirstFrame = true;                   // Flag to detect first frame of playback

		// Settings for smooth player offset
		float _offsetSmoothingFactor = 5.0f;          // Controls how quickly the camera follows player movement
		
	};
}