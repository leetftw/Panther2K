// Panther2K.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Panther2K.h"
#include "WindowsSetup.h"
#include <iostream>

#include <PantherLogger.h>
#include <PantherConsole.h>
#include "SetupManager.h"

#include <windows.h>
#include "Gdiplus.h"
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")
#include <Shlwapi.h>
#include "pathcch.h"

// Command line builds (for testing and debugging)
int wmain(int argc, wchar_t** argv) 
{
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
    {
        MessageBoxW(NULL, L"Failed to initialize GDI+. Panther2K can not load.", L"Panther2K Early Init", MB_OK | MB_ICONERROR);
        return false;
    }

    // Load font
    HRSRC res = FindResourceW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDR_FONT_IBM), RT_RCDATA);
    HGLOBAL mem = LoadResource(GetModuleHandleW(NULL), res);
    void* data = LockResource(mem);
    size_t len = SizeofResource(GetModuleHandleW(NULL), res);
    DWORD nFonts = 0;
    HANDLE hFontRes = AddFontMemResourceEx(data, len, NULL, &nFonts);
    if (hFontRes == 0)
    {
        MessageBoxW(NULL, L"Failed to load font. Panther2K can not load.", L"Panther2K Early Init", MB_OK | MB_ICONERROR);
        return false;
    }

    //Test();
    LibPanther::Logger logger = LibPanther::Logger(L"debug.log", PANTHER_LL_VERBOSE);
    CustomConsole console;
    console.Init();
    ShowWindow(console.WindowHandle, SW_SHOW);

    Leet::Panther2K::SetupManager setup = Leet::Panther2K::SetupManager(&console, &logger);
    setup.RunSetup();

    Sleep(10000);

    return setup.GetResult();

    if (__argc == 2 && lstrcmpiW(__wargv[1], L"--pe") == 0) 
    {
        WindowsSetup::IsWinPE = true;
    }

    return WindowsSetup::RunSetup();
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
