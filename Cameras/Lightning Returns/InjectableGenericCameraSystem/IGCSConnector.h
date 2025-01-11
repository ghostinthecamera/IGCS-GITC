#pragma once

#include "CameraManipulator.h"
#include "GameCameraData.h"
#include "CameraToolsData.h"

namespace IGCS::IGCSConnector
{
	void setupIGCSConnectorConnection();
	void connectWithReshadeAddons();
	void fillDataForAddOn(Camera camera);
}