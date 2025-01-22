#include "pch.h"
#include "SetupCore.h"
#include <wimgapi.h>
#include <string>
#include <iostream>

// Undocumented WIMGAPI flag, loads the file with solid compression
#define WIM_FLAG_SOLIDCOMPRESSION 0x20000000
#define FACILITY_PANTHER2K 1337
#define STDOUT(buffer) WriteConsoleW(consoleHandle, buffer, lstrlenW(buffer), NULL, NULL)

Leet::Panther2K::SetupEngine::SetupCore::SetupCore(LibPanther::Logger* logger)
{
	bUseLegacy = false;
	dwCallbackThread = -1;

	logger->Write(PANTHER_LL_BASIC, L"Panther2K Installation Engine Initialized. Version 2.0");
}

Leet::Panther2K::SetupEngine::SetupCore::~SetupCore()
{
	freeWimFile();
}

void Leet::Panther2K::SetupEngine::SetupCore::SetUseLegacy(bool useLegacy)
{
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	const wchar_t* buffer = useLegacy ? L"[Engine] Using Legacy.\n" : L"[Engine] Using UEFI.\n";
	installLog->Write(PANTHER_LL_DETAILED, buffer);

	bUseLegacy = useLegacy;
}

inline bool EndsWithI(const wchar_t* str, const wchar_t* suffix)
{
	if (!str || !suffix)
		return false;

	size_t strLen = lstrlenW(str);
	size_t suffixLen = lstrlenW(suffix);

	if (suffixLen > strLen)
		return 0;

	return _wcsnicmp(str + strLen - suffixLen, suffix, suffixLen) == 0;
}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::SetWimFile(const std::wstring& path)
{
	// Free any previously opened handles
	freeWimImage();
	freeWimFile();

	// Load the wim file using wimgapi
	const unsigned int flags = EndsWithI(path.c_str(), L".esd") ? WIM_FLAG_SOLIDCOMPRESSION : 0;
	wlogf(installLog, PANTHER_LL_DETAILED, MAX_PATH + 100, L"[Engine] Loading WIM file '%s', (%s)...", path.c_str(), flags ? L"solid" : L"not solid");
	
	for (int compression = WIM_COMPRESS_NONE; compression <= WIM_COMPRESS_LZMS; compression++)
	{
		hWimFile = WIMCreateFile(path.c_str(), WIM_GENERIC_READ, WIM_OPEN_EXISTING, flags, compression, NULL);
		if (hWimFile) break;
	}
	if (!hWimFile)
	{
		HRESULT result = HRESULT_FROM_WIN32(GetLastError());

		installLog->WriteDirect(PANTHER_LL_DETAILED, L"[Engine] ");
		installLog->WriteDirect(PANTHER_LL_VERBOSE, L"(WIMCreateFile) ");
		wlogf(installLog, PANTHER_LL_DETAILED, 100, L"Failed to load WIM file (0x%08x).", result);

		return HRESULT_FROM_WIN32(GetLastError());
	}

	wchar_t buffer[MAX_PATH];
	GetTempPathW(MAX_PATH, buffer);
	WIMSetTemporaryPath(hWimFile, buffer);
	return S_OK;

}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::SetWimIndex(int index)
{
	if (index < 1 || index > dwWimImageCount)
		return HRESULT_FROM_WIN32(ERROR_INDEX_OUT_OF_BOUNDS);
	if (!hWimFile)
		return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);

	dwWimImageIndex = index;
	hWimImage = WIMLoadImage(hWimFile, dwWimImageIndex);
	if (!hWimImage)
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::SetBootVolume(const std::wstring& volumeGuid)
{
	std::wstring path = L"\\\\?\\" + volumeGuid;
	DWORD attrib = GetFileAttributesW(path.c_str());
	if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	// TODO:
	// Check if: filesystem and enough space


	szBootPartition.assign(volumeGuid);
	return E_NOTIMPL;
}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::SetSystemVolume(const std::wstring& volumeGuid)
{
	// TODO:
	// Check if the volume exists
	// Check if: filesystem and enough space

	szSystemPartition.assign(volumeGuid);
	return E_NOTIMPL;
}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::SetRecoveryVolume(const std::wstring& volumeGuid)
{
	// TODO:
	// Check if the volume exists
	// Check if: filesystem and enough space

	szRecoveryPartition.assign(volumeGuid);
	return E_NOTIMPL;
}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::SetCallbackThread(unsigned int threadId)
{
	wchar_t buffer[MAX_PATH];
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadId);
	if (!hThread)
	{
		wsprintfW(buffer, L"OpenThread failed: (0x%08x).\n", HRESULT_FROM_WIN32(GetLastError()));
		STDOUT(buffer);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	CloseHandle(hThread);
	dwCallbackThread = threadId;
	return S_OK;
}

void Leet::Panther2K::SetupEngine::SetupCore::freeWimFile()
{
	if (!hWimFile)
		return;

	dwWimImageCount = -1;
	wimImageInfos.clear();
	WIMCloseHandle(hWimFile);
	hWimImage = nullptr;
}

void Leet::Panther2K::SetupEngine::SetupCore::freeWimImage()
{
	if (!hWimImage)
		return;

	dwWimImageIndex = -1;
	WIMCloseHandle(hWimImage);
	hWimImage = nullptr;
}

HRESULT Leet::Panther2K::SetupEngine::SetupCore::StartInstallation()
{
	auto threadFunction = [](LPVOID pCode) -> DWORD WINAPI
	{
		SetupCore* engine = static_cast<SetupCore*>(pCode);
		HRESULT result = WIMApplyImage(engine->hWimImage, engine->szSystemPartition.c_str(), WIM_FLAG_FILEINFO);
		if (FAILED(result))
			return result;

		result = engine->createBootFiles();
		return result;
	};

	// TODO: Create
	DWORD threadId;
	HANDLE hThread = CreateThread(nullptr, 0, threadFunction, this, 0, &threadId);
	if (!hThread)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}
