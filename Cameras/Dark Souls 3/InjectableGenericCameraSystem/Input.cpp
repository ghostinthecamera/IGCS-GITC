////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2019, Frans Bouma
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
#include "Input.h"
#include "Globals.h"
#include <atomic>
#include <array>
#include <chrono>
#include <vector>
#include "MessageHandler.h"

namespace IGCS::Input
{
    using namespace std::chrono;

    // ---------------------------------------------------------------------
    // anonymous?namespace for file?local helpers / state                    
    // ---------------------------------------------------------------------
    namespace
    {
        // key state bit?patterns (mirrors WinAPI behaviour for consistency)
        constexpr uint8_t kStateNone = 0x00;
        constexpr uint8_t kStateDownFrame = 0x88;   // key went down this frame
        constexpr uint8_t kStateReleased = 0x08;   // key released this frame

        // hammer?protection window (ms)
        constexpr milliseconds kHammerWindow{ 250 };
        using Clock = steady_clock;

        // RAW keyboard / mouse state --------------------------------------
        std::array<uint8_t, 256> g_keyStates{ };
        std::array<uint8_t, 3> g_mouseButtonStates{ };
        std::atomic_long g_mouseDeltaX{ 0 };
        std::atomic_long g_mouseDeltaY{ 0 };
        std::atomic_short g_mouseWheelDelta{ 0 };

        // last accepted activation per action (for hammer protection) ------
        std::array<Clock::time_point, static_cast<size_t>(IGCS::ActionType::Amount)> g_lastActionTime{ };

        // Helper: are we dealing with a continuous action (move / rotate / FOV
        // adjust) that must remain unprotected?
        [[nodiscard]] bool isContinuousAction(IGCS::ActionType type) noexcept
        {
            using AT = IGCS::ActionType;
            switch (type)
            {
            case AT::MoveLeft: case AT::MoveRight:  case AT::MoveForward: case AT::MoveBackward:
            case AT::MoveUp:   case AT::MoveDown:
            case AT::RotateUp: case AT::RotateDown: case AT::RotateLeft:  case AT::RotateRight:
            case AT::TiltLeft: case AT::TiltRight:
			case AT::FovIncrease: case AT::FovDecrease: case AT::GamepadFastModifier: case AT::GamepadSlowModifier:
                return true;    // exempt from hammer protection
            default:
                return false;   // apply hammer protection
            }
        }

        // Helper to safely store a mouse button state change ----------------
        inline void setMouseButtonStateInternal(int index, bool down) noexcept
        {
            if (index < 0 || index >= static_cast<int>(g_mouseButtonStates.size()))
            {
                return;
            }
            g_mouseButtonStates[index] = down ? kStateDownFrame : kStateReleased;
        }
    }   // anonymous namespace

    // ---------------------------------------------------------------------
    // PUBLIC API (as declared in Input.h)                                   
    // ---------------------------------------------------------------------

    // -------------------- key / action helpers ---------------------------
    bool isActionActivated(ActionType type)
    {
        return isActionActivated(type, false);
    }

    bool isActionActivated(ActionType type, bool altCtrlOptional)
    {
        // Check if any binding for this action type is activated
        if (Globals::instance().isAnyBindingActivated(type, altCtrlOptional))
        {
            // continuous actions are NEVER hammer-protected
            if (isContinuousAction(type))
            {
                return true;
            }

            // hammer protection: ignore re-activations within kHammerWindow
            auto now = Clock::now();
            auto& lastTime = g_lastActionTime[static_cast<size_t>(type)];
            if (now - lastTime < kHammerWindow)
            {
                return false;   // still within hammer window - ignore
            }
            lastTime = now;     // accept & timestamp
            return true;
        }

        return false;
    }

    // -------------------- state resets -----------------------------------
    void resetKeyStates() noexcept
    {
        for (auto& v : g_keyStates)
        {
            v &= ~0x08;   // clear frame?specific bit so next keydown is fresh
        }
    }

    void resetMouseState() noexcept
    {
        for (auto& v : g_mouseButtonStates)
        {
            v &= ~0x08;
        }
        g_mouseWheelDelta = 0;
    }

    // -------------------- mouse deltas -----------------------------------
    void resetMouseDeltas() noexcept
    {
        g_mouseDeltaX = 0;
        g_mouseDeltaY = 0;
        g_mouseWheelDelta = 0;
    }

    void processRawMouseData(const RAWMOUSE* rmouse) noexcept
    {
        if (rmouse && rmouse->usFlags == MOUSE_MOVE_RELATIVE)
        {
            g_mouseDeltaX = rmouse->lLastX;
            g_mouseDeltaY = rmouse->lLastY;
        }
    }

    long getMouseDeltaX() noexcept { return g_mouseDeltaX; }
    long getMouseDeltaY() noexcept { return g_mouseDeltaY; }
    short getMouseWheelDelta()     noexcept { return g_mouseWheelDelta; }

    // -------------------- mouse buttons ----------------------------------
    bool isMouseButtonDown(int button)
    {
        if (button < 0 || button >= static_cast<int>(g_mouseButtonStates.size()))
        {
            return false;
        }
        return (g_mouseButtonStates[button] & 0xF0) == 0x80;
    }

    // -------------------- raw?input registration -------------------------
    void registerRawInput()
    {
        RAWINPUTDEVICE rid{ };
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.hwndTarget = Globals::instance().mainWindowHandle();

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        {
            MessageHandler::logError("Couldn't register raw input. Error code: %010x", GetLastError());
        }
        else
        {
            MessageHandler::logDebug("Raw input registered");
        }
    }

    // -------------------- Win32 message pump hook ------------------------
    bool handleMessage(LPMSG lpMsg)
    {
        if (!lpMsg || !lpMsg->hwnd)
        {
            return false;
        }

        HWND mainWnd = Globals::instance().mainWindowHandle();
        if (!mainWnd)
        {
            return false;
        }

        // convert to client?coords for possible downstream use
        ScreenToClient(mainWnd, &lpMsg->pt);
        bool handled = false;

        switch (lpMsg->message)
        {
        case WM_INPUT:
        {
            UINT size = 0;
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lpMsg->lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
            std::vector<uint8_t> buffer(size);
            UINT read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lpMsg->lParam), RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER));
            if (read == size)
            {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    processRawMouseData(&raw->data.mouse);
                    // buttons / wheel -------------------------------------------------
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)  setMouseButtonStateInternal(0, true);
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)    setMouseButtonStateInternal(0, false);
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) setMouseButtonStateInternal(1, true);
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)   setMouseButtonStateInternal(1, false);
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)setMouseButtonStateInternal(2, true);
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)  setMouseButtonStateInternal(2, false);
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                    {
                        g_mouseWheelDelta += static_cast<short>(raw->data.mouse.usButtonData) / WHEEL_DELTA;
                    }
                }
                if (raw->header.dwType == RIM_TYPEKEYBOARD && IGCS_SUPPORT_RAWKEYBOARDINPUT)
                {
                    if (raw->data.keyboard.VKey != 0xFF)
                    {
                        g_keyStates[raw->data.keyboard.VKey] = (raw->data.keyboard.Flags & RI_KEY_BREAK) == 0 ? kStateDownFrame : kStateReleased;
                    }
                }
            }
            handled = true;
        }
        break;

        // redirect standard key messages so the host game never sees them
        case WM_KEYDOWN:
            if (lpMsg->wParam < 256) g_keyStates[lpMsg->wParam] = kStateDownFrame;
            handled = true;
            break;
        case WM_KEYUP:
            if (lpMsg->wParam < 256) g_keyStates[lpMsg->wParam] = kStateReleased;
            handled = true;
            break;

        case WM_SYSKEYDOWN:
            if (IGCS_SUPPORT_RAWKEYBOARDINPUT)
            {
                if (lpMsg->wParam < 256) g_keyStates[lpMsg->wParam] = kStateDownFrame;
                handled = true;
            }
            break;
        case WM_SYSKEYUP:
            if (IGCS_SUPPORT_RAWKEYBOARDINPUT)
            {
                if (lpMsg->wParam < 256) g_keyStates[lpMsg->wParam] = kStateReleased;
                handled = true;
            }
            break;

            // swallow ALL mouse messages so they never reach the game
        case WM_CAPTURECHANGED: case WM_LBUTTONDBLCLK: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MOUSEACTIVATE: case WM_MOUSEHOVER: case WM_MOUSEHWHEEL:
        case WM_MOUSEMOVE: case WM_MOUSELEAVE: case WM_MOUSEWHEEL: case WM_NCHITTEST: case WM_NCLBUTTONDBLCLK:
        case WM_NCLBUTTONDOWN: case WM_NCLBUTTONUP: case WM_NCMBUTTONDBLCLK: case WM_NCMBUTTONDOWN: case WM_NCMBUTTONUP:
        case WM_NCMOUSEHOVER: case WM_NCMOUSELEAVE: case WM_NCMOUSEMOVE: case WM_NCRBUTTONDBLCLK: case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP: case WM_NCXBUTTONDBLCLK: case WM_NCXBUTTONDOWN: case WM_NCXBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_XBUTTONDBLCLK: case WM_XBUTTONDOWN: case WM_XBUTTONUP:
            handled = true;
            break;
        default:
            break;
        }

        // if IGCS camera disabled ? let the game also handle message -------
        if (!g_cameraEnabled)
        {
            return false;
        }
        return handled;
    }
}  // namespace IGCS::Input