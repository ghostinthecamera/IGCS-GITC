#include "stdafx.h"
#include "MessageHandler.h"
#include <DirectXMath.h>
#include "Globals.h"
#include "BSpline.h"
#include "PathUtils.h"

namespace IGCS
{
	void ComputeKnotVector(const std::vector<CameraNode>& _nodes, std::vector<float>& _knotVector)
	{
		size_t n = _nodes.size();
		if (n < 4)
		{
			// Not enough control points for a cubic B-spline
			_knotVector.clear();
			return;
		}

		//// Clamped uniform knot vector for cubic B-spline
		//_knotVector.resize(n + 4);
		//for (size_t i = 0; i < 4; i++)
		//{
		//	_knotVector[i] = 0.0f; // Clamped at the start
		//	_knotVector[n + i] = static_cast<float>(n - 3); // Clamped at the end
		//}
		//for (size_t i = 4; i < n; i++)
		//{
		//	_knotVector[i] = static_cast<float>(i - 3);
		//}
		// Resize knot vector for a cubic B-spline
		_knotVector.resize(n + 4);

		// Initialize all knots to prevent uninitialized values
		for (size_t i = 0; i < _knotVector.size(); i++)
		{
			_knotVector[i] = 0.0f;
		}

		// Set first 4 knots to 0 (minimum value)
		for (size_t i = 0; i < 4; i++)
		{
			_knotVector[i] = 0.0f;
		}

		// Set last 4 knots to (n-3) (maximum value)
		for (size_t i = 0; i < 4; i++)
		{
			_knotVector[n + i] = static_cast<float>(n - 3);
		}

		// Set interior knots (indices 4 to n-1) with values 1 to (n-4)
		for (size_t i = 4; i < n; i++)
		{
			_knotVector[i] = static_cast<float>(i - 3);
		}
	}

	void ComputeControlPoints(const std::vector<CameraNode>& _nodes, std::vector<XMVECTOR>& _controlPointsPos, std::vector<XMVECTOR>& _controlPointsRot, std::vector<float>& _controlPointsFOV)
	{
		size_t n = _nodes.size();
		_controlPointsPos.resize(n);
		_controlPointsRot.resize(n);
		_controlPointsFOV.resize(n);

		for (size_t i = 0; i < n; i++)
		{
			_controlPointsPos[i] = _nodes[i].position;
			_controlPointsRot[i] = _nodes[i].rotation;
			_controlPointsFOV[i] = _nodes[i].fov;

			// Ensure continuity between consecutive quaternions
			if (i > 0)
			{
				PathUtils::EnsureQuaternionContinuity(_controlPointsRot[i - 1], _controlPointsRot[i]);
			}
		}
	}

	float BSplineBasis(size_t i, int p, float t, const std::vector<float>& knots)
	{
		
		if (p == 0)
		{
			return (t >= knots[i] && t < knots[i + 1]) ? 1.0f : 0.0f;
		}
		else
		{
			float denom1 = knots[i + p] - knots[i];
			float denom2 = knots[i + p + 1] - knots[i + 1];
			float term1 = (denom1 > 1e-6f) ? ((t - knots[i]) / denom1) * BSplineBasis(i, p - 1, t, knots) : 0.0f;
			float term2 = (denom2 > 1e-6f) ? ((knots[i + p + 1] - t) / denom2) * BSplineBasis(i + 1, p - 1, t, knots) : 0.0f;
			return term1 + term2;
		}
	}

	XMVECTOR BSplinePositionInterpolation(float t, const std::vector<float>& _knotVector, const std::vector<XMVECTOR>& _controlPointsPos)
	{
		const size_t n = _controlPointsPos.size();
		if (n < 4 || _knotVector.size() <= n) return XMVectorZero();

		// Safe parameter domain bounds
		const float tMin = _knotVector[3]; // Start parameter at 4th knot
		const float tMax = (n < _knotVector.size()) ? _knotVector[n] : _knotVector.back();

		// Ensure t is within valid domain
		t = max(tMin, min(t, tMax));

		// Safe endpoint handling
		if (fabs(t - tMax) < 1e-5f && n > 0) {
			return _controlPointsPos[n - 1]; // Last control point
		}

		// Regular B-spline evaluation
		XMVECTOR result = XMVectorZero();
		for (size_t i = 0; i < n; i++) {
			float b = BSplineBasis(i, 3, t, _knotVector);
			result = XMVectorAdd(result, XMVectorScale(_controlPointsPos[i], b));
		}
		return result;
	}

	XMVECTOR BSplineRotationInterpolation(float t, const std::vector<float>& _knotVector, const std::vector<XMVECTOR>& _controlPointsRot)
	{
		// Basic validity checks
		const size_t n = _controlPointsRot.size();
		if (n < 4 || _knotVector.size() <= n)
		{
			// Fallback if not enough points or invalid knot vector
			return XMVectorSet(0, 0, 0, 1);  // identity quaternion
		}

		// For a clamped, cubic B-spline with n control points,
		// the valid parameter domain is [knotVector[3] .. knotVector[n]].
		const float tMin = _knotVector[3];
		const float tMax = (n < _knotVector.size()) ? _knotVector[n] : _knotVector.back();

		// Ensure t stays within [tMin, tMax]
		t = max(tMin, min(t, tMax));

		// If t is extremely close to tMax, just return the last rotation
		if (fabs(t - tMax) < 1e-5f && n > 0)
		{
			return _controlPointsRot[n - 1];
		}

		// Standard B-spline evaluation for rotation (degree=3)
		XMVECTOR result = XMVectorZero();
		for (size_t i = 0; i < n; i++)
		{
			const float basis = BSplineBasis(i, 3, t, _knotVector);
			result = XMVectorAdd(result, XMVectorScale(_controlPointsRot[i], basis));
		}

		// Normalize the quaternion to avoid drift over time
		return XMQuaternionNormalize(result);
	}

	void BSplineGenerateArcLengthTable(std::vector<float>& _arcLengthTableBspline, std::vector<float>& _paramTableBspline,
	                                   const std::vector<CameraNode>& _nodes, const int& _samplesize, std::vector<float>& _knotVector,
	                                   std::vector<XMVECTOR>& _controlPointsPos, std::vector<XMVECTOR>& _controlPointsRot, std::vector<float>&
	                                   _controlPointsFOV)
	{
		ComputeControlPoints(_nodes, _controlPointsPos, _controlPointsRot, _controlPointsFOV);
		ComputeKnotVector(_nodes, _knotVector);

		// Check
		if (_controlPointsPos.size() < 4 || _knotVector.empty())
		{
			MessageHandler::logDebug("Not enough control points or empty knot vector for B-spline arc table. Returning.");
			_arcLengthTableBspline.clear();
			_paramTableBspline.clear();
			return;
		}

		// clear old
		_arcLengthTableBspline.clear();
		_paramTableBspline.clear();

		float tMin = _knotVector[3];                // start
		float tMax = _knotVector[_controlPointsPos.size()]; // end

		if (tMax <= tMin)
		{
			MessageHandler::logDebug("Invalid knot range. Returning.");
			return;
		}

		// How many subdivisions to do for integration
		int NUM_STEPS = _samplesize;  // you can bump this up for more accuracy
		float dt = (tMax - tMin) / static_cast<float>(NUM_STEPS);

		float accumulatedDist = 0.0f;

		// param= tMin
		_arcLengthTableBspline.push_back(accumulatedDist);
		_paramTableBspline.push_back(tMin);

		// Evaluate derivative at the start
		XMVECTOR prevDeriv = BSplineDerivative(tMin, _knotVector, _controlPointsPos);
		float prevSpeed = XMVectorGetX(XMVector3Length(prevDeriv));

		float currentT = tMin;

		for (int i = 1; i <= NUM_STEPS; i++)
		{
			//float nextT = tMin + dt * (float)i;
			float nextT = (i == NUM_STEPS) ? tMax : (tMin + dt * (float)i);

			// derivative at nextT
			XMVECTOR nextDeriv = BSplineDerivative(nextT, _knotVector, _controlPointsPos);
			float nextSpeed = XMVectorGetX(XMVector3Length(nextDeriv));

			// trapezoid rule over [currentT, nextT]
			float distSegment = 0.5f * (prevSpeed + nextSpeed) * dt;
			accumulatedDist += distSegment;

			// store
			_arcLengthTableBspline.push_back(accumulatedDist);
			_paramTableBspline.push_back(nextT);

			// shift
			currentT = nextT;
			prevDeriv = nextDeriv;
			prevSpeed = nextSpeed;
		}

		if (fabs(_paramTableBspline.back() - tMax) > 1e-6f) {
			//// The last parameter isn't exactly at tMax, so add a point at exactly tMax
			//_paramTableBspline.push_back(tMax);
			//// Keep the same arc length as the previous point (approximation for small dt)
			//_arcLengthTableBspline.push_back(_arcLengthTableBspline.back());

			// Alternative: compute actual arc length to tMax for higher precision
			XMVECTOR finalDeriv = BSplineDerivative(tMax, _knotVector, _controlPointsPos);
			float finalSpeed = XMVectorGetX(XMVector3Length(finalDeriv));
			float finalSegment = 0.5f * (prevSpeed + finalSpeed) * (tMax - currentT);
			_arcLengthTableBspline.push_back(_arcLengthTableBspline.back() + finalSegment);
		}

		float totalLen = _arcLengthTableBspline.back();
		//MessageHandler::logDebug("B-spline Arc length table (derivative-based) created. Total path length: %f", totalLen);
	}

	float BSplineFoV(float t, const std::vector<float>& _knotVector, const std::vector<float>& _controlPointsFOV)
	{
		size_t n = _controlPointsFOV.size();
		if (n < 4 || _knotVector.size() <= n)
		{
			// Fallback for not enough points or invalid knot vector
			if (n > 0)
			{
				return _controlPointsFOV[0];  // just return the first FOV
			}
			else
			{
				return 60.0f;  // or some default
			}
		}

		// The domain for a fully-clamped cubic B-spline is [knotVector[3] .. knotVector[n]]
		float tMin = _knotVector[3];
		float tMax = (n < _knotVector.size()) ? _knotVector[n] : _knotVector.back();

		// Clamp the incoming t to [tMin, tMax]
		t = max(tMin, min(t, tMax));

		// If we are extremely close to tMax, just use the last control point's FOV.
		// That avoids floating-point edge cases where the basis function might behave oddly.
		if (fabs(t - tMax) < 1e-5f && n > 0)
		{
			return _controlPointsFOV[n - 1];
		}

		// Otherwise, do the standard B-spline evaluation with the cubic basis
		float result = 0.0f;
		for (size_t i = 0; i < n; i++)
		{
			float b = BSplineBasis(i, 3, t, _knotVector);
			result += _controlPointsFOV[i] * b;
		}
		return result;
	}

	// --------------------------------------------------------------
	// 2) Derivative of the B-spline basis function
	//    Formula: d/dt [N_{i,p}(t)] = p / [knots[i+p]-knots[i]] * N_{i,p-1}(t)
	//                               - p / [knots[i+p+1]-knots[i+1]] * N_{i+1,p-1}(t)
	// --------------------------------------------------------------
	float BSplineBasisDeriv(size_t i, int p, float t, const std::vector<float>& knots)
	{
		if (p == 0)
		{
			// derivative of a 0th-degree basis is 0
			return 0.0f;
		}

		float denom1 = knots[i + p] - knots[i];
		float denom2 = knots[i + p + 1] - knots[i + 1];

		float left = 0.0f;
		float right = 0.0f;

		// left term
		if (fabs(denom1) > 1e-9f)
		{
			left = static_cast<float>(p) / denom1 * BSplineBasis(i, p - 1, t, knots);
		}
		// right term
		if (fabs(denom2) > 1e-9f)
		{
			right = static_cast<float>(p) / denom2 * BSplineBasis(i + 1, p - 1, t, knots);
			// note the minus sign
			right = -right;
		}

		return left + right;
	}

	// --------------------------------------------------------------
	// 4) Evaluate B-spline derivative: C'(t) = sum_i [controlPoints[i] * d/dt(N_{i,p}(t))]
	// --------------------------------------------------------------
	static XMVECTOR BSplineDerivative(const float t, const std::vector<float>& _knotVector, const std::vector<XMVECTOR>& _controlPointsPos)
	{
		XMVECTOR result = XMVectorZero();
		size_t n = _controlPointsPos.size();
		if (n < 4 || _knotVector.empty())
		{
			return result;
		}

		// p=3 for cubic
		for (size_t i = 0; i < n; i++)
		{
		    float dBasis = BSplineBasisDeriv(i, 3, t, _knotVector);
		    result = XMVectorAdd(result, XMVectorScale(_controlPointsPos[i], dBasis));
		}
		return result;

	}

}