////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020-2025, Frans Bouma
// All rights reserved.
// https://github.com/FransBouma/InjectableGenericCameraSystem
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "InterceptorHelper.h"
#include "GameConstants.h"
#include "GameImageHooker.h"
#include "MessageHandler.h"
#include "CameraManipulator.h"
#include "Globals.h"
#include "MemoryPatcher.h"

using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
	void cameraStructInterceptor();
    void cameraPositionInterceptor();
    void cameraRotationInterceptor();
	void vignetteInterceptor();
    void timescaleInterceptor();
	void playerStructInterceptor();
    void playerLightCheckInterceptor();

}

// external addresses used in asm.
extern "C" {
	uint8_t* _cameraStructInterceptionContinue = nullptr;
	uint8_t* _cameraPositionInterceptionContinue = nullptr;
	uint8_t* _cameraRotationInterceptionContinue = nullptr;
	uint8_t* _vignetteInterceptionContinue = nullptr;
	uint8_t* _timescaleInterceptionContinue = nullptr;
	uint8_t* _playerStructInterceptionContinue = nullptr;
    uint8_t* _playerLightCheckInterceptionContinue = nullptr;
    uint8_t* _playerLightCheckJumpTarget = nullptr;
}

namespace IGCS::GameSpecific
{
    // AOB pattern constants - definitions moved here from function body for better organization
    const struct {
        string active_camera_address = "F3 0F 11 4A ?? E9 ?? ?? ?? ?? | 48 8B 48 ?? 48 8B 81 ?? ?? ?? ?? 48 85 C0 74 03 8B 40 ?? 85 C0 0F 84 ?? ?? ?? ??";
		string camera_write_byte = "48 83 78 18 00 0F 85 27 06 00 00 F3 0F 10 87 F0 04 00 00 F3 0F 10 8F F4 04 00 00 F3 0F 10 97 F8 04 00 00"; // write 48 83 78 18 01
		string fov_write = "F3 0F 11 40 38 48 83 79 18 00 0F 85 3E 05 00 00 48 85 C0 0F 84 DD 04 00 00"; //5 nops
		string camera_position_write = "89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 80 B9 ?? ?? ?? ?? 00 C6 81 ?? ?? ?? ?? 01 75 32";
		string camera_rotation_write = "8B 02 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 0C 89 41 4C 80 B9 ?? ?? ?? ?? 00 C6 81 ?? ?? ?? ?? 01 75 33 48 8B CF 48 85 FF 74 2B";
        string disable_flaslight = "75 48 48 85 C0 74 43 48 63 8F ?? ?? ?? ?? 85 C9 78 18 48 8B 80 ?? ?? ?? ??";
		string hud_toggle = "74 5B 48 8B 41 ?? 48 89 74 24 ?? 4C 89 74 24 ?? 4C 8B 30 8B 70 ??"; //change 74 5B (je) to EB 5B (jmp)
        string vignette_address = "48 8B 41 30 8B 80 0C 02 00 00 C3 CC CC CC";
        string timescale_address = "F3 0F 59 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? 0F 5A C0 F2 41 0F 5E C0";
        string player_address = "F3 0F 10 40 ?? 48 8D 54 24 ?? F3 0F 10 48 ?? 48 8B C8 F3 0F 10 50 ??";
        string player_position_zero = "75 27 F3 0F 11 47 ?? F3 0F 11 4F ?? F3 0F 11 57 ?? 80 BF ?? ?? ?? ?? 00 C6 87 ?? ?? ?? ?? 01"; //change 75 27 to EB 27
        string light_injection = "48 8B 47 ?? A8 01 75 48 48 85 C0 74 43 48 63 8F ?? ?? ?? ??";
    } aob;

    bool InterceptorHelper::initializeAOBBlocks(const LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock>& aobBlocks)
    {
        // Initialize all AOB blocks with their patterns
        aobBlocks[ACTIVE_CAMERA_ADDRESS_INTERCEPT] = AOBBlock(ACTIVE_CAMERA_ADDRESS_INTERCEPT, aob.active_camera_address, 1);
        aobBlocks[FOV_WRITE] = AOBBlock(FOV_WRITE, aob.fov_write, 1);
        aobBlocks[CAMERA_POSITION_WRITE] = AOBBlock(CAMERA_POSITION_WRITE, aob.camera_position_write, 1);
        aobBlocks[CAMERA_ROTATION_WRITE] = AOBBlock(CAMERA_ROTATION_WRITE, aob.camera_rotation_write, 1);
        aobBlocks[DISABLE_FLASHLIGHT] = AOBBlock(DISABLE_FLASHLIGHT, aob.disable_flaslight, 1);
        aobBlocks[HUD_TOGGLE] = AOBBlock(HUD_TOGGLE, aob.hud_toggle, 1);
        aobBlocks[VIGNETTE_TOGGLE] = AOBBlock(VIGNETTE_TOGGLE, aob.vignette_address, 1);
        aobBlocks[TIMESCALE_ADDRESS] = AOBBlock(TIMESCALE_ADDRESS, aob.timescale_address, 1);
        aobBlocks[PLAYER_ADDRESS] = AOBBlock(PLAYER_ADDRESS, aob.player_address, 1);
        aobBlocks[PLAYER_POSITION_ZERO] = AOBBlock(PLAYER_POSITION_ZERO, aob.player_position_zero, 1);
        aobBlocks[DISABLE_PLAYER_LIGHT_CHECK] = AOBBlock(DISABLE_PLAYER_LIGHT_CHECK, aob.light_injection, 1);

    	// Scan for all patterns
        bool result = true;
        for (auto& [key, block] : aobBlocks)
        {
            const bool scanResult = block.scan(hostImageAddress, hostImageSize);
            if (!scanResult) {
                MessageHandler::logError("Failed to find pattern for block '%s'", key.c_str());
            }
            result &= scanResult; 
        }

        if (result) {
            MessageHandler::logLine("All interception offsets found successfully.");
        }
        else {
            MessageHandler::logError("One or more interception offsets weren't found: tools aren't compatible with this game's version.");
        }
        return result;
    }

    bool InterceptorHelper::setCameraStructInterceptorHook()
    {
        return tryInitHook(ACTIVE_CAMERA_ADDRESS_INTERCEPT, [](AOBBlock& block) {
	        GameImageHooker::setHook(&block, 0x0E, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
        });
    }

	bool InterceptorHelper::setPostCameraStructHooks()
	{
		bool result = true;
		result &= tryInitHook(CAMERA_POSITION_WRITE, [](AOBBlock& block) {
			GameImageHooker::setHook(&block, 0x0F, &_cameraPositionInterceptionContinue, &cameraPositionInterceptor);
		});
		result &= tryInitHook(CAMERA_ROTATION_WRITE, [](AOBBlock& block) {
			GameImageHooker::setHook(&block, 0x17, &_cameraRotationInterceptionContinue, &cameraRotationInterceptor);
		});
        result &= tryInitHook(VIGNETTE_TOGGLE, [](AOBBlock& block) {
            GameImageHooker::setHook(&block, 0x0A, &_vignetteInterceptionContinue, &vignetteInterceptor); // pass continue var but we dont need it
        });
        result &= tryInitHook(TIMESCALE_ADDRESS, [](AOBBlock& block) {
            GameImageHooker::setHook(&block, 0x10, &_timescaleInterceptionContinue, &timescaleInterceptor); // pass continue var but we dont need it
        });
        result &= tryInitHook(PLAYER_ADDRESS, [](AOBBlock& block) {
            GameImageHooker::setHook(&block, 0x0F, &_playerStructInterceptionContinue, &playerStructInterceptor); // pass continue var but we dont need it
        });
        result &= tryInitHook(DISABLE_PLAYER_LIGHT_CHECK, [](AOBBlock& block) {
            // The original jne is `75 48`. The target address is:
            // instruction_address + instruction_size + relative_offset
            // re7.exe+241610E + 2 + 0x48 = re7.exe+2416158
            // Our block starts at 2416108. The jne starts at an offset of 6 from there.
            LPBYTE jneInstructionAddress = block.locationInImage() + 6;
            _playerLightCheckJumpTarget = jneInstructionAddress + 2 + 0x48; // Calculate absolute jump address

            // We need to overwrite 14 bytes to fit our hook.
            GameImageHooker::setHook(&block, 0xE, &_playerLightCheckInterceptionContinue, &playerLightCheckInterceptor);
        });

		return result;
	}

    bool InterceptorHelper::toggleHud(const bool hudVisible)
    {
    	Utils::saveBytesWrite(HUD_TOGGLE, sizeof(hudPatch), hudPatch, !hudVisible);
    	return true;
    }

    bool InterceptorHelper::togglePause(map<string, AOBBlock>& aobBlocks, bool enabled)
    {
        return false;
    }

    void InterceptorHelper::getAbsoluteAddresses(map<string, AOBBlock>& aobBlocks)
    {
		//_timescaleAbsolute = Utils::calculateAbsoluteAddress(&aobBlocks[TIMESCALE_ABS_INTERCEPT_KEY], 4);
		//MessageHandler::logDebug("Timescale absolute address: %p", _timescaleAbsolute);
    }

    bool InterceptorHelper::cameraSetup(bool enabled, GameAddressData& addressData)
    {
        try {
            Utils::toggleNOPState(FOV_WRITE, 5, enabled);
            Utils::saveBytesWrite(PLAYER_POSITION_ZERO, sizeof(playerPositionPatch), playerPositionPatch, enabled);

        	return true;
        }
        catch (const exception& e) {
            MessageHandler::logError("Failed to set up camera: %s", e.what());
            return false;
        }
    }

    bool InterceptorHelper::toolsInit()
    {
	    try
	    {
	    	//MemoryPatcher::addPatches({
			   // {"FocusLoss1", 0x774204, patchbyte},
			//});

			//// Enable all patches apart from shadow res at init
			//std:vector<std::string> initPatches = { "FocusLoss1", "FocusLoss2", "FocusLoss3", "Vignette", "ChromaticAbberation"};
			//MemoryPatcher::togglePatchGroup(initPatches, true);
	    }
	    catch (const exception& e)
	    {
			MessageHandler::logError("Failed to initialise pre-system ready instructions: %s", e.what());
			return false;
	    }
        // If we reach here, all initializations were successful
		MessageHandler::logLine("Pre-system patches completed successfully.");

    	return true;
    }

    void InterceptorHelper::handleSettings(map<string, AOBBlock>& aobBlocks, GameAddressData& addressData)
    {
        // This can be extended to handle game-specific settings
        // Such as vignette, bloom, etc. based on user preferences

        /*
        // Example implementation for vignette/bloom toggles:
        bool vignetteEnabled = Globals::instance().settings().vignetteToggle;
        bool bloomEnabled = Globals::instance().settings().bloomToggle;

        try {
            if (aobBlocks[VIGNETTE_KEY].nopState == !vignetteEnabled) {
                Utils::toggleNOPState(&aobBlocks[VIGNETTE_KEY], 5, vignetteEnabled);
            }

            if (aobBlocks[BLOOM_KEY].nopState == !bloomEnabled) {
                Utils::toggleNOPState(&aobBlocks[BLOOM_KEY], 5, bloomEnabled);
            }
        }
        catch (const exception& e) {
            MessageHandler::logError("Failed to handle settings: %s", e.what());
        }
        */
    }
}