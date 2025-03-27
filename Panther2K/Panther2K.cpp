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
    printf("Panther2K Early load. Version " PANTHER_VERSION "\n");

    printf("Creating logger...\n");
    LibPanther::Logger logger = LibPanther::Logger(L"debug.log", PANTHER_LL_VERBOSE);

    // Separate stack frame to perform deinitialization
    HRESULT result;
    {
        printf("Creating console...\n");
        CustomConsole console;
        console.Init();
        ShowWindow(console.WindowHandle, SW_SHOW);

        //WinPartedDll::EnumVolumes(&console, &logger, nullptr);
        
        Leet::Panther2K::SetupManager setup = Leet::Panther2K::SetupManager(&console, &logger);
        setup.RunSetup();
        result = setup.GetResult();
    }

    // List unfreed memory
    safeCleanup(&logger);

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
