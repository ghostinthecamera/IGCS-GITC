#pragma once
#include "stdafx.h"


namespace IGCS
{
    class WindowHook {
    public:
        static bool initialize();
        static void cleanup();

    private:
        typedef HWND(WINAPI* GetForegroundWindow_t)(void);
        static GetForegroundWindow_t _originalGetForegroundWindow;
        static HWND WINAPI hookedGetForegroundWindow(void);
    };
};