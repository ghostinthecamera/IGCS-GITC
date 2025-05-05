#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include "MessageHandler.h"
#include <DirectXMath.h>

namespace IGCS 
{
	//XMVECTOR RiemannCubicRotation4Node(float globalT, const std::vector<CameraNode>& _nodes, CameraPath& path);
	//XMVECTOR RiemannCubicRotationMonotonic(float globalT, const std::vector<CameraNode>& _nodes, CameraPath& path);


	XMVECTOR XMQuaternionLogImproved(XMVECTOR q);

	float ComputeAdaptiveTension(XMVECTOR logQ0, XMVECTOR logQ1, XMVECTOR logQ2);

	XMVECTOR ComputeTorqueMinimalTangent(XMVECTOR Q0, XMVECTOR Q1, XMVECTOR Q2);

	XMVECTOR ClampTangent2(XMVECTOR tangent, float maxMagnitude);

	XMVECTOR RiemannCubicRotation4Node(float globalT, const std::vector<CameraNode>& _nodes, CameraPath& path);

	XMVECTOR RiemannCubicRotationMonotonic(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);

}
