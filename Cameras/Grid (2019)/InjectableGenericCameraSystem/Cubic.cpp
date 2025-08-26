#include "stdafx.h"
#include "MessageHandler.h"
#include "CentripetalCatmullRom.h"
#include <DirectXMath.h>
#include "Globals.h"
#include "Cubic.h"
#include "PathUtils.h"

namespace IGCS
{
	// Main position interpolation function with the smooth approach
	XMVECTOR CubicPositionInterpolation_Smooth(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		// Get node indices and local parameter
		int i0, i1, i2, i3;
		float t = globalT - path.getsegmentIndex(globalT);
		path.getnodeSequence(i0, i1, i2, i3, globalT);

		// Get positions
		XMVECTOR p0 = _nodes[i0].position;
		XMVECTOR p1 = _nodes[i1].position;
		XMVECTOR p2 = _nodes[i2].position;
		XMVECTOR p3 = _nodes[i3].position;

		// Use smoother curve generation
		XMVECTOR pos, vel;
		position_catmull_rom_smooth(pos, vel, t, p0, p1, p2, p3);

		return pos;
	}

	// Smooth FOV interpolation that matches the position approach
	float CubicFoV_Smooth(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		// Get node indices and local parameter
		int i0, i1, i2, i3;
		float t = globalT - static_cast<float>(path.getsegmentIndex(globalT));
		path.getnodeSequence(i0, i1, i2, i3, globalT);

		// Extract FOV values
		float f0 = _nodes[i0].fov;
		float f1 = _nodes[i1].fov;
		float f2 = _nodes[i2].fov;
		float f3 = _nodes[i3].fov;

		// Calculate direct differences
		float d10 = f1 - f0;
		float d21 = f2 - f1;
		float d32 = f3 - f2;

		// Apply smoother constraint
		float v1 = cubic_mono_velocity_smooth(d10, d21);
		float v2 = cubic_mono_velocity_smooth(d21, d32);

		// Use hermite with the constrained velocities
		float fov, derivative;
		scalar_hermite(fov, derivative, t, f1, f2, v1, v2);

		return fov;
	}

	// Updated to use the new monotonic quaternion interpolation
	XMVECTOR CubicRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		// Get node indices and local parameter
		int i0, i1, i2, i3;
		float t = globalT - path.getsegmentIndex(globalT);
		path.getnodeSequence(i0, i1, i2, i3, globalT);

		// Retrieve quaternions from nodes
		XMVECTOR q0 = _nodes[i0].rotation;
		XMVECTOR q1 = _nodes[i1].rotation;
		XMVECTOR q2 = _nodes[i2].rotation;
		XMVECTOR q3 = _nodes[i3].rotation;

		// Use the monotonic catmull-rom interpolation
		XMVECTOR rot, vel;
		//quat_catmull_rom_mono(rot, vel, t, q0, q1, q2, q3);
		quat_catmull_rom_mono_improved(rot, vel, t, q0, q1, q2, q3);
		return rot;

		// Alternative non-monotonic version:
		 //quat_catmull_rom(rot, vel, t, q0, q1, q2, q3);
		 //return rot;

	}

	void CubicGenerateArcLengthTable(std::vector<float>& _arcLengthTableCubic, std::vector<float>& _paramTableCubic, const std::vector<CameraNode>& _nodes, const int& _samplesize, const CameraPath& path)
	{
		_arcLengthTableCubic.clear();
		_paramTableCubic.clear();

		int SAMPLES_PER_SEGMENT = _samplesize;
		float accumulatedDist = 0.0f;
		// Starting sample at globalT = 0
		XMVECTOR prevPos = CubicPositionInterpolation_Smooth(0.0f, _nodes, path);
		_arcLengthTableCubic.push_back(accumulatedDist);
		_paramTableCubic.push_back(0.0f);

		int totalSegments = static_cast<int>(_nodes.size() - 1);
		for (int seg = 0; seg < totalSegments; ++seg)
		{
			for (int step = 1; step <= SAMPLES_PER_SEGMENT; ++step)
			{
				float localT = static_cast<float>(step) / static_cast<float>(SAMPLES_PER_SEGMENT);
				float globalT = static_cast<float>(seg) + localT;
				XMVECTOR curPos = CubicPositionInterpolation_Smooth(globalT, _nodes, path);
				float dist = XMVectorGetX(XMVector3Length(curPos - prevPos));
				accumulatedDist += dist;
				_arcLengthTableCubic.push_back(accumulatedDist);
				_paramTableCubic.push_back(globalT);
				prevPos = curPos;
			}
		}
		//MessageHandler::logDebug("Cubic arc length table created. Total path length: %f", _arcLengthTableCubic.back());
	}

	//Helper functions for the above
	/////////////////

	// Position hermite interpolation
	void position_hermite(
		XMVECTOR& pos,
		XMVECTOR& vel,
		float x,
		XMVECTOR p0,
		XMVECTOR p1,
		XMVECTOR v0,
		XMVECTOR v1)
	{
		float w0 = 2.0f * x * x * x - 3.0f * x * x + 1.0f;
		float w1 = 3.0f * x * x - 2.0f * x * x * x;
		float w2 = x * x * x - 2.0f * x * x + x;
		float w3 = x * x * x - x * x;

		float q0 = 6.0f * x * x - 6.0f * x;
		float q1 = 6.0f * x - 6.0f * x * x;
		float q2 = 3.0f * x * x - 4.0f * x + 1.0f;
		float q3 = 3.0f * x * x - 2.0f * x;

		pos = XMVectorAdd(
			XMVectorAdd(
				XMVectorScale(p0, w0),
				XMVectorScale(p1, w1)),
			XMVectorAdd(
				XMVectorScale(v0, w2),
				XMVectorScale(v1, w3)));

		vel = XMVectorAdd(
			XMVectorAdd(
				XMVectorScale(p0, q0),
				XMVectorScale(p1, q1)),
			XMVectorAdd(
				XMVectorScale(v0, q2),
				XMVectorScale(v1, q3)));
	}

	// Standard Catmull-Rom for quaternions
	void quat_catmull_rom(
		XMVECTOR& rot,
		XMVECTOR& vel,
		float x,
		XMVECTOR r0,
		XMVECTOR r1,
		XMVECTOR r2,
		XMVECTOR r3)
	{
		// Ensure quaternions are normalized
		r0 = XMQuaternionNormalize(r0);
		r1 = XMQuaternionNormalize(r1);
		r2 = XMQuaternionNormalize(r2);
		r3 = XMQuaternionNormalize(r3);

		// Ensure shortest path
		if (XMVectorGetX(XMQuaternionDot(r0, r1)) < 0.0f) r0 = XMVectorNegate(r0);
		if (XMVectorGetX(XMQuaternionDot(r1, r2)) < 0.0f) r2 = XMVectorNegate(r2);
		if (XMVectorGetX(XMQuaternionDot(r2, r3)) < 0.0f) r3 = XMVectorNegate(r3);

		// Calculate relative rotations in angle-axis space
		XMVECTOR d10 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r0), r1));
		XMVECTOR d21 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r1), r2));
		XMVECTOR d32 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r2), r3));

		// Compute central difference velocities
		XMVECTOR v1 = XMVectorScale(XMVectorAdd(d10, d21), 0.5f);
		XMVECTOR v2 = XMVectorScale(XMVectorAdd(d21, d32), 0.5f);

		// Call hermite with the computed velocities
		quat_hermite(rot, vel, x, r1, r2, v1, v2);
	}

	// Scalar hermite interpolation
	void scalar_hermite(
		float& value,
		float& derivative,
		float x,
		float v0,
		float v1,
		float d0,
		float d1)
	{
		float t2 = x * x;
		float t3 = t2 * x;
		float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
		float h10 = t3 - 2.0f * t2 + x;
		float h01 = -2.0f * t3 + 3.0f * t2;
		float h11 = t3 - t2;

		float q0 = 6.0f * t2 - 6.0f * x;
		float q1 = 6.0f * x - 6.0f * t2;
		float q2 = 3.0f * t2 - 4.0f * x + 1.0f;
		float q3 = 3.0f * t2 - 2.0f * x;

		value = h00 * v0 + h10 * d0 + h01 * v1 + h11 * d1;
		derivative = q0 * v0 + q1 * v1 + q2 * d0 + q3 * d1;
	}

	// Convert scaled angle-axis back to quaternion
	XMVECTOR quat_from_scaled_angle_axis(XMVECTOR v)
	{
		float len = XMVectorGetX(XMVector3Length(v));

		// Handle zero rotation case
		if (len < 1e-6f)
			return XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		// Create quaternion from axis-angle
		XMVECTOR axis = XMVector3Normalize(v);
		float halfAngle = len * 0.5f;
		float sinHalfAngle = sinf(halfAngle);

		return XMVectorSet(
			XMVectorGetX(axis) * sinHalfAngle,
			XMVectorGetY(axis) * sinHalfAngle,
			XMVectorGetZ(axis) * sinHalfAngle,
			cosf(halfAngle));
	}

	// Hermite interpolation for quaternions
	void quat_hermite(
		XMVECTOR& rot,
		XMVECTOR& vel,
		float x,
		XMVECTOR r0,
		XMVECTOR r1,
		XMVECTOR v0,
		XMVECTOR v1)
	{
		float w1 = 3.0f * x * x - 2.0f * x * x * x;
		float w2 = x * x * x - 2.0f * x * x + x;
		float w3 = x * x * x - x * x;

		float q1 = 6.0f * x - 6.0f * x * x;
		float q2 = 3.0f * x * x - 4.0f * x + 1.0f;
		float q3 = 3.0f * x * x - 2.0f * x;

		// Get the quaternion difference and convert to scaled angle-axis
		XMVECTOR r1_sub_r0 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r0), r1));

		// Interpolate in angle-axis space, then convert back to quaternion
		XMVECTOR angle_axis_result = XMVectorAdd(
			XMVectorAdd(
				XMVectorScale(r1_sub_r0, w1),
				XMVectorScale(v0, w2)),
			XMVectorScale(v1, w3));

		// Convert back to quaternion and apply to r0
		rot = XMQuaternionMultiply(r0, quat_from_scaled_angle_axis(angle_axis_result));

		// Compute velocity
		vel = XMVectorAdd(
			XMVectorAdd(
				XMVectorScale(r1_sub_r0, q1),
				XMVectorScale(v0, q2)),
			XMVectorScale(v1, q3));
	}

	// Detect if rotation angle is below a threshold where SLERP should be used
	bool is_small_rotation(XMVECTOR q1, XMVECTOR q2) {
		float dotProduct = XMVectorGetX(XMQuaternionDot(q1, q2));
		// Ensure we're using the shortest path
		if (dotProduct < 0.0f) {
			dotProduct = -dotProduct;
		}

		// If dot product is close to 1, the rotation angle is small
		// acos(0.9999) ≈ 0.0141 radians ≈ 0.81 degrees
		return dotProduct > 0.9999f;
	}

	// Enhanced quaternion SLERP that avoids numerical issues with very small angles
	XMVECTOR quat_slerp_safe(XMVECTOR q1, XMVECTOR q2, float t) {
		// Ensure quaternions are normalized
		q1 = XMQuaternionNormalize(q1);
		q2 = XMQuaternionNormalize(q2);

		// Calculate dot product
		float dot = XMVectorGetX(XMQuaternionDot(q1, q2));

		// Ensure shortest path by negating quaternion if needed
		if (dot < 0.0f) {
			q2 = XMVectorNegate(q2);
			dot = -dot;
		}

		// If very close, use linear interpolation to avoid numerical issues
		if (dot > 0.9999f) {
			XMVECTOR result = XMVectorAdd(
				q1,
				XMVectorScale(XMVectorSubtract(q2, q1), t)
			);
			return XMQuaternionNormalize(result);
		}

		// Standard SLERP for normal cases
		float angle = acosf(dot);
		float sinAngle = sinf(angle);

		// Calculate scale factors
		float scale0 = sinf((1.0f - t) * angle) / sinAngle;
		float scale1 = sinf(t * angle) / sinAngle;

		// Interpolate
		XMVECTOR result = XMVectorAdd(
			XMVectorScale(q1, scale0),
			XMVectorScale(q2, scale1)
		);

		return result;
	}

	// Convert quaternion difference to scaled angle-axis with safeguards
	XMVECTOR quat_to_scaled_angle_axis_safe(XMVECTOR q) {
		// Extract the vector part of the quaternion
		XMVECTOR axis = XMVectorSet(
			XMVectorGetX(q),
			XMVectorGetY(q),
			XMVectorGetZ(q),
			0.0f);

		float axisLength = XMVectorGetX(XMVector3Length(axis));

		// Handle zero rotation case with higher precision threshold
		if (axisLength < 1e-7f) {
			return XMVectorZero();
		}

		// Compute the angle with higher precision for small rotations
		float w = XMVectorGetW(q);
		float angle;

		// Use different computation methods based on angle size for better precision
		if (w > 0.9999f) {
			// For very small angles, use a more accurate approximation
			angle = 2.0f * axisLength;
		}
		else {
			// Standard computation for larger angles
			angle = 2.0f * acosf(w);
		}

		// Scale the axis by the angle
		return XMVectorScale(XMVector3Normalize(axis), angle);
	}

	// Improved monotonic quaternion Catmull-Rom interpolation
	void quat_catmull_rom_mono_improved(
		XMVECTOR& rot,
		XMVECTOR& vel,
		float x,
		XMVECTOR r0,
		XMVECTOR r1,
		XMVECTOR r2,
		XMVECTOR r3)
	{
		// Ensure quaternions are normalized
		r0 = XMQuaternionNormalize(r0);
		r1 = XMQuaternionNormalize(r1);
		r2 = XMQuaternionNormalize(r2);
		r3 = XMQuaternionNormalize(r3);

		// Ensure shortest path
		if (XMVectorGetX(XMQuaternionDot(r0, r1)) < 0.0f) r0 = XMVectorNegate(r0);
		if (XMVectorGetX(XMQuaternionDot(r1, r2)) < 0.0f) r2 = XMVectorNegate(r2);
		if (XMVectorGetX(XMQuaternionDot(r2, r3)) < 0.0f) r3 = XMVectorNegate(r3);

		// Check if we're dealing with very small rotations where SLERP should be used
		if (is_small_rotation(r1, r2)) 
		{
			// For very small rotations, use SLERP directly to avoid numerical issues
			rot = quat_slerp_safe(r1, r2, x);

			// Calculate velocity by approximating the derivative
			const float h = 0.001f; // Small delta for numerical derivative
			XMVECTOR rotPlus = quat_slerp_safe(r1, r2, x + h);
			vel = XMQuaternionMultiply(
				XMQuaternionInverse(rot),
				XMVectorScale(quat_to_scaled_angle_axis_safe(
					XMQuaternionMultiply(XMQuaternionInverse(rot), rotPlus)
				), 1.0f / h)
			);

			return;
		}

		// Calculate relative rotations in angle-axis space with improved precision
		XMVECTOR d10 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r0), r1));
		XMVECTOR d21 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r1), r2));
		XMVECTOR d32 = quat_to_scaled_angle_axis_safe(XMQuaternionMultiply(XMQuaternionInverse(r2), r3));

		// Calculate chord lengths (rotation magnitudes) for non-uniform spacing
		float l0 = XMVectorGetX(XMVector3Length(d10));
		float l1 = XMVectorGetX(XMVector3Length(d21));
		float l2 = XMVectorGetX(XMVector3Length(d32));

		// Ensure non-zero lengths
		l0 = max(l0, 1e-6f);
		l1 = max(l1, 1e-6f);
		l2 = max(l2, 1e-6f);

		// Compute weighted derivatives with non-uniform spacing
		float alpha1 = l1 / (l0 + l1);
		float alpha2 = l1 / (l1 + l2);

		XMVECTOR v1 = XMVectorAdd(
			XMVectorScale(d10, (1.0f - alpha1)),
			XMVectorScale(d21, alpha1)
		);

		XMVECTOR v2 = XMVectorAdd(
			XMVectorScale(d21, (1.0f - alpha2)),
			XMVectorScale(d32, alpha2)
		);

		// Apply monotonicity constraints for each component
		for (int i = 0; i < 3; i++) {
			float d10i = XMVectorGetByIndex(d10, i);
			float d21i = XMVectorGetByIndex(d21, i);
			float d32i = XMVectorGetByIndex(d32, i);

			float v1i = XMVectorGetByIndex(v1, i);
			float v2i = XMVectorGetByIndex(v2, i);

			// Apply monotonicity constraint
			if (d10i * d21i <= 0.0f) {
				v1 = XMVectorSetByIndex(v1, 0.0f, i);
			}
			else {
				// Apply Fritsch-Carlson constraint
				float alpha = v1i / d21i;
				float beta = v2i / d21i;
				float tau = 3.0f;

				if (alpha * alpha + beta * beta > tau * tau) {
					float factor = tau / sqrt(alpha * alpha + beta * beta);
					v1 = XMVectorSetByIndex(v1, v1i * factor, i);
					v2 = XMVectorSetByIndex(v2, v2i * factor, i);
				}
			}
		}

		// Scale velocities by segment length for proper Hermite interpolation
		v1 = XMVectorScale(v1, 1.0f);
		v2 = XMVectorScale(v2, 1.0f);

		// Call original hermite with the computed velocities
		quat_hermite(rot, vel, x, r1, r2, v1, v2);
	}

	// Relaxed monotonic constraint for smoother curves
	// Balances monotonicity with smoothness
	float cubic_mono_velocity_smooth(float d0, float d1)
	{
		// If signs differ or either is zero, use a small blend instead of zero
		// This allows some minor non-monotonicity for smoother curves
		if ((d0 * d1) <= 0.0f) {
			// Small blend factor instead of strict zero
			return (d0 + d1) * 0.1f;
		}

		// Less aggressive constraint for smoother curves
		float avg = (d0 + d1) * 0.5f;

		// More relaxed constraint with higher multiplier (4.0 instead of 3.0)
		float maxVal = min(abs(d0) * 4.0f, abs(d1) * 4.0f);

		return (avg > 0.0f) ? min(avg, maxVal) : max(avg, -maxVal);
	}

	// Relaxed vector version of the monotonic velocity constraint
	XMVECTOR cubic_mono_velocity_smooth(XMVECTOR d0, XMVECTOR d1)
	{
		XMVECTOR result = XMVectorZero();

		for (int i = 0; i < 3; i++) {
			float d0i = XMVectorGetByIndex(d0, i);
			float d1i = XMVectorGetByIndex(d1, i);
			float vi = cubic_mono_velocity_smooth(d0i, d1i);
			result = XMVectorSetByIndex(result, vi, i);
		}

		return result;
	}

	// Smoother version of Catmull-Rom that balances monotonicity with smoothness
	void position_catmull_rom_smooth(
		XMVECTOR& pos,
		XMVECTOR& vel,
		float x,
		XMVECTOR p0,
		XMVECTOR p1,
		XMVECTOR p2,
		XMVECTOR p3)
	{
		// Calculate chord lengths for adaptive tension
		float chordLength = XMVectorGetX(XMVector3Length(XMVectorSubtract(p2, p1)));

		// Calculate direct differences
		XMVECTOR d10 = XMVectorSubtract(p1, p0);
		XMVECTOR d21 = XMVectorSubtract(p2, p1);
		XMVECTOR d32 = XMVectorSubtract(p3, p2);

		// Use smoother velocity constraints
		XMVECTOR v1 = cubic_mono_velocity_smooth(d10, d21);
		XMVECTOR v2 = cubic_mono_velocity_smooth(d21, d32);

		// Adaptive tension based on segment character
		// For short segments or sharp turns, use more relaxed constraints
		XMVECTOR d10n = XMVector3Normalize(d10);
		XMVECTOR d21n = XMVector3Normalize(d21);
		XMVECTOR d32n = XMVector3Normalize(d32);

		float dot1 = XMVectorGetX(XMVector3Dot(d10n, d21n));
		float dot2 = XMVectorGetX(XMVector3Dot(d21n, d32n));

		// Calculate blend factor based on angle between segments
		// Sharper turns get more relaxed constraints
		float angleFactor1 = (1.0f - dot1) * 0.5f;
		float angleFactor2 = (1.0f - dot2) * 0.5f;

		// Adaptive relaxation of constraints based on angle
		XMVECTOR standardV1 = XMVectorScale(XMVectorAdd(d10, d21), 0.5f);
		XMVECTOR standardV2 = XMVectorScale(XMVectorAdd(d21, d32), 0.5f);

		// Blend between constrained and unconstrained tangents
		v1 = XMVectorLerp(v1, standardV1, angleFactor1);
		v2 = XMVectorLerp(v2, standardV2, angleFactor2);

		// Use hermite with the adaptively constrained velocities
		position_hermite(pos, vel, x, p1, p2, v1, v2);
	}
}