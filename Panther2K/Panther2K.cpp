// Panther2K.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Panther2K.h"
#include <iostream>

#include <PantherLogger.h>
#include <PantherConsole.h>
#include "SetupManager.h"

#include <windows.h>

// Command line builds (for testing and debugging)
int wmain(int argc, wchar_t** argv)
{
    // Initialize GDI+.
    printf("Panther2K Early load. Version " PANTHER_VERSION "\n");
    /*printf("Initializing GDI+...\n");
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
    {
        MessageBoxW(NULL, L"Failed to initialize GDI+. Panther2K can not load.", L"Panther2K Early Init", MB_OK | MB_ICONERROR);
        return false;
    }*/

    printf("Creating logger...\n");
    LibPanther::Logger logger = LibPanther::Logger(L"debug.log", PANTHER_LL_VERBOSE);
    printf("Creating console...\n");
    CustomConsole console;
    console.Init();
    ShowWindow(console.WindowHandle, SW_SHOW);

    Leet::Panther2K::SetupManager setup = Leet::Panther2K::SetupManager(&console, &logger);
    setup.RunSetup();

    return setup.GetResult();

    /*
    if (__argc == 2 && lstrcmpiW(__wargv[1], L"--pe") == 0) 
    {
        WindowsSetup::IsWinPE = true;
    }

    return WindowsSetup::RunSetup();
    */
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
