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
	void timescaleInterceptor();
	void fovReadInterceptor();
	void lodSettingInterceptor();
    void gameplayTimescaleInterceptor();
    void playerPositionInterceptor();
}

// external addresses used in asm.
extern "C" {
	uint8_t* _cameraStructInterceptionContinue = nullptr;
	uint8_t* _timescaleInterceptionContinue = nullptr;
	uint8_t* _fovReadInterceptionContinue = nullptr;
	uint8_t* _timescaleAbsolute = nullptr;
	uint8_t* _lodSettingInterceptionContinue = nullptr;
	uint8_t* _gameplayTimescaleInterceptionContinue = nullptr;
	uint8_t* _playerPositionInterceptionContinue = nullptr;
}

namespace IGCS::GameSpecific
{

    // Helper function for initialization hooks that need error checking
    bool tryInitHook(map<string, AOBBlock>& aobBlocks, const string& key,
        const function<void(AOBBlock&)>& operation)
    {
        try {
            if (aobBlocks.find(key) != aobBlocks.end()) {
                operation(aobBlocks[key]);
                return true;
            }
            MessageHandler::logError("Block '%s' not found in AOBBlocks map", key.c_str());
            return false;
        }
        catch (const exception& e) {
            MessageHandler::logError("Error in operation for block '%s': %s", key.c_str(), e.what());
            return false;
        }
    }

    // AOB pattern constants - definitions moved here from function body for better organization
    const struct {
        string active_camera_address = "41 0F 11 86 ?? ?? ?? ?? 41 0F 11 8E ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 84 24";
		string timescale_injection = "F3 0F 10 4B ?? 48 8B 4B ??";
		string fov_injection_key1 = "8B 02 89 01 8B 42 ?? 89 41 ?? 0F 28 42 ?? 66 0F 7F 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? | 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 0F B6 42 ?? 88 41 ?? C3";
		string fov_write_key2 = "8B 02 89 01 8B 42 ?? 89 41 ?? 0F 28 42 ?? 66 0F 7F 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? | 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 0F B6 42 ?? 88 41 ?? C3";
		string lod_key = "41 88 86 ?? ?? ?? ?? 41 89 96 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ??";
		string timescale_absolute_intercept_key = "F3 0F 10 4B ?? 48 8B 4B ?? E8 | ?? ?? ?? ??";
        string hud_toggle = "80 B9 58 02 00 00 00 74 0C 80 B9 ?? ?? ?? ?? 00 74 03 B0 01 C3 32 C0 C3 CC CC CC CC";
        string gameplay_timescale = "48 B8 00 00 00 00 00 00 F0 3F F2 0F 11 8B ?? ?? ?? ??";
        string gameplay_timescale_interception = "48 89 83 ?? ?? ?? ?? 48 8B 43 ?? F2 0F 11 8B ?? ?? ?? ??";
        string hud_toggle_checker_nop = "74 05 E8 ?? ?? ?? ?? 48 8D 8B ?? ?? ?? ?? 0F 28 CE"; // 2 nops
        string player_position = "48 8B 41 08 0F 10 80 E0 02 00 00 66";
    } aob;

    bool InterceptorHelper::initializeAOBBlocks(const LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock>& aobBlocks)
    {
        // Initialize all AOB blocks with their patterns
        aobBlocks[ACTIVE_CAMERA_ADDRESS_INTERCEPT] = AOBBlock(ACTIVE_CAMERA_ADDRESS_INTERCEPT, aob.active_camera_address, 1);
    	aobBlocks[TIMESCALE_INJECTION] = AOBBlock(TIMESCALE_INJECTION, aob.timescale_injection, 1);
        aobBlocks[FOV_WRITE_INJECTION1] = AOBBlock(FOV_WRITE_INJECTION1, aob.fov_injection_key1, 1);
		aobBlocks[FOV_WRITE2] = AOBBlock(FOV_WRITE2, aob.fov_write_key2, 1);
		aobBlocks[LOD_KEY] = AOBBlock(LOD_KEY, aob.lod_key, 1);
		aobBlocks[TIMESCALE_ABS_INTERCEPT_KEY] = AOBBlock(TIMESCALE_ABS_INTERCEPT_KEY, aob.timescale_absolute_intercept_key, 1);
		aobBlocks[HUD_TOGGLE_INJECTION] = AOBBlock(HUD_TOGGLE_INJECTION, aob.hud_toggle, 1);
		aobBlocks[GAMEPLAY_TIMESCALE] = AOBBlock(GAMEPLAY_TIMESCALE, aob.gameplay_timescale, 1);
		aobBlocks[GAMEPLAY_TIMESCALE_INTERCEPTION] = AOBBlock(GAMEPLAY_TIMESCALE_INTERCEPTION, aob.gameplay_timescale_interception, 1);
		aobBlocks[HUD_TOGGLE_CHECKER_NOP] = AOBBlock(HUD_TOGGLE_CHECKER_NOP, aob.hud_toggle_checker_nop, 1);
		aobBlocks[PLAYER_POSITION] = AOBBlock(PLAYER_POSITION, aob.player_position, 1);

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

    bool InterceptorHelper::setCameraStructInterceptorHook(map<string, AOBBlock>& aobBlocks)
    {
        return tryInitHook(aobBlocks, ACTIVE_CAMERA_ADDRESS_INTERCEPT, [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0x10, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
            });
    }

	bool InterceptorHelper::setPostCameraStructHooks(map<string, AOBBlock>& aobBlocks)
	{
		bool result = true;
		result &= tryInitHook(aobBlocks, FOV_WRITE_INJECTION1,
			[](AOBBlock& block) {
				GameImageHooker::setHook(&block, 0x0F, &_fovReadInterceptionContinue, &fovReadInterceptor);
			});
		result &= tryInitHook(aobBlocks, TIMESCALE_INJECTION,
			[](AOBBlock& block) {
				GameImageHooker::setHook(&block, 0x0E, &_timescaleInterceptionContinue, &timescaleInterceptor);
			});
		result &= tryInitHook(aobBlocks, LOD_KEY,
			[](AOBBlock& block) {
				GameImageHooker::setHook(&block, 0x0E, &_lodSettingInterceptionContinue, &lodSettingInterceptor);
			});
		result &= tryInitHook(aobBlocks, GAMEPLAY_TIMESCALE_INTERCEPTION,
			[](AOBBlock& block) {
				GameImageHooker::setHook(&block, 0x13, &_gameplayTimescaleInterceptionContinue, &gameplayTimescaleInterceptor);
			});
		result &= tryInitHook(aobBlocks, PLAYER_POSITION,
			[](AOBBlock& block) {
				GameImageHooker::setHook(&block, 0x0F, &_playerPositionInterceptionContinue, &playerPositionInterceptor);
			});

		return result;
	}

    bool InterceptorHelper::toggleHud(map<string, AOBBlock>& aobBlocks, const bool hudVisible)
    {
    	Utils::saveBytesWrite(aobBlocks[HUD_TOGGLE_INJECTION], sizeof(hudByte), hudByte, !hudVisible);
        Utils::toggleNOPState(aobBlocks[HUD_TOGGLE_CHECKER_NOP], 2, !hudVisible);
    	return true;
    }

    bool InterceptorHelper::togglePause(map<string, AOBBlock>& aobBlocks, bool enabled)
    {
        return false;
    }

    void InterceptorHelper::getAbsoluteAddresses(map<string, AOBBlock>& aobBlocks)
    {
        _timescaleAbsolute = Utils::calculateAbsoluteAddress(&aobBlocks[TIMESCALE_ABS_INTERCEPT_KEY], 4);

		MessageHandler::logDebug("Timescale absolute address: %p", _timescaleAbsolute);
    }

    bool InterceptorHelper::cameraSetup(map<string, AOBBlock>& aobBlocks, bool enabled, GameAddressData& addressData)
    {
        try {
            return true;
        }
        catch (const exception& e) {
            MessageHandler::logError("Failed to set up camera: %s", e.what());
            return false;
        }
    }

    bool InterceptorHelper::toolsInit(map<string, AOBBlock>& aobBlocks)
    {
	    try
	    {
	    	MemoryPatcher::addPatches({
			    {"FocusLoss1", 0x774204, patchbyte},
			    {"FocusLoss2", 0x7741EC, patchbyte},
			    {"FocusLoss3", 0x7741F4, patchbyte},
				{"Vignette", 0x9074E3, vignettePatch},
                {"ChromaticAbberation", 0x907501, caPatch },
                {"ShadowRes", 0x16BD8D4, Utils::intToBytes(shadowResPatch)} //Grid_dx12.exe+16BD8D4
            });

			// Enable all patches apart from shadow res at init
			std:vector<std::string> initPatches = { "FocusLoss1", "FocusLoss2", "FocusLoss3", "Vignette", "ChromaticAbberation"};
            MemoryPatcher::togglePatchGroup(initPatches, true);
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