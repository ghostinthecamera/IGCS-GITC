#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include "MessageHandler.h"
#include <DirectXMath.h>
#include <random>
#include "Globals.h"

namespace IGCS::PathUtils
{
	template <typename T>
	inline T lerpFMA(T v0, T v1, T t) {
		return fma(t, v1, fma(-t, v0, v0));
	}

	template <typename T>
	inline T lerp(T v0, T v1, T t) {
		return (1 - t) * v0 + t * v1;
	}


	// Handheld camera shake functions
	float perlinNoise1D(float x);
	float perlinNoise2D(float x, float y);
	float fractalBrownianMotion(float x, const int octaves, const float persistence);
	XMVECTOR generateHandheldPositionNoise(float time, float intensity, float breathingRate, float driftSpeed);
	XMVECTOR generateHandheldRotationNoise(float time, float intensity, float breathingRate, float driftSpeed);
	XMVECTOR simulateHandheldCamera(float deltaTime, float driftIntensity, float jitterIntensity, float breathingIntensity,
		XMVECTOR& currentVelocity, float driftSpeed);

	XMVECTOR LerpV(const XMVECTOR& a, const XMVECTOR& b, float t);

	XMVECTOR XMQuaternionLog(const DirectX::XMVECTOR& q);
	XMVECTOR SquadRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes);
	XMVECTOR SquadRotationExp(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
	XMVECTOR XMQuaternionExp(const XMVECTOR& q);
	XMVECTOR ClampTangent(const XMVECTOR& tangent, float maxMagnitude);

	float safeDiv(float num, float den, float eps);
	float vectorDistance(FXMVECTOR A, FXMVECTOR B);
	float applyEasing(float t, const EasingType& _easingType, const float easingValue);
	double smoothStep(double t);
	double smootherStep(double t);
	float SmoothStepFloat(float start, float end, float t);
	float SmoothStepDouble(float start, float end, double t);
	double smoothStepExp(double edge0, double edge1, double xt, double exponent);
	float randomJitter();
	float smoothNoise(float time, float frequency);
	inline void EnsureQuaternionContinuity(const FXMVECTOR qBase, XMVECTOR& qToCheck);

}