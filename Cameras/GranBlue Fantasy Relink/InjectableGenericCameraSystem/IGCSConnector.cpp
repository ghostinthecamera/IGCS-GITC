#include "stdafx.h"
#include "MessageHandler.h"
#include "Globals.h"
#include "System.h"
#include "Utils.h"
#include "IGCSConnector.h"
#include "Camera.h"

using namespace std;
using namespace IGCS::GameSpecific::CameraManipulator;

namespace IGCS::IGCSConnector
{
    vector <LPBYTE> _dataBlocksFromToolsToAddOns;

    void setupIGCSConnectorConnection()
    {
        connectWithReshadeAddons();
    }

    void fillDataForAddOn(Camera camera)
    {
        if (!isCameraFound())
        {
            return;
        }

        const std::vector<LPBYTE> buffers = _dataBlocksFromToolsToAddOns;
        if (buffers.size() <= 0)
        {
            // no addon is listening
            return;
        }

        //store all this in our interim cameratoolsdata struct, we will then just copy this to the pointers in our buffer
        GameSpecific::CameraManipulator::setMatrixRotationVectors(camera);

        CameraToolsData interim;
        interim.cameraEnabled = g_cameraEnabled;
        interim.coordinates = (Vec3)GameSpecific::CameraManipulator::getCurrentCameraCoords();
        interim.rotationMatrixRightVector = camera.getRightVector();
        interim.rotationMatrixUpVector = camera.getUpVector();
        interim.rotationMatrixForwardVector = camera.getForwardVector();
        //if (g_cameraEnabled)
        //{
        //    interim.pitch = camera.getPitch();
        //    interim.yaw = camera.getYaw();
        //    interim.roll = camera.getRoll();
        //    interim.lookQuaternion = camera.getToolsQuaternion();
        //}
        //else
        //{
            interim.pitch = camera.getGameEulers().x();
            interim.yaw = camera.getGameEulers().y();
            interim.roll = camera.getGameEulers().z();
            interim.lookQuaternion = camera.getGameQuaternion();
        //}
        interim.fov = GameSpecific::CameraManipulator::fovinDegrees(getCurrentFoV());
        interim.cameraMovementLocked = Globals::instance().cameraMovementLocked();

        //iterate through the buffers and apply our values
        for (const LPBYTE buffer : buffers)
        {
            CameraToolsData* data = (CameraToolsData*)buffer;
            *data = interim;
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
                const IgcsConnector_connectFromCameraTools connectFunc = (IgcsConnector_connectFromCameraTools)GetProcAddress((HMODULE)info.lpBaseOfDll, "connectFromCameraTools");
                if (nullptr == connectFunc)
                {
                    // not an addon we're looking for
                    continue;
                }
                // might be a candidate, check if it also exports getDataFromCameraToolsBuffer
                const IgcsConnector_getDataFromCameraToolsBuffer getFromToolsBufferFunc = (IgcsConnector_getDataFromCameraToolsBuffer)GetProcAddress((HMODULE)info.lpBaseOfDll, "getDataFromCameraToolsBuffer");
                if (nullptr == getFromToolsBufferFunc)
                {
                    // not a candidate
                    continue;
                }
                // get module name for logging
                GetModuleBaseNameA(processHandle, moduleHandle, modulebaseName, sizeof(modulebaseName));

                // two functions found, call the connect func to initialize the buffer
                const bool result = connectFunc();
                if (!result)
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
                MessageHandler::logDebug("Buffer for data from tools to addon: %p", (void*)buffer);

                // if this is our own Igcsconnector addon, it should receive camera path info, if supported, so we have to connect it to the camera path manager
                //if (std::string(modulebaseName) == "IgcsConnector.addon64")
                //{
                //    _pathController.connectToAddon((HMODULE)info.lpBaseOfDll);
                //}
            }
        }
    }
}