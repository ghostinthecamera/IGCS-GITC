#pragma once

#include "stdafx.h"

static uint8_t IGCS_StartScreenshotSession(uint8_t type);
static void IGCS_MoveCameraPanorama(float stepAngle);
static void IGCS_MoveCameraMultishot(float stepLeftRight, float stepUpDown, float fovDegrees, bool fromStartPosition);
static void IGCS_EndScreenshotSession();


