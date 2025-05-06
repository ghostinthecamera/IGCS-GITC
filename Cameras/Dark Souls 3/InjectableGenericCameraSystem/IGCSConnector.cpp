#include "stdafx.h"
#include "MessageHandler.h"
#include "Globals.h"
#include "System.h"
#include "Utils.h"
#include "IGCSConnector.h"
#include "Camera.h"
#include "CameraManipulator.h"
#include "CameraToolsData.h"

namespace IGCS::IGCSConnector
{
    void setupIGCSConnectorConnection()
    {
        connectWithReshadeAddons();
    }

    void fillDataForAddOn()
    {
        // Check vector size first (cheaper than isCameraFound() potentially)
        const auto& buffers = _dataBlocksFromToolsToAddOns;
        if (buffers.empty())
        {
            return;
        }

        if (!GameSpecific::CameraManipulator::isCameraFound())
        {
            return;
        }

        // Compute shared values once before the loop
        GameSpecific::CameraManipulator::setMatrixRotationVectors();
        const auto& camera = Camera::instance();

        // Cache all the values we need only once
        const auto cameraEnabled = g_cameraEnabled;
        const auto coordinates = GameSpecific::CameraManipulator::getCurrentCameraCoords();
        const auto rightVector = camera.getRightVector();
        const auto upVector = camera.getUpVector();
        const auto forwardVector = camera.getForwardVector();
        const auto gameEulers = camera.getGameEulers();
	        const float pitch = gameEulers.x;
	        const float yaw = gameEulers.y;
	        const float roll = gameEulers.z;
        const auto lookQuaternion = camera.getGameQuaternion();
        const float fov = GameSpecific::CameraManipulator::fovinDegrees(GameSpecific::CameraManipulator::getCurrentFoV());
        const auto movementLocked = Globals::instance().cameraMovementLocked();

        // Populate each buffer directly, avoiding the interim struct completely
        for (const LPBYTE buffer : buffers)
        {
            const auto data = reinterpret_cast<CameraToolsData*>(buffer);

            // Directly assign to the destination buffer
            data->cameraEnabled = cameraEnabled;
            data->coordinates = coordinates;
            data->rotationMatrixRightVector = rightVector;
            data->rotationMatrixUpVector = upVector;
            data->rotationMatrixForwardVector = forwardVector;
            data->pitch = pitch;
            data->yaw = yaw;
            data->roll = roll;
            data->lookQuaternion = lookQuaternion;
            data->fov = fov;
            data->cameraMovementLocked = movementLocked;
        }
    }

    void connectWithReshadeAddons()
    {
        // first try to locate the addon modules. Then try to get the addresses of well known functions to call.
        // get function handles, then call the functions to obtain the pointers to the buffers we'll use for exchanging data.
        typedef bool(__stdcall* IgcsConnector_connectFromCameraTools)();
        typedef LPBYTE(__stdcall* IgcsConnector_getDataFromCameraToolsBuffer)();

        const HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
        if (nullptr == processHandle)
        {
            return;
        }

        HMODULE modules[1024];
        DWORD cbNeeded;
        char modulebaseName[512] = { 0 };
        if (EnumProcessModules(processHandle, modules, sizeof(modules), &cbNeeded))
        {
            for (int i = 0; i < cbNeeded / sizeof(HMODULE); i++)
            {
                // check if this module exports the connectFromCameraTools function.
                const HMODULE moduleHandle = modules[i];
                MODULEINFO info;
                if (!GetModuleInformation(processHandle, moduleHandle, &info, sizeof(MODULEINFO)))
                {
                    continue;
                }
                const auto connectFunc = reinterpret_cast<IgcsConnector_connectFromCameraTools>(GetProcAddress(static_cast<HMODULE>(info.lpBaseOfDll),
	                "connectFromCameraTools"));
                if (nullptr == connectFunc)
                {
                    // not an addon we're looking for
                    continue;
                }
                // might be a candidate, check if it also exports getDataFromCameraToolsBuffer
                const auto getFromToolsBufferFunc = reinterpret_cast<IgcsConnector_getDataFromCameraToolsBuffer>(GetProcAddress(
	                static_cast<HMODULE>(info.lpBaseOfDll), "getDataFromCameraToolsBuffer"));
                if (nullptr == getFromToolsBufferFunc)
                {
                    // not a candidate
                    continue;
                }
                // get module name for logging
                GetModuleBaseNameA(processHandle, moduleHandle, modulebaseName, sizeof(modulebaseName));

                // two functions found, call the connect func to initialize the buffer
                if (const bool result = connectFunc(); !result)
                {
                    MessageHandler::logLine("Function 'connectFromCameraTools' in ReShade 5 addon '%s' returned false. Ignored.", modulebaseName);
                    continue;
                }

                // all set
                MessageHandler::logLine("Connected to ReShade 5 addon: %s", modulebaseName);
                LPBYTE buffer = getFromToolsBufferFunc();
                if (nullptr != buffer)
                {
                    _dataBlocksFromToolsToAddOns.push_back(buffer);
                }
                MessageHandler::logDebug("Buffer for data from tools to addon: %p", static_cast<void*>(buffer));

                // Store module handle for later use
                _addonModules.push_back(moduleHandle);

                // if this is our own IGCSConnector addon, it should receive camera path info, if supported, so we have to connect it to the camera path manager
                if (std::string(modulebaseName) == "IgcsConnector.addon64")
                {
                    MessageHandler::logLine("Found IGCSConnector addon - setting up path management functions");
                	CameraPathManager::instance().connectToAddon(static_cast<HMODULE>(info.lpBaseOfDll));
                }
                 
            }
        }
        CloseHandle(processHandle);
    }

}
