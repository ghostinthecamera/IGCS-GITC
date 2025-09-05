#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include "MessageHandler.h"
#include <DirectXMath.h>

namespace IGCS
{
    /**
     * @brief Performs quaternion interpolation between camera nodes using Riemann cubic interpolation
     * @param globalT Global parameter value (integer part selects segment, fractional part is local parameter)
     * @param _nodes Vector of camera nodes to interpolate between
     * @param path Reference to the camera path containing the nodes
     * @return Interpolated rotation as a quaternion
     */
    XMVECTOR RiemannCubicRotation4Node(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);

    /**
     * @brief Performs quaternion interpolation between camera nodes using Riemann cubic interpolation with monotonicity constraints
     * @param globalT Global parameter value (integer part selects segment, fractional part is local parameter)
     * @param _nodes Vector of camera nodes to interpolate between
     * @param path Reference to the camera path containing the nodes
     * @return Interpolated rotation as a quaternion
     */
    XMVECTOR RiemannCubicRotationMonotonic(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
}