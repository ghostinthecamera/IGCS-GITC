#pragma once
#include "stdafx.h"
#include "Gamepad.h"
#include "Utils.h"
#include "GameConstants.h"
#include "CDataFile.h"
#include "Defaults.h"
#include <map>
#include "ActionData.h"

namespace IGCS
{
	struct Settings
	{
		// settings written to config file
		bool invertY;
		float fastMovementMultiplier;
		float slowMovementMultiplier;
		float movementUpMultiplier;
		float movementSpeed;
		float rotationSpeed;
		float fovChangeSpeed;
		int cameraControlDevice;		// 0==keyboard/mouse, 1 == gamepad, 2 == both, see Defaults.h
		bool allowCameraMovementWhenMenuIsUp;
		bool disableInGameDofWhenCameraIsEnabled;

		// settings not persisted to config file.
		// add settings to edit here.
		float resolutionScale;			// 0.5-4.0
		int todHour;					// 0-23
		int todMinute;					// 0-59
		float fogStrength;				// 0-200. 0 is no fog (ugly), 200 is thick fog all around you. Can go higher if one wants.
		float fogStartCurve;			// 0-1. 1 is default. 

		void loadFromFile(map<ActionType, ActionData*> keyBindingPerActionType)
		{
			CDataFile iniFile;
			if (!iniFile.Load(IGCS_SETTINGS_INI_FILENAME))
			{
				// doesn't exist
				return;
			}
			invertY = iniFile.GetBool("invertY", "CameraSettings");
			allowCameraMovementWhenMenuIsUp = iniFile.GetBool("allowCameraMovementWhenMenuIsUp", "CameraSettings");
			disableInGameDofWhenCameraIsEnabled = iniFile.GetBool("disableInGameDofWhenCameraIsEnabled", "CameraSettings");
			fastMovementMultiplier = Utils::clamp(iniFile.GetFloat("fastMovementMultiplier", "CameraSettings"), 0.0f, FASTER_MULTIPLIER);
			slowMovementMultiplier = Utils::clamp(iniFile.GetFloat("slowMovementMultiplier", "CameraSettings"), 0.0f, SLOWER_MULTIPLIER);
			movementUpMultiplier = Utils::clamp(iniFile.GetFloat("movementUpMultiplier", "CameraSettings"), 0.0f, DEFAULT_UP_MOVEMENT_MULTIPLIER);
			movementSpeed = Utils::clamp(iniFile.GetFloat("movementSpeed", "CameraSettings"), 0.0f, DEFAULT_MOVEMENT_SPEED);
			rotationSpeed = Utils::clamp(iniFile.GetFloat("rotationSpeed", "CameraSettings"), 0.0f, DEFAULT_ROTATION_SPEED);
			fovChangeSpeed = Utils::clamp(iniFile.GetFloat("fovChangeSpeed", "CameraSettings"), 0.0f, DEFAULT_FOV_SPEED);
			cameraControlDevice = Utils::clamp(iniFile.GetInt("cameraControlDevice", "CameraSettings"), 0, DEVICE_ID_ALL, DEVICE_ID_ALL);

			// load keybindings. They might not be there, or incomplete. 
			for (std::pair<ActionType, ActionData*> kvp : keyBindingPerActionType)
			{
				int valueFromIniFile = iniFile.GetInt(kvp.second->getName(), "KeyBindings");
				if (valueFromIniFile == INT_MIN)
				{
					// not found
					continue;
				}
				kvp.second->setValuesFromIniFileValue(valueFromIniFile);
			}
		}


		void saveToFile(map<ActionType, ActionData*> keyBindingPerActionType)
		{
			CDataFile iniFile;
			iniFile.SetBool("invertY", invertY, "", "CameraSettings");
			iniFile.SetBool("allowCameraMovementWhenMenuIsUp", allowCameraMovementWhenMenuIsUp, "", "CameraSettings");
			iniFile.SetBool("disableInGameDofWhenCameraIsEnabled", disableInGameDofWhenCameraIsEnabled, "", "CameraSettings");
			iniFile.SetFloat("fastMovementMultiplier", fastMovementMultiplier, "", "CameraSettings");
			iniFile.SetFloat("slowMovementMultiplier", slowMovementMultiplier, "", "CameraSettings");
			iniFile.SetFloat("movementUpMultiplier", movementUpMultiplier, "", "CameraSettings");
			iniFile.SetFloat("movementSpeed", movementSpeed, "", "CameraSettings");
			iniFile.SetFloat("rotationSpeed", rotationSpeed, "", "CameraSettings");
			iniFile.SetFloat("fovChangeSpeed", fovChangeSpeed, "", "CameraSettings");
			iniFile.SetInt("cameraControlDevice", cameraControlDevice, "", "CameraSettings");

			// save keybindings
			if (!keyBindingPerActionType.empty())
			{
				for (std::pair<ActionType, ActionData*> kvp : keyBindingPerActionType)
				{
					// we're going to write 4 bytes, the keycode is 1 byte, and for each bool we'll shift in a 1 or 0, depending on whether it's true (1) or false (0).
					int value = kvp.second->getIniFileValue();
					iniFile.SetInt(kvp.second->getName(), value, "", "KeyBindings");
				}
				iniFile.SetSectionComment("KeyBindings", "Values are 4-byte ints: A|B|C|D, where A is byte for VT keycode, B, C and D are bools if resp. Alt, Ctrl or Shift is required");
			}

			iniFile.SetFileName(IGCS_SETTINGS_INI_FILENAME);
			iniFile.Save();
		}


		void init(bool persistedOnly)
		{
			invertY = CONTROLLER_Y_INVERT;
			fastMovementMultiplier = FASTER_MULTIPLIER;
			slowMovementMultiplier = SLOWER_MULTIPLIER;
			movementUpMultiplier = DEFAULT_UP_MOVEMENT_MULTIPLIER;
			movementSpeed = DEFAULT_MOVEMENT_SPEED;
			rotationSpeed = DEFAULT_ROTATION_SPEED;
			fovChangeSpeed = DEFAULT_FOV_SPEED;
			cameraControlDevice = DEVICE_ID_ALL;
			allowCameraMovementWhenMenuIsUp = false;
			disableInGameDofWhenCameraIsEnabled = false;

			if (!persistedOnly)
			{
				resolutionScale = 1.0f;
				todHour = 12;
				todMinute = 0;
				fogStrength = 1.0f;
				fogStartCurve = 1.0f;
			}
		}
	};
}