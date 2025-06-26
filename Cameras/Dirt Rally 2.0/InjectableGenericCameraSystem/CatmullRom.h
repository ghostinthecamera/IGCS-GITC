#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include <DirectXMath.h>

namespace IGCS {
       XMVECTOR CatmullRomPositionInterpolation(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
       XMVECTOR CatmullRomRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
       float CatmullRomFoV(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
       void CatmullRomGenerateArcTable(std::vector<float>& _arcLengthTable, std::vector<float>& _paramTable, const std::vector<CameraNode>& _nodes,const int _samplesize,const CameraPath& path);
}