#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <PantherLogger.h>

struct ImageInfo
{
	unsigned int Architecture;
	wchar_t* DisplayName;
	SYSTEMTIME CreationTime;
	unsigned long long TotalSize;
};

namespace Leet::Panther2K::SetupEngine
{
	class SetupCore
	{
	public:
		SetupCore(LibPanther::Logger* logger);
		~SetupCore();

		// Setters for installation config
		void SetUseLegacy(bool useLegacy);

		HRESULT SetWimFile(const std::wstring& path);
		HRESULT SetWimIndex(int index);

		HRESULT SetBootVolume(const std::wstring& volumeGuid);
		HRESULT SetSystemVolume(const std::wstring& volumeGuid);
		HRESULT SetRecoveryVolume(const std::wstring& volumeGuid); // optional, if set to empty value the engine will not create a recovery partition

		HRESULT SetCallbackThread(unsigned int threadId);
		HRESULT StartInstallation();
	private:
		void freeWimFile();
		void freeWimImage();
		HRESULT createBootFiles();

		LibPanther::Logger* installLog;

		// WIMFILE data (legacy phase 2)
		std::wstring szWimPath;
		HANDLE hWimFile;
		int dwWimImageCount;
		std::vector<ImageInfo> wimImageInfos;
		int dwWimImageIndex;
		HANDLE hWimImage;

		// BOOT METHOD DATA (legacy phase 3)
		bool bUseLegacy;

		// PARTITION data (legacy phase 4)
		std::wstring szBootPartition;
		std::wstring szSystemPartition;
		std::wstring szRecoveryPartition;

		unsigned int dwCallbackThread;
	};
}

