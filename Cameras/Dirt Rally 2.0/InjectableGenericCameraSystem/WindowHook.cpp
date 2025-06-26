#include "stdafx.h"
#include "WindowHook.h"
#include "MinHook.h"
#include "Globals.h"
#include "MessageHandler.h"

// Static member initialization


namespace IGCS
{
    WindowHook::GetForegroundWindow_t WindowHook::_originalGetForegroundWindow = nullptr;

    bool WindowHook::initialize() {
        // Get the address of GetForegroundWindow from USER32.dll
        HMODULE user32 = GetModuleHandleW(L"USER32.dll");
        if (!user32) {
            MessageHandler::logError("WindowHook::initialize: Failed to get USER32.dll handle");
            return false;
        }

        FARPROC getForegroundWindowAddr = GetProcAddress(user32, "GetForegroundWindow");
        if (!getForegroundWindowAddr) {
            MessageHandler::logError("WindowHook::initialize: Failed to get GetForegroundWindow address");
            return false;
        }

        // Create the hook
        MH_STATUS status = MH_CreateHook(
            getForegroundWindowAddr,
            &WindowHook::hookedGetForegroundWindow,
            reinterpret_cast<void**>(&_originalGetForegroundWindow)
        );

        if (status != MH_OK) {
            MessageHandler::logError("WindowHook::initialize: Failed to create GetForegroundWindow hook, error: %d", status);
            return false;
        }

        // Enable the hook
        status = MH_EnableHook(getForegroundWindowAddr);
        if (status != MH_OK) {
            MessageHandler::logError("WindowHook::initialize: Failed to enable GetForegroundWindow hook, error: %d", status);
            return false;
        }

        MessageHandler::logLine("Successfully hooked GetForegroundWindow");
        return true;
    }

    void WindowHook::cleanup() {
        if (_originalGetForegroundWindow) {
            HMODULE user32 = GetModuleHandleW(L"USER32.dll");
            if (user32) {
                FARPROC getForegroundWindowAddr = GetProcAddress(user32, "GetForegroundWindow");
                if (getForegroundWindowAddr) {
                    MH_DisableHook(getForegroundWindowAddr);
                    MH_RemoveHook(getForegroundWindowAddr);
                }
            }
            _originalGetForegroundWindow = nullptr;
        }
    }

    HWND WINAPI WindowHook::hookedGetForegroundWindow(void) {
        // Get the actual foreground window
        HWND actualForeground = _originalGetForegroundWindow();

        // Check if it belongs to our process
        DWORD currentProcess = GetCurrentProcessId();
        DWORD foregroundProcess;
        GetWindowThreadProcessId(actualForeground, &foregroundProcess);

        // If it's our process, let it through normally
        if (foregroundProcess == currentProcess) {
            return actualForeground;
        }

        // Otherwise, return our game window
        HWND gameWindow = Globals::instance().mainWindowHandle();
        return gameWindow ? gameWindow : actualForeground;
    }

}