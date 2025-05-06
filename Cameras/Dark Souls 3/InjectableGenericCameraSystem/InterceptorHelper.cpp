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

#include <functional>

#include "GameConstants.h"
#include "GameImageHooker.h"
#include "MessageHandler.h"
#include "Utils.h"


using namespace std;

//--------------------------------------------------------------------------------------------------------------------------------
// external asm functions
extern "C" {
    void cameraStructInterceptor();
    void dofStruct();
    void timescaleinterceptor();
    void playerpointerinterceptor();
    void entitytimeinterceptor();
    void entityopacityinterceptor();
}

// external addresses used in asm.
extern "C" {
    LPBYTE _cameraStructInterceptionContinue = nullptr;
    LPBYTE _dofInterceptionContinue = nullptr;
    LPBYTE _timestructinterceptionContinue = nullptr;
    LPBYTE _playerpointerContinue = nullptr;
    LPBYTE _entitytimeContinue = nullptr;
    LPBYTE _entityopacityContinue = nullptr;
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
        string camera_address = "0F 28 00 66 0F 7F 43 ?? 0F 28 48 ?? 66 0F 7F 4B ?? 0F 28 40 ?? 66 0F 7F 43 ?? 0F 28 48 ?? 66 0F 7F 4B ?? 48 8B 4F ?? 48 8B 57 ??";
        string fov = "89 4B ?? 8B 48 ?? 89 4B ?? 8B 48 ?? 89 4B ?? 8B 48 ?? 89 4B ?? 48 8B C8 E8 ?? ?? ?? ??";
        string dof = "8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 8B 42 ?? 89 41 ?? 48 8B C1 C3";
        string timestop = "F3 0F 11 5D 87 F3 0F 11 5D ?? 48 89 54 24 ?? 4C 89 6C 24 ?? 48 8D 45 ?? 48 8D 4D ?? 44 0F 2F EB";
    	string timestop_nop = "89 83 ?? ?? ?? ?? 80 BB ?? ?? ?? ?? 00 74 09 45 3B FE 41 0F 93 C0 EB 07";
        string playerpointer = "48 8B 43 08 48 8B 88 90 1F 00 00 48 8B 49 28 E8 ?? ?? ?? ?? 84 C0 74 0B 48 8B D6 48 8B CB E8 ?? ?? ?? ?? 48 8B 5C 24 30 48 8B 74 24 38 48 83 C4 20 5F";
        string entity = "F3 0F 10 83 ?? ?? ?? ?? F3 0F 59 83 ?? ?? ?? ?? F3 0F 59 D0 80 BB ?? ?? ?? ?? 00 74 03 0F 28 D6 80 BB ?? ?? ?? ?? 00";
        string hudtoggle = "4C 63 47 ?? 4D 03 C0 48 8B 47 ?? 49 8B D6 48 8B CF | 42 FF 14 C0 48 63 47 ?? 89 47 ?? 80 7F 68 00 74 2F 80 7F 69 00 74 29 83 F8 0E 77 18";
        string pause_byte = "01 00 00 00 F3 0F 10 40 ?? 0F 2F C6 0F 43 CA 88 0D ?? ?? ?? ??";
        string vignette = "F3 0F 10 41 ?? F3 0F 11 05 ?? ?? ?? ?? F3 0F 10 49 ?? F3 0F 11 0D ?? ?? ?? ?? 8B 81 ?? ?? ?? ?? 89 05 ?? ?? ?? ??";
        string bloom = "F3 0F 10 49 ?? F3 0F 11 0D ?? ?? ?? ?? 8B 41 ?? 89 05 ?? ?? ?? ?? 8B 41 ?? 89 05 ?? ?? ?? ?? F3 0F 10 41 ?? F3 0F 11 05 ?? ?? ?? ??";
        string opacity = "F3 0F 10 8B ?? ?? ?? ?? F3 0F 59 8B ?? ?? ?? ?? F3 0F 59 8B ?? ?? ?? ?? F3 0F 59 8B ?? ?? ?? ?? F3 0F 59 C1 48 83 C4 20 5B C3 90";
        string timestop_correct = "E9 ?? ?? ?? ?? 48 89 45 28 48 8B 45 20 48 8B 55 28 48 3B C2 E9 ?? ?? ?? ?? 48 8B 45 18 48 8B 55 10 8B 00 | 89 02";
    } aob;

    bool InterceptorHelper::initializeAOBBlocks(const LPBYTE hostImageAddress, DWORD hostImageSize, map<string, AOBBlock>& aobBlocks)
    {
        // Initialize all AOB blocks with their patterns
        aobBlocks[CAMERA_ADDRESS_INTERCEPT_KEY] = AOBBlock(CAMERA_ADDRESS_INTERCEPT_KEY, aob.camera_address, 1);
        aobBlocks[FOV_INTERCEPT_KEY] = AOBBlock(FOV_INTERCEPT_KEY, aob.fov, 1);
        aobBlocks[DOF_KEY] = AOBBlock(DOF_KEY, aob.dof, 1);
        aobBlocks[TIMESTOP_KEY] = AOBBlock(TIMESTOP_KEY, aob.timestop, 1);
        aobBlocks[TIMESTOP_KEY_NOP] = AOBBlock(TIMESTOP_KEY_NOP, aob.timestop_nop, 1);
        aobBlocks[PLAYERPOINTER_KEY] = AOBBlock(PLAYERPOINTER_KEY, aob.playerpointer, 1);
        aobBlocks[ENTITY_KEY] = AOBBlock(ENTITY_KEY, aob.entity, 1);
        aobBlocks[HUDTOGGLE] = AOBBlock(HUDTOGGLE, aob.hudtoggle, 1);
        aobBlocks[PAUSE_BYTE] = AOBBlock(PAUSE_BYTE, aob.pause_byte, 1);
        aobBlocks[VIGNETTE_KEY] = AOBBlock(VIGNETTE_KEY, aob.vignette, 1);
        aobBlocks[BLOOM_KEY] = AOBBlock(BLOOM_KEY, aob.bloom, 1);
        aobBlocks[OPACITY_KEY] = AOBBlock(OPACITY_KEY, aob.opacity, 1);
        //aobBlocks[TIMESTOP_CORRECT_KEY] = AOBBlock(TIMESTOP_CORRECT_KEY, aob.timestop_correct, 1);

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
        return tryInitHook(aobBlocks, CAMERA_ADDRESS_INTERCEPT_KEY, [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0x23, &_cameraStructInterceptionContinue, &cameraStructInterceptor);
            });
    }

    bool InterceptorHelper::setPostCameraStructHooks(map<string, AOBBlock>& aobBlocks)
    {
        bool result = true;

        // Set up DOF hook
        result &= tryInitHook(aobBlocks, DOF_KEY,
            [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0xF, &_dofInterceptionContinue, &dofStruct);
            });

        // Set up timestop hook
        result &= tryInitHook(aobBlocks, TIMESTOP_KEY,
            [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0xF, &_timestructinterceptionContinue, &timescaleinterceptor);
            });


        // Set up opacity hook
        result &= tryInitHook(aobBlocks, OPACITY_KEY,
            [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0x10, &_entityopacityContinue, &entityopacityinterceptor);
            });

        // Set up player pointer hook
        result &= tryInitHook(aobBlocks, PLAYERPOINTER_KEY,
            [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0xF, &_playerpointerContinue, &playerpointerinterceptor);
            });

        // Set up entity time hook
        result &= tryInitHook(aobBlocks, ENTITY_KEY,
            [](AOBBlock& block) {
                GameImageHooker::setHook(&block, 0x10, &_entitytimeContinue, &entitytimeinterceptor);
            });

        return result;
    }

    bool InterceptorHelper::toggleHud(map<string, AOBBlock>& aobBlocks, bool hudVisible)
    {
        try {
            Utils::toggleNOPState(aobBlocks[HUDTOGGLE], 4, !hudVisible);
            return true;
        }
        catch (const exception& e) {
            MessageHandler::logError("Failed to toggle HUD: %s", e.what());
            return false;
        }
    }

    bool InterceptorHelper::togglePause(map<string, AOBBlock>& aobBlocks, bool enabled)
    {
        try {
            uint8_t pause[] = { 0x00, 0x00, 0x00, 0x00 };
            Utils::SaveBytesWrite(aobBlocks[PAUSE_BYTE], 4, pause, enabled);
            return true;
        }
        catch (const exception& e) {
            MessageHandler::logError("Failed to toggle pause: %s", e.what());
            return false;
        }
    }

    void InterceptorHelper::getAbsoluteAddresses(map<string, AOBBlock>& aobBlocks)
    {
        // This function can be extended to calculate absolute addresses from relative ones
        // if needed for the game-specific implementation
    }

    bool InterceptorHelper::cameraSetup(map<string, AOBBlock>& aobBlocks, bool enabled, GameAddressData& addressData)
    {
        try {
            // Setup FOV interception - direct access for performance
            Utils::toggleNOPState(aobBlocks[FOV_INTERCEPT_KEY], 3, enabled);

            // Handle DOF setup
            if (addressData.dofAddress == nullptr) {
                MessageHandler::logError("DOF Struct not found");
                return false;
            }

            const auto dof = reinterpret_cast<uint8_t*>(addressData.dofAddress + DOF_OFFSET);
            if (addressData.dofbyte == static_cast<uint8_t>(0)) {
                addressData.dofbyte = *dof;
            }
            *dof = enabled ? static_cast<uint8_t>(0) : addressData.dofbyte;

            return true;
        }
        catch (const exception& e) {
            MessageHandler::logError("Failed to set up camera: %s", e.what());
            return false;
        }
    }

    bool InterceptorHelper::toolsInit(map<string, AOBBlock>& aobBlocks)
    {
		// This function can be extended to initialize additional tools or patches
    	return false;
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