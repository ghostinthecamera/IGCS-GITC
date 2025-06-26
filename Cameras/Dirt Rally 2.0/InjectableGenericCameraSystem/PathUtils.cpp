#include "stdafx.h"
#include "PathUtils.h"


namespace IGCS::PathUtils
{
	using namespace DirectX;

	// Linear interpolation helper.
	XMVECTOR LerpV(const XMVECTOR& a, const XMVECTOR& b, float t)
	{
		return XMVectorAdd(XMVectorScale(a, 1.0f - t), XMVectorScale(b, t));
	}

	XMVECTOR XMQuaternionLog(const XMVECTOR& q)
	{
		// Assume q is a normalized quaternion: q = (x, y, z, w)
		float w = XMVectorGetW(q);
		float x = XMVectorGetX(q);
		float y = XMVectorGetY(q);
		float z = XMVectorGetZ(q);
		// Compute the norm of the vector part.
		float sinTheta = sqrtf(x * x + y * y + z * z);
		// theta = arccos(w)
		float theta = acosf(w);
		if (fabs(sinTheta) > 1e-6f)
		{
			float scale = theta / sinTheta;
			// Return a pure quaternion: (scaled vector, 0)
			return XMVectorSet(x * scale, y * scale, z * scale, 0.0f);
		}
		return XMVectorZero();
	}

	XMVECTOR SquadRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes)
	{
		int totalSegments = static_cast<int>(_nodes.size()) - 1;
		if (totalSegments < 1)
		{
			return XMQuaternionIdentity();
		}

		// Compute segment index and local interpolation factor
		int segmentIndex = static_cast<int>(floorf(globalT));
		segmentIndex = max(0, min(segmentIndex, totalSegments - 1));

		float tLocal = globalT - static_cast<float>(segmentIndex);

		// Ensure we have enough nodes for SQUAD interpolation
		int i0 = max(segmentIndex - 1, 0);
		int i1 = segmentIndex;
		int i2 = min(segmentIndex + 1, totalSegments);
		int i3 = min(segmentIndex + 2, static_cast<int>(_nodes.size()) - 1);

		XMVECTOR Q0 = _nodes[i0].rotation;
		XMVECTOR Q1 = _nodes[i1].rotation;
		XMVECTOR Q2 = _nodes[i2].rotation;
		XMVECTOR Q3 = _nodes[i3].rotation;

		// Ensure quaternion continuity
		//if (XMVectorGetX(XMQuaternionDot(Q0, Q1)) < 0.0f) Q1 = XMVectorNegate(Q1);
		//if (XMVectorGetX(XMQuaternionDot(Q1, Q2)) < 0.0f) Q2 = XMVectorNegate(Q2);
		//if (XMVectorGetX(XMQuaternionDot(Q2, Q3)) < 0.0f) Q3 = XMVectorNegate(Q3);

		// Compute squad control points for segment (Q1 -> Q2)
		XMVECTOR A, B, C;
		XMQuaternionSquadSetup(&A, &B, &C, Q0, Q1, Q2, Q3);

		// Corrected: Interpolating from Q1 to Q2
		XMVECTOR result = XMQuaternionSquad(Q1, A, B, Q2, tLocal);

		return XMQuaternionNormalize(result);
	}


	XMVECTOR SquadRotationExp(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		int i0, i1, i2, i3;
		float t = globalT - path.getsegmentIndex(globalT);
		path.getnodeSequence(i0, i1, i2, i3, globalT);

		XMVECTOR q0 = _nodes[i0].rotation;
		XMVECTOR q1 = _nodes[i1].rotation;
		XMVECTOR q2 = _nodes[i2].rotation;
		XMVECTOR q3 = _nodes[i3].rotation;

		EnsureQuaternionContinuity(q0, q1);
		EnsureQuaternionContinuity(q1, q2);
		EnsureQuaternionContinuity(q2, q3);

		XMVECTOR logA = XMQuaternionLog(XMQuaternionMultiply(XMQuaternionInverse(q1), q0));
		XMVECTOR logB = XMQuaternionLog(XMQuaternionMultiply(XMQuaternionInverse(q1), q2));
		XMVECTOR A = XMQuaternionMultiply(q1, XMQuaternionExp(XMVectorScale(XMVectorAdd(logA, logB), -0.25f)));

		XMVECTOR logC = XMQuaternionLog(XMQuaternionMultiply(XMQuaternionInverse(q2), q1));
		XMVECTOR logD = XMQuaternionLog(XMQuaternionMultiply(XMQuaternionInverse(q2), q3));
		XMVECTOR B = XMQuaternionMultiply(q2, XMQuaternionExp(XMVectorScale(XMVectorAdd(logC, logD), -0.25f)));

		return XMQuaternionNormalize(XMQuaternionSquad(q1, A, B, q2, t));
	}

	inline void EnsureQuaternionContinuity(const FXMVECTOR qBase, XMVECTOR& qToCheck) 
	{
		// If dot < 0, flip the sign of qToCheck
		if (const float dotVal = XMVectorGetX(XMVector4Dot(qBase, qToCheck)); dotVal < 0.0f)
			qToCheck = XMVectorNegate(qToCheck);  // or qToCheck = -qToCheck
	}

	XMVECTOR XMQuaternionExp(const XMVECTOR& q)
	{
		// q is assumed to be a pure quaternion: q = (x, y, z, 0)
		float x = XMVectorGetX(q);
		float y = XMVectorGetY(q);
		float z = XMVectorGetZ(q);
		float theta = sqrtf(x * x + y * y + z * z);
		float cosTheta = cosf(theta);
		if (fabs(theta) > 1e-6f)
		{
			float scale = sinf(theta) / theta;
			return XMVectorSet(x * scale, y * scale, z * scale, cosTheta);
		}
		return XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	}


	// Helper function to clamp the magnitude of a tangent vector
	XMVECTOR ClampTangent(const XMVECTOR& tangent, float maxMagnitude)
	{
		float magnitude = XMVectorGetX(XMVector3Length(tangent));
		if (magnitude > maxMagnitude)
		{
			float scale = maxMagnitude / magnitude;
			return XMVectorScale(tangent, scale);
		}
		return tangent;
	}

	float safeDiv(float num, float den, float eps = 1e-8f) 
	{ 
		return (fabsf(den) < eps) ? 0.0f : (num / den); 
	}

	float vectorDistance(FXMVECTOR A, FXMVECTOR B) 
	{ 
		return XMVectorGetX(XMVector3Length(XMVectorSubtract(B, A))); 
	}

	float applyEasing(float t, const EasingType& _easingType, const float easingValue) {
		switch (_easingType) {
		case EasingType::EaseIn:
			// Use _easeInParam as an exponent (default 2 gives quadratic easing)
			return powf(t, easingValue);
		case EasingType::EaseOut:
			// Ease out: 1 - (1-t)^easeOutParam.
			return 1.0f - powf(1.0f - t, easingValue);
		case EasingType::EaseInOut:
			return (1.0f - cosf(powf(t, easingValue) * XM_PI)) / 2.0f;
		case EasingType::None:
		default:
			return t;
		}
	}

	double smoothStep(double t) {
		return t * t * (3.0 - 2.0 * t);
	}

	double smootherStep(double t) {
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	float SmoothStepFloat(float start, float end, float t)
	{
		// typical smoothstep
		t = t * t * (3.f - 2.f * t);
		return start + (end - start) * t;
	}

	float SmoothStepDouble(float start, float end, double t)
	{
		// typical smoothstep
		t = t * t * (3.f - 2.f * t);
		return start + (end - start) * static_cast<float>(t);
	}

	// A generalized smoothstep that lets you control the sharpness via an exponent.
	double smoothStepExp(double edge0, double edge1, double xt, double exponent) {
		double t = (xt - edge0) / (edge1 - edge0);
		t = max(0.0, min(1.0, t));
		// For exponent == 1, you get linear interpolation.
		// For exponent == 2, a quadratic behavior (similar to standard smoothstep but not exactly the same).
		// You can adjust the exponent to control the curvature.
		return pow(t, exponent) / (pow(t, exponent) + pow(1 - t, exponent));
	};

	// Helper function to generate smooth random noise
	float randomJitter()
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_real_distribution<float> dist(-0.02f, 0.02f);

		return dist(gen);
	}

	float smoothNoise(float time, float frequency)
	{
		return sin(time * frequency) * 0.5f + cos(time * frequency * 0.7f) * 0.3f;
	}

	// Improved Perlin noise implementation for 1D
	float perlinNoise1D(float x) {
		// Hash function
		auto hash = [](int x) -> float {
			x = (x << 13) ^ x;
			return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
			};

		int x0 = (int)floorf(x);
		int x1 = x0 + 1;
		float dx = x - (float)x0;

		// Smoothstep for interpolation
		float t = dx * dx * (3.0f - 2.0f * dx);

		float n0 = hash(x0);
		float n1 = hash(x1);

		return lerp(n0, n1, t);
	}

	// 2D Perlin noise function
	float perlinNoise2D(float x, float y) {
		// Simple 2D hash function
		auto hash2D = [](int x, int y) -> float {
			int n = x + y * 57;
			n = (n << 13) ^ n;
			return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
			};

		int x0 = static_cast<int>(floorf(x));
		int y0 = static_cast<int>(floorf(y));
		int x1 = x0 + 1;
		int y1 = y0 + 1;

		// Smoothstep for better interpolation
		float sx = x - static_cast<float>(x0);
		float sy = y - static_cast<float>(y0);
		float tx = sx * sx * (3.0f - 2.0f * sx);
		float ty = sy * sy * (3.0f - 2.0f * sy);

		float n00 = hash2D(x0, y0);
		float n10 = hash2D(x1, y0);
		float n01 = hash2D(x0, y1);
		float n11 = hash2D(x1, y1);

		// Bilinear interpolation
		float nx0 = lerp(n00, n10, tx);
		float nx1 = lerp(n01, n11, tx);
		return lerp(nx0, nx1, ty);
	}

	// Fractal Brownian Motion - combines multiple octaves of noise
	float fractalBrownianMotion(float x, const int octaves, const float persistence) {
		float total = 0.0f;
		float frequency = 1.0f;
		float amplitude = 1.0f;
		float maxValue = 0.0f;

		for (int i = 0; i < octaves; i++) {
			total += perlinNoise1D(x * frequency) * amplitude;
			maxValue += amplitude;
			amplitude *= persistence;
			frequency *= 2.0f;
		}

		return total / maxValue;
	}

	// Generate realistic handheld camera position noise
	XMVECTOR generateHandheldPositionNoise(float time, float intensity, float breathingRate, float driftSpeed) {
		// Drift component (low frequency)
		float driftX = fractalBrownianMotion(time * driftSpeed, 3, 0.5f) * intensity * 0.07f;
		float driftY = fractalBrownianMotion(time * (driftSpeed * 0.9f) + 100.0f, 3, 0.5f) * intensity * 0.07f;
		float driftZ = fractalBrownianMotion(time * (driftSpeed * 0.8f) + 200.0f, 3, 0.5f) * intensity * 0.05f;

		// Breathing component (medium frequency)
		float breathingAmplitude = intensity * 0.04f;
		float breathingY = sin(time * breathingRate) * breathingAmplitude;
		float breathingX = cos(time * breathingRate * 0.3f) * breathingAmplitude * 0.3f;

		// Jitter component (high frequency)
		float jitterScale = intensity * 0.015f;
		float jitterX = fractalBrownianMotion(time * 3.0f, 2, 0.8f) * jitterScale;
		float jitterY = fractalBrownianMotion(time * 3.0f + 300.0f, 2, 0.8f) * jitterScale;
		float jitterZ = fractalBrownianMotion(time * 3.0f + 600.0f, 2, 0.8f) * jitterScale * 0.7f;

		// Combine all components
		float x = driftX + breathingX + jitterX;
		float y = driftY + breathingY + jitterY;
		float z = driftZ + jitterZ;

		return XMVectorSet(x, y, z, 0.0f);
	}

	// Generate realistic handheld camera rotation noise
	XMVECTOR generateHandheldRotationNoise(float time, float intensity, float breathingRate, float driftSpeed) {
		// Base rotation scale - increased for more noticeable rotation drift
		float rotationScale = intensity * 0.02f;  // Increased from 0.015f

		// Apply drift speed parameter to all drift frequencies
		// Very slow drift rotation with asymmetric motion between axes
		float driftPitch = fractalBrownianMotion(time * driftSpeed, 3, 0.5f) * rotationScale * 4.0f;        // Increased multiplier
		float driftYaw = fractalBrownianMotion(time * (driftSpeed * 0.7f) + 432.1f, 3, 0.5f) * rotationScale * 4.5f; // Increased multiplier
		float driftRoll = fractalBrownianMotion(time * (driftSpeed * 0.4f) + 123.4f, 3, 0.5f) * rotationScale * 1.5f; // Less roll but still noticeable

		// Secondary slow drift components with different frequencies for more organic motion
		float driftPitch2 = fractalBrownianMotion(time * (driftSpeed * 1.3f) + 567.1f, 2, 0.4f) * rotationScale * 1.5f;
		float driftYaw2 = fractalBrownianMotion(time * (driftSpeed * 1.1f) + 267.8f, 2, 0.4f) * rotationScale * 1.3f;

		// Breathing affects rotation (mid-frequency)
		float breathingScale = rotationScale * 1.5f;  // Reduced from 2.0f to emphasize drift more
		float breathingPitch = sin(time * breathingRate * 0.7f) * breathingScale;
		float breathingYaw = cos(time * breathingRate * 0.4f + 0.3f) * breathingScale * 0.5f;

		// Small tremor/jitter in rotation (high-frequency) - reduced compared to drift
		float jitterScale = rotationScale * 0.3f;  // Reduced from 0.4f to make drift more apparent
		float jitterPitch = fractalBrownianMotion(time * 3.0f + 234.5f, 2, 0.7f) * jitterScale;
		float jitterYaw = fractalBrownianMotion(time * 3.2f + 637.2f, 2, 0.7f) * jitterScale;
		float jitterRoll = fractalBrownianMotion(time * 2.8f + 916.3f, 2, 0.7f) * jitterScale * 0.3f;

		// Combine all rotational components with primary + secondary drift
		float pitch = driftPitch + driftPitch2 + breathingPitch + jitterPitch;
		float yaw = driftYaw + driftYaw2 + breathingYaw + jitterYaw;
		float roll = driftRoll + jitterRoll;

		// Apply a subtle bias to create a tendency for specific directions in longer shots
		// This simulates the gradual arm fatigue that creates directional bias
		static float biasX = 0.0f, biasY = 0.0f;
		if (fabs(biasX) < 0.001f && fabs(biasY) < 0.001f) {
			// Initialize bias randomly but only once
			// We want a consistent directional bias per camera path
			biasX = (fractalBrownianMotion(time * 0.1f, 1, 0.5f) - 0.5f) * 0.03f;
			biasY = (fractalBrownianMotion(time * 0.1f + 100.0f, 1, 0.5f) - 0.5f) * 0.02f;
		}

		// Apply subtle bias over time
		float biasWeight = min(time * 0.005f, 0.5f);  // Gradually increase bias influence
		pitch += biasX * biasWeight * intensity;
		yaw += biasY * biasWeight * intensity;

		// Convert Euler angles to quaternion
		return Utils::generateEulerQuaternion(XMFLOAT3(pitch, yaw, roll),MULTIPLICATION_ORDER,false, false, false);
	}

	// Physics-based camera simulation with spring-damping system
	XMVECTOR simulateHandheldCamera(float deltaTime, float driftIntensity, float jitterIntensity,
		float breathingIntensity, XMVECTOR& currentVelocity, float driftSpeed) {
		// Constants for spring-damper system - reduced dampening for more natural drift
		const float springConstant = 5.0f;  // Reduced from 5.0f to allow more drift
		const float dampingFactor = 2.0f;   // Reduced from 2.0f to allow more motion

		// Generate random target position using Perlin noise
		static float time = 0.0f;
		time += deltaTime;

		// Generate target position using noise - using drift speed parameter
		float driftX = perlinNoise2D(time * driftSpeed, time * (driftSpeed * 0.5f)) * driftIntensity * 3.0f;
		float driftY = perlinNoise2D(time * (driftSpeed * 0.9f) + 100.0f, time * (driftSpeed * 0.4f) + 300.0f) * driftIntensity * 3.5f;
		float driftZ = perlinNoise2D(time * (driftSpeed * 0.7f) + 200.0f, time * (driftSpeed * 0.3f) + 700.0f) * driftIntensity * 2.5f;

		// Add breathing motion - slightly increased
		float breathingY = sin(time * 0.9f) * breathingIntensity * 1.2f;
		float breathingX = cos(time * 0.6f) * breathingIntensity * 0.5f;
		float breathingZ = sin(time * 0.7f + 0.4f) * breathingIntensity * 0.4f;

		const float jitterScalar = 8.0f;

		// Add jitter (hand tremor) - reduced relative to drift
		float jitterX = perlinNoise1D(time * jitterScalar) * jitterIntensity;
		float jitterY = perlinNoise1D(time * jitterScalar + 500.0f) * jitterIntensity;
		float jitterZ = perlinNoise1D(time * jitterScalar + 1000.0f) * jitterIntensity * 0.5f;

		// Combine all components
		XMVECTOR targetPosition = XMVectorSet(
			driftX + breathingX + jitterX,
			driftY + breathingY + jitterY,
			driftZ + breathingZ + jitterZ,
			0.0f
		);

		// Apply spring-damper physics
		static XMVECTOR currentPosition = XMVectorZero();

		// Calculate spring force: -k*x where x is displacement from target
		XMVECTOR displacement = XMVectorSubtract(currentPosition, targetPosition);
		XMVECTOR springForce = XMVectorScale(displacement, -springConstant);

		// Calculate damping force: -c*v where v is velocity
		XMVECTOR dampingForce = XMVectorScale(currentVelocity, -dampingFactor);

		// Calculate total force
		XMVECTOR totalForce = XMVectorAdd(springForce, dampingForce);

		// Update velocity: v = v + a*dt where a = F
		currentVelocity = XMVectorAdd(currentVelocity, XMVectorScale(totalForce, deltaTime));

		// Update position: x = x + v*dt
		currentPosition = XMVectorAdd(currentPosition, XMVectorScale(currentVelocity, deltaTime));

		return currentPosition;
	}

}