#pragma once
#include "stdafx.h"
#include "CameraPath.h"
#include <DirectXMath.h>

namespace IGCS
{
	XMVECTOR CubicRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
	XMVECTOR CubicPositionInterpolation_Smooth(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
	float CubicFoV_Smooth(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path);
	void CubicGenerateArcLengthTable(std::vector<float>& _arcLengthTableCubic, std::vector<float>& _paramTableCubic, const std::vector<CameraNode>& _nodes, const int& _samplesize, const CameraPath& path);

	void quat_catmull_rom_mono_improved(XMVECTOR& rot, XMVECTOR& vel, float x, XMVECTOR r0, XMVECTOR r1, XMVECTOR r2, XMVECTOR r3);
	void quat_hermite(XMVECTOR& rot, XMVECTOR& vel, float x, XMVECTOR r0, XMVECTOR r1, XMVECTOR v0, XMVECTOR v1);
	void quat_catmull_rom(XMVECTOR& rot, XMVECTOR& vel, float x, XMVECTOR r0, XMVECTOR r1, XMVECTOR r2, XMVECTOR r3);

	void position_catmull_rom_smooth(XMVECTOR& pos, XMVECTOR& vel, float x, XMVECTOR p0, XMVECTOR p1, XMVECTOR p2, XMVECTOR p3);
	float cubic_mono_velocity_smooth(float d0, float d1);
	XMVECTOR cubic_mono_velocity_smooth(XMVECTOR d0, XMVECTOR d1);
	void position_hermite(XMVECTOR& pos, XMVECTOR& vel, float x, XMVECTOR p0, XMVECTOR p1, XMVECTOR v0, XMVECTOR v1);

	void scalar_hermite(float& value, float& derivative, float x, float v0, float v1, float d0, float d1);
	
	bool is_small_rotation(XMVECTOR q1, XMVECTOR q2);
	XMVECTOR quat_slerp_safe(XMVECTOR q1, XMVECTOR q2, float t);
	XMVECTOR quat_to_scaled_angle_axis_safe(XMVECTOR q);
	XMVECTOR quat_from_scaled_angle_axis(XMVECTOR v);




	
	
	



}
