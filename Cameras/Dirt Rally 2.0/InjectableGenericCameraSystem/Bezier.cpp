#include "stdafx.h"
#include "MessageHandler.h"
#include "CentripetalCatmullRom.h"
#include <DirectXMath.h>
#include "Globals.h"
#include "Bezier.h"
#include "PathUtils.h"

namespace IGCS
{

	void BezierGenerateArcLengthTable(std::vector<float>& _arcLengthTableBezier, std::vector<float>& _paramTableBezier, const std::vector<CameraNode>& _nodes, const
	                                  int& _samplesize)
	{
		_arcLengthTableBezier.clear();
		_paramTableBezier.clear();

		int SAMPLES_PER_SEGMENT = _samplesize;
		float accumulatedDist = 0.0f;

		// Sample starting at globalT = 0
		XMVECTOR prevPos = BezierPositionInterpolation(0.0f, _nodes);
		_arcLengthTableBezier.push_back(accumulatedDist);
		_paramTableBezier.push_back(0.0f);

		int totalSegments = (int)_nodes.size() - 1;
		for (int seg = 0; seg < totalSegments; ++seg)
		{
			for (int step = 1; step <= SAMPLES_PER_SEGMENT; ++step)
			{
				float localT = static_cast<float>(step) / SAMPLES_PER_SEGMENT;
				float globalT = seg + localT;
				XMVECTOR curPos = BezierPositionInterpolation(globalT, _nodes);
				float dist = XMVectorGetX(XMVector3Length(curPos - prevPos));
				accumulatedDist += dist;
				_arcLengthTableBezier.push_back(accumulatedDist);
				_paramTableBezier.push_back(globalT);
				prevPos = curPos;
			}
		}
		//MessageHandler::logDebug("Bezier Arc length table created. Total path length: %f", _arcLengthTableBezier.back());
	}

	// ***** NEW CODE: Helper to compute Bezier control points for position (and FOV)
	BezierSegment getBezierSegmentForPosition(size_t segmentIndex, const std::vector<CameraNode>& _nodes)
	{
		BezierSegment seg;
		size_t n = _nodes.size();
		// Endpoints: c0 and c3
		seg.c0 = _nodes[segmentIndex].position;
		seg.c3 = _nodes[segmentIndex + 1].position;

		// Compute tangent at node[segmentIndex]
		XMVECTOR tan0;
		if (segmentIndex == 0)
			tan0 = XMVectorSubtract(_nodes[1].position, _nodes[0].position);
		else
			tan0 = XMVectorScale(XMVectorAdd(XMVectorSubtract(_nodes[segmentIndex + 1].position, _nodes[segmentIndex - 1].position), XMVectorZero()), 0.5f);
		// Use 1/3 of the chord length to position control point
		seg.c1 = XMVectorAdd(seg.c0, XMVectorScale(tan0, 1.0f / 3.0f));

		// Compute tangent at node[segmentIndex+1]
		XMVECTOR tan1;
		if (segmentIndex + 2 >= n)
			tan1 = XMVectorSubtract(_nodes[n - 1].position, _nodes[n - 2].position);
		else
			tan1 = XMVectorScale(XMVectorAdd(XMVectorSubtract(_nodes[segmentIndex + 2].position, _nodes[segmentIndex].position), XMVectorZero()), 0.5f);
		seg.c2 = XMVectorSubtract(seg.c3, XMVectorScale(tan1, 1.0f / 3.0f));

		return seg;
	}

	// ***** NEW CODE: Cubic Bezier interpolation for position
	XMVECTOR BezierPositionInterpolation(float globalT, const std::vector<CameraNode>& _nodes)
	{
		int totalSegments = static_cast<int>(_nodes.size() - 1);
		if (totalSegments < 1)
			return XMVectorZero();

		int seg = static_cast<int>(floorf(globalT));
		seg = max(0, seg);
		if (seg >= totalSegments) { seg = totalSegments - 1; globalT = static_cast<float>(totalSegments); }
		float t = globalT - static_cast<float>(seg); // local parameter in [0,1]

		// Get control points for this segment
		BezierSegment bs = getBezierSegmentForPosition(seg, _nodes);

		float u = 1.0f - t;
		float u2 = u * u;
		float u3 = u2 * u;
		float t2 = t * t;
		float t3 = t2 * t;

		// Standard cubic Bezier evaluation: B(t) = u^3*c0 + 3*u^2*t*c1 + 3*u*t^2*c2 + t^3*c3
		XMVECTOR result = XMVectorScale(bs.c0, u3);
		result = XMVectorAdd(result, XMVectorScale(bs.c1, 3 * u2 * t));
		result = XMVectorAdd(result, XMVectorScale(bs.c2, 3 * u * t2));
		result = XMVectorAdd(result, XMVectorScale(bs.c3, t3));
		return result;
	}

	// ***** NEW CODE: Cubic Bezier interpolation for FOV (scalar)
	float BezierFoV(float globalT, const std::vector<CameraNode>& _nodes)
	{
		int totalSegments = static_cast<int>(_nodes.size() - 1);
		if (totalSegments < 1)
			return (_nodes.empty() ? 60.0f : _nodes[0].fov);

		int seg = static_cast<int>(floorf(globalT));
		seg = max(0, seg);
		if (seg >= totalSegments) { seg = totalSegments - 1; globalT = static_cast<float>(totalSegments); }
		float t = globalT - static_cast<float>(seg);

		// For FOV, we compute control points similar to position:
		float p0 = _nodes[seg].fov;
		float p3 = _nodes[seg + 1].fov;
		float tan0, tan1;
		if (seg == 0)
			tan0 = p3 - p0;
		else
			tan0 = (p3 - _nodes[seg - 1].fov) * 0.5f;
		if (seg + 2 >= _nodes.size())
			tan1 = p3 - p0;
		else
			tan1 = (_nodes[seg + 2].fov - p0) * 0.5f;
		float p1 = p0 + tan0 / 3.0f;
		float p2 = p3 - tan1 / 3.0f;

		float u = 1.0f - t;
		float u3 = u * u * u;
		float t3 = t * t * t;
		float result = u3 * p0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t3 * p3;
		return result;
	}

	// ***** NEW CODE: Compute squad control quaternion for rotation
	XMVECTOR computeSquadControl(const XMVECTOR& qm1, const XMVECTOR& q0, const XMVECTOR& q1)
	{
		XMVECTOR invQ0 = XMQuaternionInverse(q0);
		XMVECTOR logTerm1 = PathUtils::XMQuaternionLog(XMQuaternionMultiply(invQ0, qm1));
		XMVECTOR logTerm2 = PathUtils::XMQuaternionLog(XMQuaternionMultiply(invQ0, q1));
		XMVECTOR avg = XMVectorScale(XMVectorAdd(logTerm1, logTerm2), -0.25f);
		XMVECTOR control = XMQuaternionMultiply(q0, XMQuaternionExp(avg));
		return XMQuaternionNormalize(control);
	}

	// ***** NEW CODE: SQUAD function for quaternion Bezier interpolation
	XMVECTOR squad(const XMVECTOR& q0, const XMVECTOR& q1, const XMVECTOR& s0, const XMVECTOR& s1, float t)
	{
		// First, do spherical linear interpolations (slerp)
		XMVECTOR slerp1 = XMQuaternionSlerp(q0, q1, t);
		XMVECTOR slerp2 = XMQuaternionSlerp(s0, s1, t);

		//if (XMVectorGetX(XMQuaternionDot(slerp1, slerp2)) < 0.0f)
		//	slerp2 = XMVectorNegate(slerp2);
		PathUtils::EnsureQuaternionContinuity(slerp1, slerp2);

		// Then slerp between these two results with factor 2t(1-t)
		float blend = 2 * t * (1 - t);
		XMVECTOR result = XMQuaternionSlerp(slerp1, slerp2, blend);
		return XMQuaternionNormalize(result);
	}

	// ***** NEW CODE: Cubic Bezier interpolation for rotation using SQUAD
	XMVECTOR BezierRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes)
	{
		int totalSegments = (int)_nodes.size() - 1;
		if (totalSegments < 1)
			return XMQuaternionIdentity();

		int seg = static_cast<int>(floorf(globalT));
		if (seg < 0) seg = 0;
		if (seg >= totalSegments) { seg = totalSegments - 1; globalT = (float)totalSegments; }
		float t = globalT - seg;

		// End quaternions for this segment
		XMVECTOR q0 = XMQuaternionNormalize(_nodes[seg].rotation);
		XMVECTOR q1 = XMQuaternionNormalize(_nodes[seg + 1].rotation);
		// Enforce continuity (avoid flips)
		//if (XMVectorGetX(XMQuaternionDot(q0, q1)) < 0.0f)
		//	q1 = XMVectorNegate(q1);

		// Get previous and next quaternions for tangent estimation (one-sided at boundaries)
		XMVECTOR qm1 = (seg == 0) ? q0 : XMQuaternionNormalize(_nodes[seg - 1].rotation);
		if (seg != 0 && XMVectorGetX(XMQuaternionDot(qm1, q0)) < 0.0f)
			qm1 = XMVectorNegate(qm1);

		XMVECTOR q2 = (seg + 2 < _nodes.size()) ? XMQuaternionNormalize(_nodes[seg + 2].rotation) : q1;
		if (seg + 2 < _nodes.size() && XMVectorGetX(XMQuaternionDot(q1, q2)) < 0.0f)
			q2 = XMVectorNegate(q2);

		// Compute squad control points for q0 and q1
		XMVECTOR s0 = computeSquadControl(qm1, q0, q1);
		XMVECTOR s1 = computeSquadControl(q0, q1, q2);

		// Evaluate squad (which is equivalent to a quaternion cubic Bezier) at t
		return squad(q0, q1, s0, s1, t);
	}



}
