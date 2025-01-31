// Panther2K.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Panther2K.h"
#include "WindowsSetup.h"
#include <iostream>

void Test()
{
    wchar_t buffer[MAX_PATH];

    HANDLE hWim = WIMCreateFile(L"X:\\3. OS Installers\\database\\install.wim", WIM_GENERIC_READ, WIM_OPEN_EXISTING, 0, WIM_COMPRESS_XPRESS, NULL);
    if (!hWim)
    {
        std::wcout << L"WIMCreateFile Error: " << GetLastError();
        return;
    }

    GetTempPathW(MAX_PATH, buffer);
    WIMSetTemporaryPath(hWim, buffer);

    HANDLE hImage = WIMLoadImage(hWim, 1);
    if (!hImage)
    {
        std::wcout << L"WIMLoadImage Error: " << GetLastError();
        return;
    }

    BOOL result = WIMApplyImage(hImage, L"\\\\?\\Volume{33ecd09d-ffb6-422a-9662-d67b390b52c0}\\", 0);
    if (!result)
    {
        std::wcout << L"WIMApplyImage Error: " << GetLastError();
        return;
    }
}

// Command line builds (for testing and debugging)
int wmain(int argc, wchar_t** argv) 
{
    //Test();
    //return 0;

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
