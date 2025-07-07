// Panther2K.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Panther2K.h"
#include <iostream>

#include <PantherLogger.h>
#include <PantherConsole.h>
#include "SetupManager.h"

#include <windows.h>
#include "WinPartedDll.h"

// Command line builds (for testing and debugging)
int wmain(int argc, wchar_t** argv)
{
    printf("[wmain] Panther2K Early load. Version " BASE_VER_STRING "\n");

    printf("[wmain] Creating logger...\n");
    Leet::Panther2K::Util::Logger logger = Leet::Panther2K::Util::Logger(L"debug.log", PANTHER_LL_VERBOSE);

    // Separate stack frame to perform deinitialization
    HRESULT result;
    {
        printf("[wmain] Creating console...\n");
        Leet::Panther2K::Util::CustomConsole console;
        console.Init();
        ShowWindow(console.WindowHandle, SW_SHOW);

        //WinPartedDll::EnumVolumes(&console, &logger, nullptr);
        
        Leet::Panther2K::SetupManager setup = Leet::Panther2K::SetupManager(&console, &logger);
        setup.RunSetup();
        result = setup.GetResult();
    }

    // List unfreed memory
    safeCleanup((&logger));

    wchar_t returnString[MAX_PATH];
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, \
        result, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), returnString, MAX_PATH, NULL))
        wprintf_s(L"[wmain] Result from setup manager: %s", returnString);

    return result;
}

// Headless builds (for releases)
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return wmain(0, nullptr);
}
