/*
 * Leet's Panther 2K
 * This project is licensed under the Simple Classic Theme license, version 1.0.
 *
 * SetupEngineC.cpp/h: C interface for interacting with Panther2K SetupCore.
 */

#pragma once

#include "../engine_def.h"

#ifdef BUILDING_C_LIB
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

typedef void* HSetupEngine;
EXPORT HRESULT PantherCreateEngine(HSetupEngine* engine, const wchar_t* loggerFile, int logLevel);
EXPORT HRESULT PantherCreateEngine(HSetupEngine* engine, void* logger);

EXPORT HRESULT PantherFreeEngine(HSetupEngine engine);

EXPORT HRESULT PantherEngineSetUseLegacy(HSetupEngine engine, bool useLegacy);

EXPORT HRESULT PantherEngineSetWimFile(HSetupEngine engine, const wchar_t* path);
EXPORT HRESULT PantherEngineGetWimInfo(HSetupEngine engine, PantherWimInfo** lpWimPtr);
EXPORT HRESULT PantherEngineSetWimIndex(HSetupEngine engine, int index);

EXPORT HRESULT PantherEngineSetBootVolume(HSetupEngine engine, const wchar_t* volumeGuid);
EXPORT HRESULT PantherEngineSetSystemVolume(HSetupEngine engine, const wchar_t* volumeGuid);
EXPORT HRESULT PantherEngineSetRecoveryVolume(HSetupEngine engine, const wchar_t* volumeGuid); // optional, if set to empty value the engine will not create a recovery partition

EXPORT HRESULT PantherEngineSetCallbackThread(HSetupEngine engine, unsigned int threadId);
EXPORT HRESULT PantherEngineStartInstallation(HSetupEngine engine);