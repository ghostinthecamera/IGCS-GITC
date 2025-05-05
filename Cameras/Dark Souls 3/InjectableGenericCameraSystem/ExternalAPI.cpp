#include "stdafx.h"
#include "Globals.h"
#include "MessageHandler.h"

extern "C" __declspec(dllexport) uint8_t IGCS_StartScreenshotSession(uint8_t type);
extern "C" __declspec(dllexport) void IGCS_MoveCameraPanorama(float stepAngle);
extern "C" __declspec(dllexport) void IGCS_MoveCameraMultishot(float stepLeftRight, float stepUpDown, float fovDegrees, bool fromStartPosition);
extern "C" __declspec(dllexport) void IGCS_EndScreenshotSession();


using namespace IGCS;

// <summary>
/// Starts a screenshot session of the specified type
/// </summary>
///	<param name="type">specifies the screenshot session type:</param>
///	0: Normal panorama
/// 1: Multishot (moving the camera over a line or grid or other trajectory)
/// <returns>0 if the session was successfully started or a value > 0 if not:
/// 1 if the camera wasn't enabled
/// 2 if a camera path is playing
/// 3 if there was already a session active
/// 4 if camera feature isn't available
/// 5 if another error occurred</returns>

static uint8_t IGCS_StartScreenshotSession(uint8_t type)
{
	MessageHandler::logDebug("Starting IGCS Connector session");

	// start the session
	return System::instance().startIGCSsession(type);
}


/// <summary>
/// Rotates the camera in the current session over the specified stepAngle.
/// </summary>
/// <param name="stepAngle">
/// stepAngle is the angle (in radians) to rotate over to the right. Negative means the camera will rotate to the left.
/// </param>
///	<remarks>stepAngle isn't divided by movementspeed/rotation speed yet, so it has to be done locally in the camerafeaturebase.</remarks>
static void IGCS_MoveCameraPanorama(float stepAngle)
{
	System::instance().stepCameraforIGCSSession(stepAngle);
}


/// <summary>
/// Moves the camera up/down/left/right based on the values specified and whether that's relative to the start location or to the current camera location.
/// </summary>
/// <param name="stepLeftRight">The amount to step left/right. Negative values will make the camera move to the left, positive values make the camera move to the right</param>
/// <param name="stepUpDown">The amount to step up/down. Negative values with make the camera move down, positive values make the camera move up</param>
/// <param name="fovDegrees">The fov in degrees to use for the step. If &lt= 0, the value is ignored</param>
/// <param name="fromStartPosition">If true the values specified will be relative to the start location of the session, otherwise to the current location of the camera</param>
static void IGCS_MoveCameraMultishot(float stepLeftRight, float stepUpDown, float fovDegrees, bool fromStartPosition)
{
	System::instance().stepCameraforIGCSSession(stepLeftRight, stepUpDown, fovDegrees, fromStartPosition);
}


/// <summary>
/// Ends the active screenshot session, restoring camera data if required.
/// </summary>
static void IGCS_EndScreenshotSession()
{
	MessageHandler::logDebug("Ending IGCS Connector Session");
	System::instance().endIGCSsession();
}
