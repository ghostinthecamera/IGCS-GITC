#include "stdafx.h"
#include "MessageHandler.h"
#include "CatmullRom.h"
#include <DirectXMath.h>

namespace IGCS 
{
    XMVECTOR CatmullRomPositionInterpolation(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
    {  
       int i0, i1, i2, i3; //create variables to be populated by our getNodeSequence function  

       float t = globalT - path.getsegmentIndex(globalT); //extract the local t value for this segment from the global t value  
       path.getnodeSequence(i0, i1, i2, i3, globalT); //populate the variables with the correct node indices  

       XMVECTOR p0 = _nodes[i0].position; //get the position of the first node  
       XMVECTOR p1 = _nodes[i1].position; //get the position of the second node  
       XMVECTOR p2 = _nodes[i2].position; //get the position of the third node  
       XMVECTOR p3 = _nodes[i3].position; //get the position of the fourth node  

       // Use DirectXMath’s CatmullRom for vector positions  
       return XMVectorCatmullRom(p0, p1, p2, p3, t);  
    }

	XMVECTOR CatmullRomRotationInterpolation(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		int i0, i1, i2, i3; //create variables to be populated by our getNodeSequence function

		float t = globalT - path.getsegmentIndex(globalT); //extract the local t value for this segment from the global t value
		path.getnodeSequence(i0, i1, i2, i3, globalT); //populate the variables with the correct node indices

		XMVECTOR q0 = _nodes[i0].rotation; //get the rotation of the first node
		XMVECTOR q1 = _nodes[i1].rotation; //get the rotation of the second node
		XMVECTOR q2 = _nodes[i2].rotation; //get the rotation of the third node
		XMVECTOR q3 = _nodes[i3].rotation;  //get the rotation of the fourth node

		q0 = XMQuaternionNormalize(q0); //normalize the quaternions
		q1 = XMQuaternionNormalize(q1);
		q2 = XMQuaternionNormalize(q2);
		q3 = XMQuaternionNormalize(q3);

		// avoid quaternion flips
		if (XMVectorGetX(XMQuaternionDot(q0, q1)) < 0.0f) q1 = XMVectorNegate(q1);
		if (XMVectorGetX(XMQuaternionDot(q1, q2)) < 0.0f) q2 = XMVectorNegate(q2);
		if (XMVectorGetX(XMQuaternionDot(q2, q3)) < 0.0f) q3 = XMVectorNegate(q3);

		XMVECTOR catmullQ = XMVectorCatmullRom(q0, q1, q2, q3, t); //use DirectXMath’s CatmullRom for quaternions
		return XMQuaternionNormalize(catmullQ); // Make sure it's still a valid quaternion
	}

	float CatmullRomFoV(float globalT, const std::vector<CameraNode>& _nodes, const CameraPath& path)
	{
		int i0, i1, i2, i3; //create variables to be populated by our getNodeSequence function

		float t = globalT - path.getsegmentIndex(globalT); //extract the local t value for this segment from the global t value
		path.getnodeSequence(i0, i1, i2, i3, globalT); //populate the variables with the correct node indices

		float p0 = _nodes[i0].fov; //get the fov of the first node
		float p1 = _nodes[i1].fov; //get the fov of the second node
		float p2 = _nodes[i2].fov; //get the fov of the third node
		float p3 = _nodes[i3].fov; //get the fov of the fourth node

		// Using standard Catmull–Rom formula
		float t2 = t * t;
		float t3 = t2 * t;

		// Catmull-Rom interpolation
		float f =
			0.5f * ((2.0f * p1)
				+ (-p0 + p2) * t
				+ (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
				+ (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);

		return f;
	}

	void CatmullRomGenerateArcTable(std::vector<float>& _arcLengthTable, std::vector<float>& _paramTable, const std::vector<CameraNode>& _nodes, const int _samplesize, const CameraPath& path)
	{
		if (_nodes.size() < 2)
		{
			MessageHandler::logError("Not enough nodes, returning from arc table generation");
			_arcLengthTable.clear();
			_paramTable.clear();
			return;
		}

		_arcLengthTable.clear();
		_paramTable.clear();

		// We'll subdivide each segment into N samples for measuring distance.
		int SAMPLES_PER_SEGMENT = _samplesize;

		// Start from distance = 0 at the first node
		float accumulatedDist = 0.0f;
		_arcLengthTable.push_back(accumulatedDist);
		_paramTable.push_back(0.0f); // globalT = 0.0 means segment=0, t=0

		// For each segment [i..i+1], step in SAMPLES_PER_SEGMENT increments
		// i in [0.._nodes.size()-2]
		XMVECTOR prevPos = CatmullRomPositionInterpolation(0.0f, _nodes, path); // The position at the start

		int totalSegments = (int)_nodes.size() - 1;
		for (int i = 0; i < totalSegments; ++i)
		{
			for (int step = 1; step <= SAMPLES_PER_SEGMENT; ++step)
			{
				float localT = (float)step / (float)SAMPLES_PER_SEGMENT; // [0..1]
				float globalT = (float)i + localT; // i + fraction

				XMVECTOR curPos = CatmullRomPositionInterpolation(globalT, _nodes, path);

				// distance from previous sample
				float dist = XMVectorGetX(XMVector3Length(curPos - prevPos));
				accumulatedDist += dist;

				// store in the tables
				_arcLengthTable.push_back(accumulatedDist);
				_paramTable.push_back(globalT);

				prevPos = curPos;
			}
		}

		// Optionally, log or store final total distance:
		float totalLength = _arcLengthTable.back();
		/*MessageHandler::logDebug("Arc length table created. Total path length: %f", totalLength);
		MessageHandler::logDebug("Size of Arc length table: %zu b", ((_arcLengthTable.capacity() * sizeof(float)) + (_paramTable.capacity() * sizeof(float))));*/
	}
}
