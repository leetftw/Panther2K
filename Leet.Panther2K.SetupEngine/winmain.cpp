#include "pch.h"

#include <PantherLogger.h>
#include "SetupEngine.h"
#define BUILDING_C_LIB
#include "SetupEngineC.h"

// Example C code
int APIENTRY wWinMain_C(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Initialize the engine
	HSetupEngine engine;
	HRESULT result = PantherCreateEngine(&engine, L"testc.log", PANTHER_LL_VERBOSE);
	if (FAILED(result)) return result;

	// Set source image
	result = PantherEngineSetWimFile(engine, L"G:\\sources\\boot.wim");
	if (FAILED(result)) return result;
	result = PantherEngineSetWimIndex(engine, 1);
	if (FAILED(result)) return result;

	// Set destination info
	result = PantherEngineSetUseLegacy(engine, false);
	if (FAILED(result)) return result;
	result = PantherEngineSetBootVolume(engine, L"{9c98dd49-2b9c-4b2a-a5c7-da8b16db65de}");
	if (FAILED(result)) return result;
	result = PantherEngineSetSystemVolume(engine, L"{e11579f5-7651-43f6-a02e-e5971bda25e4}");
	if (FAILED(result)) return result;

	// Create the message queue by calling PeekMessage
	MSG msg;
	PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	// Set callback thread
	result = PantherEngineSetCallbackThread(engine, GetCurrentThreadId());
	if (FAILED(result)) return result;

	// Begin the installation
	result = PantherEngineStartInstallation(engine);
	if (FAILED(result)) return result;

	// Implement message loop which handles callbacks
	// Required to determine when installation is finished and for progress info.
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		if (msg.message == WM_USER + 0x1337)
		{
			printf("[Client] Received confirmation, installation complete.");
			system("pause");
			break;
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

// Example C++ code
int APIENTRY wWinMain_CPP(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Create logger
	LibPanther::Logger logger = LibPanther::Logger(L"test.log", PANTHER_LL_VERBOSE);
	logger.Write(PANTHER_LL_NORMAL, L"Panther2K Technical Test Starting. Version 2.0");

	// Initialize the engine
	Leet::Panther2K::SetupEngine engine = Leet::Panther2K::SetupEngine(&logger);

	logger.Write(PANTHER_LL_NORMAL, L"[Client] Loading source files...");

	// Set the source image
	HRESULT result = engine.SetWimFile(L"G:\\sources\\boot.wim");
	if (FAILED(result)) return result;

	// TODO: ENUMERATE IMAGES
	PantherWimInfo* wimInfo;
	engine.GetWimInfo(&wimInfo);
	LocalFree(wimInfo);

	result = engine.SetWimIndex(1);
	if (FAILED(result)) return result; 

	logger.Write(PANTHER_LL_NORMAL, L"[Client] Setting destination info...");

	// Use UEFI
	engine.SetUseLegacy(false);
	
	// Set target volumes (Recovery is optional)
	// These volumes do not need mount points assigned to them
	result = engine.SetBootVolume(L"{9c98dd49-2b9c-4b2a-a5c7-da8b16db65de}");
	if (FAILED(result)) return result;
	result = engine.SetSystemVolume(L"{e11579f5-7651-43f6-a02e-e5971bda25e4}");
	if (FAILED(result)) return result;

	// Create the message queue by calling PeekMessage
	MSG msg;
	PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	logger.Write(PANTHER_LL_NORMAL, L"[Client] Registering callback and starting installation...");

	// Set callback thread
	engine.SetCallbackThread(GetCurrentThreadId());

	// Begin the installation
	engine.StartInstallation();
	
	// Implement message loop which handles callbacks
	// Required to determine when installation is finished and for progress info.
	logger.Write(PANTHER_LL_NORMAL, L"[Client] Waiting for completion notification.");
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		if (msg.message == WM_USER + 0x1337)
		{
			logger.Write(PANTHER_LL_NORMAL, L"[Client] Received confirmation, installation complete.");
			system("pause");
			break;
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int wmain(int argc, wchar_t** argv)
{
	wWinMain_CPP(GetModuleHandleW(NULL), nullptr, nullptr, 0);
}