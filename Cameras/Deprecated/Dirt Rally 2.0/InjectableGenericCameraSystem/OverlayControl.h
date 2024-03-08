#pragma once

#include "stdafx.h"

namespace IGCS::OverlayControl
{
	void init();
	void toggleOverlay();
	void renderOverlay();
	bool isMainMenuVisible();
	void addNotification(std::string notificationText);
}

