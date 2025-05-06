#include "stdafx.h"
#include "CameraPath.h"
#include "RiemannCubic.h"
#include "PathUtils.h"

namespace IGCS
{
	//XMVECTOR RiemannCubicRotation4Node(float globalT, const std::vector<CameraNode>& _nodes, CameraPath& path)
	//{
	//	// Ensure we have at least two nodes for interpolation
	//	int totalSegments = (int)_nodes.size() - 1;
	//	if (totalSegments < 1)
	//	{
	//		// Fallback to identity quaternion if there aren't enough nodes
	//		return XMQuaternionIdentity();
	//	}

	//	// Clamp the segment index to a valid range.
	//	// Here, we assume globalT is defined so that each integer step represents one segment.
	//	int seg = static_cast<int>(floorf(globalT));
	//	if (seg < 0)
	//		seg = 0;
	//	if (seg >= totalSegments)
	//	{
	//		seg = totalSegments - 1;
	//		globalT = (float)totalSegments;
	//	}
	//	float tLocal = globalT - seg;
	//	int i0, i1, i2, i3; //create variables to be populated by our getNodeSequence function
	//	path.getnodeSequence(i0, i1, i2, i3, globalT); //populate the variables with the correct node indices
	//	//// For interpolation between Q1 and Q2, we choose:
	//	////   Q1 = starting endpoint, Q2 = ending endpoint,
	//	//// while Q0 (previous to Q1) and Q3 (after Q2) are used for computing tangents.
	//	//int i0 = max(seg - 1, 0);                           // Q0: previous node (clamped)
	//	//int i1 = seg;                                     // Q1: starting endpoint
	//	//int i2 = min(seg + 1, (int)_nodes.size() - 1);      // Q2: ending endpoint
	//	//int i3 = min(seg + 2, (int)_nodes.size() - 1);      // Q3: next node (clamped)

	//	// Retrieve keyframe rotations and normalize them
	//	XMVECTOR Q0 = XMQuaternionNormalize(_nodes[i0].rotation);
	//	XMVECTOR Q1 = XMQuaternionNormalize(_nodes[i1].rotation);
	//	XMVECTOR Q2 = XMQuaternionNormalize(_nodes[i2].rotation);
	//	XMVECTOR Q3 = XMQuaternionNormalize(_nodes[i3].rotation);

	//	// Enforce continuity between quaternions by checking adjacent dot products.
	//	if (XMVectorGetX(XMQuaternionDot(Q0, Q1)) < 0.0f)
	//		Q0 = XMVectorNegate(Q0);
	//	if (XMVectorGetX(XMQuaternionDot(Q1, Q2)) < 0.0f)
	//		Q2 = XMVectorNegate(Q2);
	//	if (XMVectorGetX(XMQuaternionDot(Q2, Q3)) < 0.0f)
	//		Q3 = XMVectorNegate(Q3);

	//	// Convert quaternions to log space
	//	XMVECTOR logQ0 = XMQuaternionLog(Q0);
	//	XMVECTOR logQ1 = XMQuaternionLog(Q1);
	//	XMVECTOR logQ2 = XMQuaternionLog(Q2);
	//	XMVECTOR logQ3 = XMQuaternionLog(Q3);

	//	// Compute tangents at Q1 and Q2 using Q0 and Q3 respectively.
	//	// Tangent at Q1 (d1) is computed as 0.5*(logQ2 - logQ0)
	//	// Tangent at Q2 (d2) is computed as 0.5*(logQ3 - logQ1)
	//	XMVECTOR d1 = XMVectorScale(XMVectorSubtract(logQ2, logQ0), Globals::instance().settings().alphaValue);
	//	XMVECTOR d2 = XMVectorScale(XMVectorSubtract(logQ3, logQ1), Globals::instance().settings().alphaValue);

	//	// Optional: Clamp the tangents to prevent overly large derivatives
	//	auto clampTangent = [](XMVECTOR tangent, float maxMagnitude) -> XMVECTOR {
	//		float magnitude = XMVectorGetX(XMVector3Length(tangent));
	//		if (magnitude > maxMagnitude)
	//		{
	//			float scale = maxMagnitude / magnitude;
	//			return XMVectorScale(tangent, scale);
	//		}
	//		return tangent;
	//		};

	//	d1 = clampTangent(d1, 1.0f);
	//	d2 = clampTangent(d2, 1.0f);

	//	// Perform cubic Hermite interpolation in log space between Q1 and Q2
	//	float t2 = tLocal * tLocal;
	//	float t3 = t2 * tLocal;
	//	XMVECTOR interpLog = logQ1
	//	+ tLocal * d1
	//	+ t2 * (3.0f * (logQ2 - logQ1) - 2.0f * d1 - d2)
	//	+ t3 * (2.0f * (logQ1 - logQ2) + d1 + d2);

	//	// Exponentiate back to quaternion space and normalize the result
	//	XMVECTOR interpRot = XMQuaternionExp(interpLog);
	//	return XMQuaternionNormalize(interpRot);
	//}


	//XMVECTOR RiemannCubicRotationMonotonic(float globalT, const std::vector<CameraNode>& _nodes, CameraPath& path)
	//{
	//	// Ensure we have at least two nodes for interpolation
	//	int totalSegments = (int)_nodes.size() - 1;
	//	if (totalSegments < 1)
	//	{
	//		// Fallback to identity quaternion if there aren't enough nodes
	//		return XMQuaternionIdentity();
	//	}

	//	// Clamp the segment index to a valid range.
	//	// Here, we assume globalT is defined so that each integer step represents one segment.
	//	int seg = static_cast<int>(floorf(globalT));
	//	if (seg < 0)
	//		seg = 0;
	//	if (seg >= totalSegments)
	//	{
	//		seg = totalSegments - 1;
	//		globalT = (float)totalSegments;
	//	}
	//	float tLocal = globalT - seg;

	//	// For interpolation between Q1 and Q2, choose:
	//	//   Q1 = starting endpoint, Q2 = ending endpoint,
	//	// while Q0 (preceding Q1) and Q3 (following Q2) are used for tangent calculations.
	//	int i0 = max(seg - 1, 0);                           // Q0: previous node (clamped)
	//	int i1 = seg;                                      // Q1: starting endpoint
	//	int i2 = min(seg + 1, (int)_nodes.size() - 1);       // Q2: ending endpoint
	//	int i3 = min(seg + 2, (int)_nodes.size() - 1);       // Q3: next node (clamped)

	//	// Retrieve keyframe rotations and normalize them
	//	XMVECTOR Q0 = XMQuaternionNormalize(_nodes[i0].rotation);
	//	XMVECTOR Q1 = XMQuaternionNormalize(_nodes[i1].rotation);
	//	XMVECTOR Q2 = XMQuaternionNormalize(_nodes[i2].rotation);
	//	XMVECTOR Q3 = XMQuaternionNormalize(_nodes[i3].rotation);

	//	// Enforce continuity between quaternions by checking adjacent dot products.
	//	if (XMVectorGetX(XMQuaternionDot(Q0, Q1)) < 0.0f)
	//		Q0 = XMVectorNegate(Q0);
	//	if (XMVectorGetX(XMQuaternionDot(Q1, Q2)) < 0.0f)
	//		Q2 = XMVectorNegate(Q2);
	//	if (XMVectorGetX(XMQuaternionDot(Q2, Q3)) < 0.0f)
	//		Q3 = XMVectorNegate(Q3);

	//	// Convert quaternions to log space
	//	XMVECTOR logQ0 = XMQuaternionLog(Q0);
	//	XMVECTOR logQ1 = XMQuaternionLog(Q1);
	//	XMVECTOR logQ2 = XMQuaternionLog(Q2);
	//	XMVECTOR logQ3 = XMQuaternionLog(Q3);

	//	// Compute initial tangents for Q1 and Q2 using Q0 and Q3 respectively.
	//	// Tangent at Q1 (d1) is computed as 0.5*(logQ2 - logQ0)
	//	// Tangent at Q2 (d2) is computed as 0.5*(logQ3 - logQ1)
	//	XMVECTOR d1 = XMVectorScale(XMVectorSubtract(logQ2, logQ0), Globals::instance().settings().alphaValue);
	//	XMVECTOR d2 = XMVectorScale(XMVectorSubtract(logQ3, logQ1), Globals::instance().settings().alphaValue);

	//	// --- Enforce monotonicity on the tangents ---
	//	// Compute the chord between Q1 and Q2 in log space.
	//	XMVECTOR delta = XMVectorSubtract(logQ2, logQ1);
	//	float deltaMag = XMVectorGetX(XMVector3Length(delta));
	//	const float epsilon = 1e-5f;
	//	if (deltaMag > epsilon)
	//	{
	//		// Get the unit direction of the chord.
	//		XMVECTOR deltaUnit = XMVectorScale(delta, 1.0f / deltaMag);

	//		// Project the tangents onto the chord direction.
	//		float d1Proj = XMVectorGetX(XMVector3Dot(d1, deltaUnit));
	//		float d2Proj = XMVectorGetX(XMVector3Dot(d2, deltaUnit));

	//		// If the projected tangents oppose the chord, set them to zero.
	//		if (d1Proj < 0.0f)
	//			d1 = XMVectorZero();
	//		if (d2Proj < 0.0f)
	//			d2 = XMVectorZero();

	//		// If the sum of the projected tangents is too large relative to the chord,
	//		// scale them down to avoid overshooting.
	//		if ((d1Proj + d2Proj) > (3.0f * deltaMag))
	//		{
	//			float scale = (3.0f * deltaMag) / (d1Proj + d2Proj);
	//			d1 = XMVectorScale(d1, scale);
	//			d2 = XMVectorScale(d2, scale);
	//		}
	//	}
	//	else
	//	{
	//		// If the chord is too small, set tangents to zero.
	//		d1 = XMVectorZero();
	//		d2 = XMVectorZero();
	//	}

	//	// --- Optional: Clamp the overall tangent magnitudes to prevent excessive derivatives ---
	//	auto clampTangent = [](XMVECTOR tangent, float maxMagnitude) -> XMVECTOR {
	//		float magnitude = XMVectorGetX(XMVector3Length(tangent));
	//		if (magnitude > maxMagnitude)
	//		{
	//			float scale = maxMagnitude / magnitude;
	//			return XMVectorScale(tangent, scale);
	//		}
	//		return tangent;
	//		};

	//	d1 = clampTangent(d1, 2.0f);
	//	d2 = clampTangent(d2, 2.0f);

	//	// Perform cubic Hermite interpolation in log space between Q1 and Q2 using the monotonic tangents.
	//	float t2 = tLocal * tLocal;
	//	float t3 = t2 * tLocal;
	//	XMVECTOR interpLog = logQ1
	//		+ tLocal * d1
	//		+ t2 * (3.0f * (logQ2 - logQ1) - 2.0f * d1 - d2)
	//		+ t3 * (2.0f * (logQ1 - logQ2) + d1 + d2);

	//	// Exponentiate back to quaternion space and normalize the result.
	//	XMVECTOR interpRot = XMQuaternionExp(interpLog);
	//	return XMQuaternionNormalize(interpRot);
	//}


	// 1. Improved Numerical Stability for Quaternion Log operation
	XMVECTOR XMQuaternionLogImproved(XMVECTOR q)
	{
		// Normalize input (safety)
		q = XMQuaternionNormalize(q);

		// Extract scalar part (w component)
		float cosHalfAngle = XMVectorGetW(q);
		// Clamp to valid domain to prevent numerical issues
		cosHalfAngle = std::clamp(cosHalfAngle, -1.0f, 1.0f);

		// Extract vector part (x,y,z components)
		XMVECTOR v = XMVectorSetW(q, 0.0f);

		// Check if we're near identity
		float sinHalfAngleSq = XMVectorGetX(XMVector3LengthSq(v));
		const float EPSILON = 1e-6f;

		if (sinHalfAngleSq < EPSILON)
		{
			// For very small rotations, use first-order approximation
			return v;
		}
		else
		{
			float halfAngle = acosf(cosHalfAngle);
			float sinHalfAngle = sqrtf(sinHalfAngleSq);
			return XMVectorScale(v, halfAngle / sinHalfAngle);
		}
	}

	// 2. Adaptive Tension Parameter calculation
	float ComputeAdaptiveTension(XMVECTOR logQ0, XMVECTOR logQ1, XMVECTOR logQ2)
	{
		// Calculate local curvature estimate
		XMVECTOR dir1 = XMVectorSubtract(logQ1, logQ0);
		XMVECTOR dir2 = XMVectorSubtract(logQ2, logQ1);

		float len1 = XMVectorGetX(XMVector3Length(dir1));
		float len2 = XMVectorGetX(XMVector3Length(dir2));

		if (len1 < 1e-5f || len2 < 1e-5f) return 0.5f; // Default for small segments

		// Normalize directions
		dir1 = XMVectorScale(dir1, 1.0f / len1);
		dir2 = XMVectorScale(dir2, 1.0f / len2);

		// Measure directional change (dot product of unit vectors)
		float alignment = XMVectorGetX(XMVector3Dot(dir1, dir2));

		// Scale alpha based on alignment: 
		// - When well-aligned (alignment near 1.0), use higher tension (~0.5)
		// - When perpendicular or opposite, use lower tension (~0.1-0.2)
		return 0.2f + 0.3f * (alignment + 1.0f) / 2.0f;
	}

	// 4. Torque-Minimizing Approach for tangent calculation
	XMVECTOR ComputeTorqueMinimalTangent(XMVECTOR Q0, XMVECTOR Q1, XMVECTOR Q2)
	{
		// Ensure shortest path between quaternions
		if (XMVectorGetX(XMQuaternionDot(Q0, Q1)) < 0.0f)
			Q0 = XMVectorNegate(Q0);
		if (XMVectorGetX(XMQuaternionDot(Q1, Q2)) < 0.0f)
			Q2 = XMVectorNegate(Q2);

		// Get the angular velocity from Q0 to Q1 and Q1 to Q2
		XMVECTOR omega1 = XMQuaternionLogImproved(
			XMQuaternionMultiply(XMQuaternionInverse(Q0), Q1));
		XMVECTOR omega2 = XMQuaternionLogImproved(
			XMQuaternionMultiply(XMQuaternionInverse(Q1), Q2));

		// Scale by segment length (assuming unit time intervals)
		omega1 = XMVectorScale(omega1, 2.0f);
		omega2 = XMVectorScale(omega2, 2.0f);

		// Create a smooth transition between omega1 and omega2
		// This minimizes angular acceleration
		XMVECTOR blendedOmega = XMVectorLerp(omega1, omega2, 0.5f);

		// The tangent in the log space at Q1
		return blendedOmega;
	}

	// Helper function to clamp tangent magnitude
	XMVECTOR ClampTangent2(XMVECTOR tangent, float maxMagnitude)
	{
		float magnitude = XMVectorGetX(XMVector3Length(tangent));
		if (magnitude > maxMagnitude)
		{
			float scale = maxMagnitude / magnitude;
			return XMVectorScale(tangent, scale);
		}
		return tangent;
	}

	// Updated Riemann Cubic Rotation function with improvements
	XMVECTOR RiemannCubicRotation4Node(float globalT, const std::vector<CameraNode>& _nodes, CameraPath& path)
	{
		// Ensure we have at least two nodes for interpolation
		int totalSegments = (int)_nodes.size() - 1;
		if (totalSegments < 1)
		{
			// Fallback to identity quaternion if there aren't enough nodes
			return XMQuaternionIdentity();
		}

		// Clamp the segment index to a valid range.
		int seg = static_cast<int>(floorf(globalT));
		if (seg < 0)
			seg = 0;
		if (seg >= totalSegments)
		{
			seg = totalSegments - 1;
			globalT = (float)totalSegments;
		}
		float tLocal = globalT - seg;

		int i0, i1, i2, i3; // Node indices for interpolation
		path.getnodeSequence(i0, i1, i2, i3, globalT); // Get node sequence from path

		// Retrieve keyframe rotations and normalize them
		XMVECTOR Q0 = XMQuaternionNormalize(_nodes[i0].rotation);
		XMVECTOR Q1 = XMQuaternionNormalize(_nodes[i1].rotation);
		XMVECTOR Q2 = XMQuaternionNormalize(_nodes[i2].rotation);
		XMVECTOR Q3 = XMQuaternionNormalize(_nodes[i3].rotation);

		// Enforce continuity between quaternions by checking adjacent dot products.
		if (XMVectorGetX(XMQuaternionDot(Q0, Q1)) < 0.0f)
			Q0 = XMVectorNegate(Q0);
		if (XMVectorGetX(XMQuaternionDot(Q1, Q2)) < 0.0f)
			Q2 = XMVectorNegate(Q2);
		if (XMVectorGetX(XMQuaternionDot(Q2, Q3)) < 0.0f)
			Q3 = XMVectorNegate(Q3);

		// Convert quaternions to improved log space
		XMVECTOR logQ0 = XMQuaternionLogImproved(Q0);
		XMVECTOR logQ1 = XMQuaternionLogImproved(Q1);
		XMVECTOR logQ2 = XMQuaternionLogImproved(Q2);
		XMVECTOR logQ3 = XMQuaternionLogImproved(Q3);

		// IMPROVEMENT 2: Use adaptive tension instead of global alpha
		float alpha1 = ComputeAdaptiveTension(logQ0, logQ1, logQ2);
		float alpha2 = ComputeAdaptiveTension(logQ1, logQ2, logQ3);

		// IMPROVEMENT 4: Use torque-minimal tangent calculation
		XMVECTOR d1 = ComputeTorqueMinimalTangent(Q0, Q1, Q2);
		XMVECTOR d2 = ComputeTorqueMinimalTangent(Q1, Q2, Q3);

		// Scale tangents by adaptive tension
		d1 = XMVectorScale(d1, alpha1);
		d2 = XMVectorScale(d2, alpha2);

		// Clamp tangents to prevent excessive derivatives
		d1 = ClampTangent2(d1, 1.0f);
		d2 = ClampTangent2(d2, 1.0f);

		// Perform cubic Hermite interpolation in log space between Q1 and Q2
		float t2 = tLocal * tLocal;
		float t3 = t2 * tLocal;
		XMVECTOR interpLog = logQ1
			+ tLocal * d1
			+ t2 * (3.0f * (logQ2 - logQ1) - 2.0f * d1 - d2)
			+ t3 * (2.0f * (logQ1 - logQ2) + d1 + d2);

		// Exponentiate back to quaternion space and normalize the result
		XMVECTOR interpRot = XMQuaternionExp(interpLog);
		return XMQuaternionNormalize(interpRot);
	}

	XMVECTOR RiemannCubicRotationMonotonic(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		// Ensure we have at least two nodes for interpolation
		int totalSegments = (int)_nodes.size() - 1;
		if (totalSegments < 1)
		{
			// Fallback to identity quaternion if there aren't enough nodes
			return XMQuaternionIdentity();
		}

		// Clamp the segment index to a valid range.
		int seg = static_cast<int>(floorf(globalT));
		if (seg < 0)
			seg = 0;
		if (seg >= totalSegments)
		{
			seg = totalSegments - 1;
			globalT = (float)totalSegments;
		}
		float tLocal = globalT - seg;

		// For interpolation between Q1 and Q2, choose:
		//   Q1 = starting endpoint, Q2 = ending endpoint,
		// while Q0 (preceding Q1) and Q3 (following Q2) are used for tangent calculations.
		int i0, i1, i2, i3; // Node indices for interpolation
		path.getnodeSequence(i0, i1, i2, i3, globalT); // Get node sequence from path

		// Retrieve keyframe rotations and normalize them
		XMVECTOR Q0 = XMQuaternionNormalize(_nodes[i0].rotation);
		XMVECTOR Q1 = XMQuaternionNormalize(_nodes[i1].rotation);
		XMVECTOR Q2 = XMQuaternionNormalize(_nodes[i2].rotation);
		XMVECTOR Q3 = XMQuaternionNormalize(_nodes[i3].rotation);

		// Enforce continuity between quaternions by checking adjacent dot products.
		if (XMVectorGetX(XMQuaternionDot(Q0, Q1)) < 0.0f)
			Q0 = XMVectorNegate(Q0);
		if (XMVectorGetX(XMQuaternionDot(Q1, Q2)) < 0.0f)
			Q2 = XMVectorNegate(Q2);
		if (XMVectorGetX(XMQuaternionDot(Q2, Q3)) < 0.0f)
			Q3 = XMVectorNegate(Q3);

		// Convert quaternions to improved log space
		XMVECTOR logQ0 = XMQuaternionLogImproved(Q0);
		XMVECTOR logQ1 = XMQuaternionLogImproved(Q1);
		XMVECTOR logQ2 = XMQuaternionLogImproved(Q2);
		XMVECTOR logQ3 = XMQuaternionLogImproved(Q3);

		// IMPROVEMENT 2: Use adaptive tension instead of global alpha
		float alpha1 = ComputeAdaptiveTension(logQ0, logQ1, logQ2);
		float alpha2 = ComputeAdaptiveTension(logQ1, logQ2, logQ3);

		// IMPROVEMENT 4: Use torque-minimal tangent calculation but maintain monotonicity
		XMVECTOR d1 = ComputeTorqueMinimalTangent(Q0, Q1, Q2);
		XMVECTOR d2 = ComputeTorqueMinimalTangent(Q1, Q2, Q3);

		// Scale tangents by adaptive tension
		d1 = XMVectorScale(d1, alpha1);
		d2 = XMVectorScale(d2, alpha2);

		// --- Enforce monotonicity on the tangents ---
		// Compute the chord between Q1 and Q2 in log space.
		XMVECTOR delta = XMVectorSubtract(logQ2, logQ1);
		float deltaMag = XMVectorGetX(XMVector3Length(delta));
		const float epsilon = 1e-5f;
		if (deltaMag > epsilon)
		{
			// Get the unit direction of the chord.
			XMVECTOR deltaUnit = XMVectorScale(delta, 1.0f / deltaMag);

			// Project the tangents onto the chord direction.
			float d1Proj = XMVectorGetX(XMVector3Dot(d1, deltaUnit));
			float d2Proj = XMVectorGetX(XMVector3Dot(d2, deltaUnit));

			// If the projected tangents oppose the chord, set them to zero.
			if (d1Proj < 0.0f)
				d1 = XMVectorZero();
			if (d2Proj < 0.0f)
				d2 = XMVectorZero();

			// If the sum of the projected tangents is too large relative to the chord,
			// scale them down to avoid overshooting.
			if ((d1Proj + d2Proj) > (3.0f * deltaMag))
			{
				float scale = (3.0f * deltaMag) / (d1Proj + d2Proj);
				d1 = XMVectorScale(d1, scale);
				d2 = XMVectorScale(d2, scale);
			}
		}
		else
		{
			// If the chord is too small, set tangents to zero.
			d1 = XMVectorZero();
			d2 = XMVectorZero();
		}

		// Clamp the overall tangent magnitudes to prevent excessive derivatives
		d1 = ClampTangent2(d1, 2.0f);
		d2 = ClampTangent2(d2, 2.0f);

		// Perform cubic Hermite interpolation in log space between Q1 and Q2 using the monotonic tangents.
		float t2 = tLocal * tLocal;
		float t3 = t2 * tLocal;
		XMVECTOR interpLog = logQ1
			+ tLocal * d1
			+ t2 * (3.0f * (logQ2 - logQ1) - 2.0f * d1 - d2)
			+ t3 * (2.0f * (logQ1 - logQ2) + d1 + d2);

		// Exponentiate back to quaternion space and normalize the result.
		XMVECTOR interpRot = XMQuaternionExp(interpLog);
		return XMQuaternionNormalize(interpRot);
	}


}