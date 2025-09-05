#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include <DirectXMath.h>

namespace IGCS
{
	XMVECTOR		BezierPositionInterpolation(float globalT, const std::vector<CameraNode>& _nodes);
	XMVECTOR		BezierRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes);
	float			BezierFoV(float globalT, const std::vector<CameraNode>& _nodes);
	void			BezierGenerateArcLengthTable(std::vector<float>& _arcLengthTableCentripetal, std::vector<float>& _paramTableCentripetal, const std::vector<
				                                 CameraNode>& _nodes, const int& _samplesize);
	struct			BezierSegment {XMVECTOR c0, c1, c2, c3;};
	BezierSegment	getBezierSegmentForPosition(size_t segmentIndex, const std::vector<CameraNode>& _nodes);
	XMVECTOR		computeSquadControl(const XMVECTOR& qm1, const XMVECTOR& q0, const XMVECTOR& q1);
	XMVECTOR		squad(const XMVECTOR& q0, const XMVECTOR& q1, const XMVECTOR& s0, const XMVECTOR& s1, float t);

}
