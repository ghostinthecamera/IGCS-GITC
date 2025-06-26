#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include <DirectXMath.h>

namespace IGCS
{
	void ComputeKnotVector(const std::vector<CameraNode>& _nodes, std::vector<float>& _knotVector);
	void ComputeControlPoints(const std::vector<CameraNode>& _nodes, std::vector<XMVECTOR>& _controlPointsPos, std::vector<XMVECTOR>& _controlPointsRot, std::vector<float>& _controlPointsFOV);
	float BSplineBasis(size_t i, int p, float t, const std::vector<float>& knots);
	XMVECTOR BSplinePositionInterpolation(float t, const std::vector<float>& _knotVector, const std::vector<XMVECTOR>& _controlPointsPos);
	XMVECTOR BSplineRotationInterpolation(float t, const std::vector<float>& _knotVector, const std::vector<XMVECTOR>& _controlPointsRot);
	void BSplineGenerateArcLengthTable(std::vector<float>& _arcLengthTableBspline, std::vector<float>& _paramTableBspline, const std::vector<CameraNode>&
	                                   _nodes, const int& _samplesize, std::vector<float>& _knotVector, std::vector<XMVECTOR>& _controlPointsPos, std::vector<
	                                   XMVECTOR>& _controlPointsRot, std::vector<float>& _controlPointsFOV);
	float BSplineFoV(float t, const std::vector<float>& _knotVector, const std::vector<float>& _controlPointsFOV);
	float BSplineBasisDeriv(size_t i, int p, float t, const std::vector<float>& knots);
	static XMVECTOR BSplineDerivative(const float t, const std::vector<float>& _knotVector, const std::vector<XMVECTOR>& _controlPointsPos);
}
