/*
 * Leet's Panther 2K
 * This project is licensed under the Simple Classic Theme license, version 1.0.
 *
 * SetupEngine.cpp/h: Core setup engine. Responsible for validating runtime
 * parameters and installing Windows.
 */

#pragma once
#include <windows.h>
#include <PantherLogger.h>
#include <string>

#include "engine_def.h"

namespace Leet::Panther2K
{
	class SetupEngine
	{
	public:
		SetupEngine(LibPanther::Logger* logger);
		~SetupEngine();

		// Setters for installation config
		void SetUseLegacy(bool useLegacy);

		HRESULT SetWimFile(const std::wstring& path);
		HRESULT GetWimInfo(PantherWimInfo** lpWimPtr);
		HRESULT SetWimIndex(int index);

		HRESULT SetBootVolume(const std::wstring& volumeGuid);
		HRESULT SetSystemVolume(const std::wstring& volumeGuid);
		HRESULT SetRecoveryVolume(const std::wstring& volumeGuid); // optional, if set to empty value the engine will not create a recovery partition

		HRESULT SetCallbackThread(unsigned int threadId);
		HRESULT StartInstallation();
	private:
		void freeWimFile();
		void freeWimImage();

		static DWORD CALLBACK WimgapiCallback(DWORD dwMessageId, WPARAM wParam, LPARAM lParam, PVOID pvUserData);
		HRESULT createBootFiles();

		LibPanther::Logger* installLog;

		// WIMFILE data (legacy phase 2)
		std::wstring szWimPath;
		HANDLE hWimFile;
		int dwWimImageCount;
		int dwWimImageIndex;
		HANDLE hWimImage;

		// BOOT METHOD DATA (legacy phase 3)
		bool bUseLegacy;

		// PARTITION data (legacy phase 4)
		std::wstring szBootPartition;
		std::wstring szSystemPartition;
		std::wstring szRecoveryPartition;

		unsigned int dwCallbackThread;
		HANDLE hFileNameReadyEvent;
	};
}

