/*
 * Leet's Panther 2K
 * This project is licensed under the Simple Classic Theme license, version 1.0.
 *
 * SetupEngineC.cpp/h: C interface for interacting with Panther2K SetupCore.
 */

#include <PantherLogger.h>
#include <vector>

#define BUILDING_C_LIB
#include "include/SetupEngineC.h"
#include "SetupEngine.h"

std::vector<HSetupEngine> engines = { };

HRESULT PantherCreateEngine(HSetupEngine* engine, const wchar_t* loggerFile, int logLevel)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

	LibPanther::Logger* logger = nullptr;
	DWORD dwAttrib = GetFileAttributes(loggerFile);
	if (!loggerFile || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	logger = new LibPanther::Logger(loggerFile, logLevel);
	return PantherCreateEngine(engine, logger);
}

HRESULT PantherCreateEngine(HSetupEngine* engine, void* logger)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

	*engine = new Leet::Panther2K::SetupEngine(static_cast<LibPanther::Logger*>(logger));
	engines.push_back(*engine);
	return S_OK;
}

HRESULT PantherFreeEngine(HSetupEngine engine)
{
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter != engines.end())
	{
		engines.erase(iter);
		Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
		delete engine;
	}
	return E_NOTIMPL;
}

HRESULT PantherEngineSetUseLegacy(HSetupEngine engine, bool useLegacy)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter == engines.end()) return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	lpEngine->SetUseLegacy(useLegacy);
	return S_OK;
}

HRESULT PantherEngineSetWimFile(HSetupEngine engine, const wchar_t* path)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter == engines.end()) return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->SetWimFile(path);
}

HRESULT PantherEngineGetWimInfo(HSetupEngine engine, PantherWimInfo** lpWimPtr)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->GetWimInfo(lpWimPtr);
}

HRESULT PantherEngineSetWimIndex(HSetupEngine engine, int index)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter == engines.end()) return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->SetWimIndex(index);
}

HRESULT PantherEngineSetBootVolume(HSetupEngine engine, const wchar_t* volumeGuid)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->SetBootVolume(volumeGuid);
}

HRESULT PantherEngineSetSystemVolume(HSetupEngine engine, const wchar_t* volumeGuid)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter == engines.end()) return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->SetSystemVolume(volumeGuid);
}

HRESULT PantherEngineSetCallbackThread(HSetupEngine engine, unsigned int threadId)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter == engines.end()) return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->SetCallbackThread(threadId);
}

HRESULT PantherEngineStartInstallation(HSetupEngine engine)
{
	if (engine == nullptr) return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	auto iter = std::find(engines.begin(), engines.end(), engine);
	if (iter == engines.end()) return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
	Leet::Panther2K::SetupEngine* lpEngine = static_cast<Leet::Panther2K::SetupEngine*>(engine);
	return lpEngine->StartInstallation();
}
