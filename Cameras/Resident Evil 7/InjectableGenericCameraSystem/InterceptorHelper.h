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
#pragma once
#include "stdafx.h"
#include <map>
#include <string>
#include <string_view>
#include "Utils.h"
#include "GameCameraData.h"
#include "AOBBlock.h"
#include "System.h"

namespace IGCS::GameSpecific
{
    /**
     * InterceptorHelper - Class responsible for finding and intercepting game memory
     * for camera manipulation and other game-specific features.
     */
    class InterceptorHelper
    {
    public:
        /**
         * Initializes AOBBlocks by scanning for memory patterns in the game executable.
         * @param hostImageAddress Base address of the game's image in memory
         * @param hostImageSize Size of the game's image in memory
         * @param aobBlocks Map to store found AOB patterns
         * @return True if all critical patterns were found, false otherwise
         */
        static bool initializeAOBBlocks(const LPBYTE hostImageAddress, DWORD hostImageSize,
            std::map<std::string, AOBBlock>& aobBlocks);

        /**
         * Sets up the main camera structure interceptor hook.
         * @return True if hook was successfully set, false otherwise
         */
        static bool setCameraStructInterceptorHook();

        /**
         * Sets up additional hooks needed after the camera structure is found.
         * @return True if all hooks were successfully set, false otherwise
         */
        static bool setPostCameraStructHooks();

        // Game-specific feature toggles

        /**
         * Toggles the game's HUD visibility.
         * @param hudVisible Whether the HUD should be visible
         * @return True if operation succeeded, false otherwise
         */
        static bool toggleHud(bool hudVisible);

        /**
         * Sets up the camera system with necessary memory modifications.
         * @param enabled Whether the custom camera should be enabled
         * @param addressData Game address data needed for camera manipulation
         * @return True if setup succeeded, false otherwise
         */
        static bool cameraSetup(bool enabled,
                                GameAddressData& addressData);

        /**
         * Retrieves absolute addresses for game variables from relative offsets.
         * @param aobBlocks Map containing AOB patterns
         */
        static void getAbsoluteAddresses(std::map<std::string, AOBBlock>& aobBlocks);

        /**
         * Initializes additional tools and patches needed by the camera system.
         * @return True if initialization succeeded, false otherwise
         */
        static bool toolsInit();

        /**
         * Applies user settings to the game state.
         * @param aobBlocks Map containing AOB patterns
         * @param addressData Game address data needed for settings application
         */
        static void handleSettings(std::map<std::string, AOBBlock>& aobBlocks,
            GameAddressData& addressData);

        /**
         * Toggles the game's pause state.
         * @param aobBlocks Map containing AOB patterns
         * @param enabled Whether the game should be paused
         * @return True if operation succeeded, false otherwise
         */
        static bool togglePause(std::map<std::string, AOBBlock>& aobBlocks, bool enabled);

        // Helper function for initialization hooks that need error checking
        static bool tryInitHook(const string& key, const function<void(AOBBlock&)>& operation)
        {
            try {
                if (auto& aob = System::instance().getAOBBlock(); aob.contains(key)) {
                    operation(aob[key]);
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
    };
}
