#include "stdafx.h"
#include "MessageHandler.h"
#include "CentripetalCatmullRom.h"
#include <DirectXMath.h>
#include "Globals.h"
#include "PathUtils.h"


namespace IGCS
{
	void CentripetalGenerateArcTable(std::vector<float>& _arcLengthTableCentripetal, std::vector<float>& _paramTableCentripetal, const std::vector<CameraNode>& _nodes, const int& _samplesize, const CameraPath& path)
	{
		if (_nodes.size() < 2)
		{
			MessageHandler::logError("Not enough nodes, returning from centripetal arc table generation");
			_arcLengthTableCentripetal.clear();
			_paramTableCentripetal.clear();
			return;
		}

		_arcLengthTableCentripetal.clear();
		_paramTableCentripetal.clear();

		int SAMPLES_PER_SEGMENT = _samplesize;  // Increase sample count if needed
		float accumulatedDist = 0.0f;

		// Evaluate the centripetal position at the start of the path.
		XMVECTOR prevPos = CentripetalPositionInterpolation(0.0f,_nodes, path);
		_arcLengthTableCentripetal.push_back(accumulatedDist);
		_paramTableCentripetal.push_back(0.0f);  // globalT = 0

		int totalSegments = (int)_nodes.size() - 1;
		for (int segIndex = 0; segIndex < totalSegments; ++segIndex)
		{
			for (int step = 1; step <= SAMPLES_PER_SEGMENT; ++step)
			{
				float localT = (float)step / (float)SAMPLES_PER_SEGMENT; // [0,1]
				float globalT = segIndex + localT; // overall parameter

				// Evaluate using the updated centripetal interpolation.
				XMVECTOR curPos = CentripetalPositionInterpolation(globalT, _nodes, path);

				// Compute distance from previous sample.
				float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(curPos, prevPos)));
				accumulatedDist += dist;

				_arcLengthTableCentripetal.push_back(accumulatedDist);
				_paramTableCentripetal.push_back(globalT);

				prevPos = curPos;
			}
		}

		float totalLen = _arcLengthTableCentripetal.back();
		//MessageHandler::logDebug("Centripetal Arc length table created. Total path length: %f", totalLen);
		//MessageHandler::logDebug("Centripetal param length table created. Total path length: %f", _paramTableCentripetal.back());
		//MessageHandler::logDebug("Size of Centripetal Arc length table: %zu b", _arcLengthTableCentripetal.size() * sizeof(float));
		//MessageHandler::logDebug("Size of Centripetal param table: %zu b", _paramTableCentripetal.size() * sizeof(float)); // Corrected "paramh" to "param"
	}

	//XMVECTOR CentripetalPositionInterpolation(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	//{
	//	// Retrieve the four node indices and compute the local t value.
	//	static const float EPSILON = 1e-8f;

	//	int i0, i1, i2, i3;
	//	float t = globalT - path.getsegmentIndex(globalT);
	//	path.getnodeSequence(i0, i1, i2, i3, globalT);

	//	// Get positions from the node array.
	//	XMVECTOR p0 = _nodes[i0].position;
	//	XMVECTOR p1 = _nodes[i1].position;
	//	XMVECTOR p2 = _nodes[i2].position;
	//	XMVECTOR p3 = _nodes[i3].position;

	//	// 1) Compute raw chord lengths
	//	float d01 = XMVectorGetX(XMVector3Length(XMVectorSubtract(p1, p0)));
	//	float d12 = XMVectorGetX(XMVector3Length(XMVectorSubtract(p2, p1)));
	//	float d23 = XMVectorGetX(XMVector3Length(XMVectorSubtract(p3, p2)));

	//	// 2) Clamp them so we never take sqrt(0) => 0
	//	if (d01 < EPSILON) d01 = EPSILON;
	//	if (d12 < EPSILON) d12 = EPSILON;
	//	if (d23 < EPSILON) d23 = EPSILON;

	//	// 3) Compute the centripetal parameter values (t0, t1, t2, t3).
	//	float t0 = 0.0f;
	//	float t1 = t0 + powf(d01, 0.5f);
	//	float t2 = t1 + powf(d12, 0.5f);
	//	float t3 = t2 + powf(d23, 0.5f);

	//	// 4) Safeguard intervals for denominators in the Lerp calls
	//	auto safeDenominator = [&](float denom) {
	//		return (fabsf(denom) < EPSILON) ? EPSILON : denom;
	//		};

	//	float denom_t1_t0 = safeDenominator(t1 - t0);
	//	float denom_t2_t1 = safeDenominator(t2 - t1);
	//	float denom_t3_t2 = safeDenominator(t3 - t2);
	//	float denom_t2_t0 = safeDenominator(t2 - t0);

	//	// 5) Map the local parameter t in [0,1] to the interval [t1, t2].
	//	float u = t1 + t * (t2 - t1);  // safe enough, since (t2 - t1) won't be zero

	//	// 6) De Casteljau-like interpolation steps, using safe denominators
	//	float A1_alpha = (u - t0) / denom_t1_t0;
	//	XMVECTOR A1 = XMVectorLerp(p0, p1, A1_alpha);

	//	float A2_alpha = (u - t1) / denom_t2_t1;
	//	XMVECTOR A2 = XMVectorLerp(p1, p2, A2_alpha);

	//	float A3_alpha = (u - t2) / denom_t3_t2;
	//	XMVECTOR A3 = XMVectorLerp(p2, p3, A3_alpha);

	//	float B1_alpha = (u - t0) / denom_t2_t0;
	//	XMVECTOR B1 = XMVectorLerp(A1, A2, B1_alpha);

	//	float B2_alpha = (u - t1) / safeDenominator(t3 - t1);
	//	XMVECTOR B2 = XMVectorLerp(A2, A3, B2_alpha);

	//	// final Lerp
	//	float C_alpha = (u - t1) / denom_t2_t1;
	//	XMVECTOR C = XMVectorLerp(B1, B2, C_alpha);

	//	return C;
	//}

	XMVECTOR CentripetalPositionInterpolation(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		//--------------------------------------------------------------------------
		// 1) Identify the "segment" from globalT, get its four control‐point indices,
		//    and the local parameter t in [0,1] for that segment.
		//--------------------------------------------------------------------------
		// Suppose path.getsegmentIndex(globalT) returns an integer k, so that
		//   globalT = k + localT   with localT in [0,1].
		// Then path.getnodeSequence(i0, i1, i2, i3, globalT) fills out the indices
		//   for the relevant segment so we know P0..P3.
		//--------------------------------------------------------------------------
		int i0, i1, i2, i3;
		path.getnodeSequence(i0, i1, i2, i3, globalT);

		// localT in [0,1] for this segment
		float localT = globalT - float(path.getsegmentIndex(globalT));

		// Retrieve the 4 positions
		XMVECTOR P0 = _nodes[i0].position;
		XMVECTOR P1 = _nodes[i1].position;
		XMVECTOR P2 = _nodes[i2].position;
		XMVECTOR P3 = _nodes[i3].position;

		//--------------------------------------------------------------------------
		// 2) Compute chord lengths^(0.5).  This is the key difference vs. uniform CR.
		//--------------------------------------------------------------------------
		//   d01 = sqrt( length( P1 - P0 ) )
		//   d12 = sqrt( length( P2 - P1 ) )
		//   d23 = sqrt( length( P3 - P2 ) )
		//--------------------------------------------------------------------------
		XMVECTOR vP10 = XMVectorSubtract(P1, P0);
		float d01 = XMVectorGetX(XMVector3Length(vP10));
		d01 = sqrtf(d01);

		XMVECTOR vP21 = XMVectorSubtract(P2, P1);
		float d12 = XMVectorGetX(XMVector3Length(vP21));
		d12 = sqrtf(d12);

		XMVECTOR vP32 = XMVectorSubtract(P3, P2);
		float d23 = XMVectorGetX(XMVector3Length(vP32));
		d23 = sqrtf(d23);

		//--------------------------------------------------------------------------
		// 3) Non‐uniform knot values: t0=0, t1=d01, t2=d01+d12, t3=d01+d12+d23.
		//    We map localT in [0,1] => "T" in [t1,t2], then re‐map to u in [0,1].
		//--------------------------------------------------------------------------
		float t0 = 0.0f;
		float t1 = d01;
		float t2 = d01 + d12;
		float t3 = d01 + d12 + d23;

		float denom = (t2 - t1);
		float T = t1 + localT * denom;   // T in [t1, t2]
		float u = (T - t1) / denom;      // final parameter in [0,1]

		//--------------------------------------------------------------------------
		// 4) Compute tangents M1 at P1 and M2 at P2.
		//    M1 = (t2 - t1)/(t2 - t0)*(P1 - P0) + (t1 - t0)/(t2 - t0)*(P2 - P1)
		//    M2 = (t3 - t2)/(t3 - t1)*(P2 - P1) + (t2 - t1)/(t3 - t1)*(P3 - P2)
		//--------------------------------------------------------------------------
		float denomP1 = (t2 - t0);
		XMVECTOR part1 = XMVectorScale(vP10, (t2 - t1) / denomP1);
		XMVECTOR part2 = XMVectorScale(vP21, (t1 - t0) / denomP1);
		XMVECTOR M1 = XMVectorAdd(part1, part2);

		float denomP2 = (t3 - t1);
		XMVECTOR part3 = XMVectorScale(vP21, (t3 - t2) / denomP2);
		XMVECTOR part4 = XMVectorScale(vP32, (t2 - t1) / denomP2);
		XMVECTOR M2 = XMVectorAdd(part3, part4);

		//--------------------------------------------------------------------------
		// 5) Standard cubic Hermite blend for u in [0,1].
		//    h1(u) =  2u^3 - 3u^2 + 1
		//    h2(u) = -2u^3 + 3u^2
		//    h3(u) =  u^3 - 2u^2 + u
		//    h4(u) =  u^3 -    u^2
		//--------------------------------------------------------------------------
		float u2 = u * u;
		float u3 = u2 * u;

		float h1 = 2.f * u3 - 3.f * u2 + 1.f;  // for P1
		float h2 = -2.f * u3 + 3.f * u2;        // for P2
		float h3 = u3 - 2.f * u2 + u; // for M1
		float h4 = u3 - u2;     // for M2

		//--------------------------------------------------------------------------
		// 6) Final combination: S(u) = h1*P1 + h2*P2 + h3*M1 + h4*M2
		//--------------------------------------------------------------------------
		XMVECTOR result = XMVectorScale(P1, h1);
		result = XMVectorAdd(result, XMVectorScale(P2, h2));
		result = XMVectorAdd(result, XMVectorScale(M1, h3));
		result = XMVectorAdd(result, XMVectorScale(M2, h4));

		return result;
	}

	XMVECTOR CentripetalRotationInterpolation(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		// Choose an epsilon small enough to avoid noticeable distortion but large enough
		// to prevent floating-point underflow.
		static const float EPSILON = 1e-8f;
		float alpha = Globals::instance().settings().alphaValue;

		// Retrieve node indices and local parameter.
		int i0, i1, i2, i3;
		float t = globalT - path.getsegmentIndex(globalT);
		path.getnodeSequence(i0, i1, i2, i3, globalT);

		// Get and normalize the quaternions.
		XMVECTOR q0 = XMQuaternionNormalize(_nodes[i0].rotation);
		XMVECTOR q1 = XMQuaternionNormalize(_nodes[i1].rotation);
		XMVECTOR q2 = XMQuaternionNormalize(_nodes[i2].rotation);
		XMVECTOR q3 = XMQuaternionNormalize(_nodes[i3].rotation);

		// Ensure quaternion continuity (flip sign if necessary).
		if (XMVectorGetX(XMQuaternionDot(q0, q1)) < 0.0f) q1 = XMVectorNegate(q1);
		if (XMVectorGetX(XMQuaternionDot(q1, q2)) < 0.0f) q2 = XMVectorNegate(q2);
		if (XMVectorGetX(XMQuaternionDot(q2, q3)) < 0.0f) q3 = XMVectorNegate(q3);

		// Define geodesic (angular) distance between quaternions.
		auto GeoDistance = [](const XMVECTOR& a, const XMVECTOR& b) -> float {
			float dotVal = XMVectorGetX(XMQuaternionDot(a, b));
			dotVal = fabsf(dotVal);
			// dotVal should be in [0,1]. (acos outside [0..1] => NaN)
			dotVal = min(max(dotVal, 0.0f), 1.0f);
			return 2.0f * acosf(dotVal); // angle in [0..PI]
			};

		// 1) Compute geodesic distances and clamp them to EPSILON
		float d01 = GeoDistance(q0, q1);
		float d12 = GeoDistance(q1, q2);
		float d23 = GeoDistance(q2, q3);

		if (d01 < EPSILON) d01 = EPSILON;
		if (d12 < EPSILON) d12 = EPSILON;
		if (d23 < EPSILON) d23 = EPSILON;

		// 2) Compute knot values for centripetal interpolation
		float t0 = 0.0f;
		float t1 = t0 + powf(d01, alpha);
		float t2 = t1 + powf(d12, alpha);
		float t3 = t2 + powf(d23, alpha);

		// Safeguard denominators to avoid division by zero
		auto safeDenominator = [&](float denom) {
			return (fabsf(denom) < EPSILON) ? EPSILON : denom;
			};

		float denom_t1_t0 = safeDenominator(t1 - t0);
		float denom_t2_t1 = safeDenominator(t2 - t1);
		float denom_t3_t2 = safeDenominator(t3 - t2);
		float denom_t2_t0 = safeDenominator(t2 - t0);
		float denom_t3_t1 = safeDenominator(t3 - t1);

		// 3) Map local t in [0,1] to the interval [t1, t2].
		float u = t1 + t * (t2 - t1); // won't be zero if (t2 - t1) isn't zero.

		// 4) De Casteljau-like slerp steps
		float A1alpha = (u - t0) / denom_t1_t0;
		XMVECTOR A1 = XMQuaternionSlerp(q0, q1, A1alpha);

		float A2alpha = (u - t1) / denom_t2_t1;
		XMVECTOR A2 = XMQuaternionSlerp(q1, q2, A2alpha);

		float A3alpha = (u - t2) / denom_t3_t2;
		XMVECTOR A3 = XMQuaternionSlerp(q2, q3, A3alpha);

		A1 = XMQuaternionNormalize(A1);
		A2 = XMQuaternionNormalize(A2);
		A3 = XMQuaternionNormalize(A3);

		float B1alpha = (u - t0) / denom_t2_t0;
		XMVECTOR B1 = XMQuaternionSlerp(A1, A2, B1alpha);

		float B2alpha = (u - t1) / denom_t3_t1;
		XMVECTOR B2 = XMQuaternionSlerp(A2, A3, B2alpha);

		B1 = XMQuaternionNormalize(B1);
		B2 = XMQuaternionNormalize(B2);

		float Calpha = (u - t1) / denom_t2_t1;
		XMVECTOR C = XMQuaternionSlerp(B1, B2, Calpha);

		// Final normalize to ensure valid quaternion
		return XMQuaternionNormalize(C);
	}

}