#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include <DirectXMath.h>

namespace IGCS {
	void CentripetalGenerateArcTable(std::vector<float>& _arcLengthTableCentripetal, std::vector<float>& _paramTableCentripetal, const std::vector<CameraNode>& _nodes, const int& _samplesize, const CameraPath& path);
	XMVECTOR CentripetalPositionInterpolation(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
	XMVECTOR CentripetalRotationInterpolation(const float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
}
