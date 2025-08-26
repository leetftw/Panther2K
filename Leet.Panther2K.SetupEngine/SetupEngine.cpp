/*
 * Leet's Panther 2K
 * This project is licensed under the Simple Classic Theme license, version 1.0.
 *
 * SetupEngine.cpp/h: Core setup engine. Responsible for validating runtime
 * parameters and installing Windows.
 */

#include "SetupEngine.h"

#include <wimgapi.h>
#include <iostream>

#include "../PugiXML/pugixml.hpp"

// Undocumented WIMGAPI flag, loads the file with solid compression
#define WIM_FLAG_SOLIDCOMPRESSION 0x20000000

#define STDOUT(buffer) WriteConsoleW(consoleHandle, buffer, lstrlenW(buffer), NULL, NULL)

Leet::Panther2K::SetupEngine::SetupEngine(Leet::Panther2K::Util::Logger* logger)
{
	bUseLegacy = false;
	dwCallbackThread = -1;
	installLog = logger;
#ifdef _DEBUG
	if (!installLog) installLog = new Leet::Panther2K::Util::Logger(L"PantherEngine.log", PANTHER_LL_VERBOSE);
#else
	if (!installLog) installLog = new Leet::Panther2K::Util::Logger(L"PantherEngine.log", PANTHER_LL_NORMAL);
#endif
	hWimFile = nullptr;
	hWimImage = nullptr;
	dwWimImageCount = -1;
	dwWimImageIndex = -1;

	wlogc(installLog, PANTHER_LL_BASIC, L"Panther2K Installation Engine Initialized. Version 2.0");
}

Leet::Panther2K::SetupEngine::~SetupEngine()
{
	freeWimFile();
}

void Leet::Panther2K::SetupEngine::SetUseLegacy(bool useLegacy)
{
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	const wchar_t* buffer = useLegacy ? L"[Engine] Using Legacy." : L"[Engine] Using UEFI.";
	wlogc(installLog, PANTHER_LL_DETAILED, buffer);

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

HRESULT Leet::Panther2K::SetupEngine::SetWimFile(const std::wstring& path)
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

		wlogf(installLog, PANTHER_LL_DETAILED, 100, installLog->GetLogLevel() == PANTHER_LL_VERBOSE ? 
			L"[Engine] (WIMCreateFile) Failed to load WIM file (0x%08x)." : L"[Engine] Failed to load WIM file (0x%08x).", result);

		return result;
	}

	wchar_t buffer[MAX_PATH];
	GetTempPathW(MAX_PATH, buffer);
	WIMSetTemporaryPath(hWimFile, buffer);
	dwWimImageCount = WIMGetImageCount(hWimFile);

	return S_OK;

}

HRESULT Leet::Panther2K::SetupEngine::GetWimInfo(PantherWimInfo** lpWimPtr)
{
	if (!hWimFile) 
		return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);

	wlogc(installLog, PANTHER_LL_DETAILED, L"[Engine] Parsing image information...");

	PantherWimInfo* wimInfo = static_cast<PantherWimInfo*>(safeLocalAlloc(installLog, 
		sizeof(PantherWimInfo) + (sizeof(PantherImageInfo) * dwWimImageCount)));

	wchar_t* str_ptr;
	long str_len;
	if (!WIMGetImageInformation(hWimFile, (PVOID*)&str_ptr, (PDWORD)&str_len))
	{
		// TODO: error handling
	}
	
	pugi::xml_document document;
	// Not a bug, first character is a UTF-16 BOM
	document.load_string(str_ptr + 1);
	LocalFree(str_ptr);

	auto elements = document.select_nodes(INODE("wim") INODE("image"));
	if (elements.size() != dwWimImageCount) return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	int i = 0;
	for (pugi::xpath_node node : elements)
	{
		// Note on INODE: it stand for (case-)insensitive node. When writing images with Wimgapi, all caps are
		// used. But in some scenarios, splitted/joined WIMs might have some names in lowercase. For those instances
		// it is needed to do a case-invariant XType querry. This involves mapping upperrcase alphabet to lowercase,
		// which is exactly what the INODE macro does.
		auto archNode = node.node().select_node(REL INODE("windows") INODE("arch"));
		if (!archNode) 
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		wimInfo->Images[i].Architecture = archNode.node().text().as_int();

		auto nameNode = node.node().select_node(REL INODE("displayname"));
		if (!nameNode) nameNode = node.node().select_node(INODE("wim") INODE("image") INODE("name"));
		if (!nameNode) 
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		wcscpy_s(wimInfo->Images[i].DisplayName, nameNode.node().text().get());

		auto lowDtNode = node.node().select_node(REL INODE("creationtime") INODE("lowpart"));
		if (!lowDtNode) 
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		wimInfo->Images[i].CreationTime.dwLowDateTime = wcstoul(lowDtNode.node().text().get(), nullptr, 16);

		auto highDtNode = node.node().select_node(REL INODE("creationtime") INODE("highpart"));
		if (!highDtNode) 
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		wimInfo->Images[i].CreationTime.dwHighDateTime = wcstoul(highDtNode.node().text().get(), nullptr, 16);

		// The actual space required is (<TOTALBYTES> - <HARDLINKBYTES>), but this gives extra headroom for temporary files
		auto totalBytesNode = node.node().select_node(REL INODE("totalbytes"));
		if (!totalBytesNode) 
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		wimInfo->Images[i].TotalSize = totalBytesNode.node().text().as_ullong();

		wlogf(installLog, PANTHER_LL_VERBOSE, MAX_PATH, L"[Engine] Found system image [%2d]: %s (Architecture %d) (Size: %llu bytes)",
			i + 1, wimInfo->Images[i].DisplayName, wimInfo->Images[i].Architecture, wimInfo->Images[i].TotalSize);

		i++;
	}

	wimInfo->ImageCount = dwWimImageCount;

	*lpWimPtr = wimInfo;
	return S_OK;
}

HRESULT Leet::Panther2K::SetupEngine::SetWimIndex(int index)
{
	if (!hWimFile)
		return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
	if (index < 1 || index > dwWimImageCount)
		return HRESULT_FROM_WIN32(ERROR_INDEX_OUT_OF_BOUNDS);

	wlogf(installLog, PANTHER_LL_DETAILED, MAX_PATH, L"[Engine] Loading image from index %d...", index);

	dwWimImageIndex = index;
	hWimImage = WIMLoadImage(hWimFile, dwWimImageIndex);
	if (!hWimImage)
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

HRESULT checkVolume(const std::wstring& volumeGuid, unsigned long long minSize, unsigned long long minSpace, ...)
{
	std::wstring path = L"\\\\?\\Volume" + volumeGuid + L"\\";

	// Buffer to store file system name
	wchar_t fsName[MAX_PATH];

	// Get file system information
	if (!GetVolumeInformationW(path.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fsName, MAX_PATH))
		return HRESULT_FROM_WIN32(GetLastError());

	// Check if the file system is in the supported list
	va_list fileSystems;
	va_start(fileSystems, minSpace);

	bool isSupported = false;
	while (true)
	{
		const wchar_t* fs = va_arg(fileSystems, wchar_t*);
		if (!fs) break;

		if (_wcsicmp(fsName, fs) == 0)
		{
			isSupported = true;
			break;
		}
	}
	va_end(fileSystems);

	if (!isSupported)
		return HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);

	// Get disk size and available space
	ULARGE_INTEGER fsSize;
	ULARGE_INTEGER fsSpace;
	if (!GetDiskFreeSpaceExW(path.c_str(), nullptr, &fsSize, &fsSpace))
		return HRESULT_FROM_WIN32(GetLastError());

	// Check if the disk meets size and space requirements
	if (fsSize.QuadPart < minSize)
		return HRESULT_FROM_WIN32(ERROR_DISK_FULL);

	if (fsSpace.QuadPart < minSpace)
		return HRESULT_FROM_WIN32(ERROR_DISK_FULL);

	// If everything is fine, return S_OK
	return S_OK;
}

HRESULT Leet::Panther2K::SetupEngine::SetBootVolume(const std::wstring& volumeGuid)
{
	// TODO: hardcoded limit?
	HRESULT res = checkVolume(volumeGuid, 50000000ULL, 0ULL, L"ntfs", L"fat32", nullptr);
	if (SUCCEEDED(res)) szBootPartition.assign(volumeGuid);
	return res;
}

HRESULT Leet::Panther2K::SetupEngine::SetSystemVolume(const std::wstring& volumeGuid)
{
	// TODO: hardcoded limit?
	HRESULT res = checkVolume(volumeGuid, 0ULL, 21474836480ULL, L"ntfs", nullptr);
	if (SUCCEEDED(res)) szSystemPartition.assign(volumeGuid);
	return res;
}

HRESULT Leet::Panther2K::SetupEngine::SetCallbackThread(unsigned int threadId)
{
	wchar_t buffer[MAX_PATH];
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadId);
	if (!hThread)
		return HRESULT_FROM_WIN32(GetLastError());

	CloseHandle(hThread);
	dwCallbackThread = threadId;
	return S_OK;
}

void Leet::Panther2K::SetupEngine::freeWimFile()
{
	if (!hWimFile)
		return;

	dwWimImageCount = -1;
	WIMCloseHandle(hWimFile);
	hWimImage = nullptr;
}

void Leet::Panther2K::SetupEngine::freeWimImage()
{
	if (!hWimImage)
		return;

	dwWimImageIndex = -1;
	WIMCloseHandle(hWimImage);
	hWimImage = nullptr;
}

HRESULT PrepareWindowsOld(std::wstring& fileSystemRoot)
{
	wchar_t pathBuffer[MAX_PATH];

	// Check if Windows.old doesn't exist, if so, no need to move anything
	swprintf_s(pathBuffer, L"%sWindows.old", fileSystemRoot.c_str());
	if (GetFileAttributesW(pathBuffer) == INVALID_FILE_ATTRIBUTES)
		return S_OK;

	// Otherwise move existing Windows.old to first-free Windows.old.xxx
	int i = 0;
	for (; i < 1000; i++)
	{
		swprintf_s(pathBuffer, L"%sWindows.old.%03d", fileSystemRoot.c_str(), i);
		
		if (GetFileAttributesW(pathBuffer) == INVALID_FILE_ATTRIBUTES)
			break;
	}

	// If there is no free Windows.old.xxx, fail
	if (i == 1000)
		return HRESULT_FROM_WIN32(ERROR_DISK_FULL);

	// Move Windows.old to Windows.old.i
	wchar_t winOld[MAX_PATH];
	swprintf_s(winOld, L"%sWindows.old", fileSystemRoot.c_str());
	if (!MoveFileW(winOld, pathBuffer))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

HRESULT CreateWindowsOld(std::wstring& fileSystemRoot)
{
	wchar_t pathBuffers[2][MAX_PATH];

	swprintf_s(pathBuffers[0], L"%sWindows.old", fileSystemRoot.c_str());
	if (!CreateDirectoryW(pathBuffers[0], NULL))
		return HRESULT_FROM_WIN32(GetLastError());

	// Move the following dirs:
	const wchar_t* dirsToMove[] =
	{
		L"Windows",
		L"Program Files",
		L"Program Files (x86)",
		L"ProgramData",
		L"Users",
		L"XboxGames"
	};

	for (const auto& dir : dirsToMove)
	{
		swprintf_s(pathBuffers[0], L"%s%s", fileSystemRoot.c_str(), dir);
		swprintf_s(pathBuffers[1], L"%sWindows.old\\%s", fileSystemRoot.c_str(), dir);
		if (!MoveFileW(pathBuffers[0], pathBuffers[1]) && GetLastError() != ERROR_FILE_NOT_FOUND) 
			return HRESULT_FROM_WIN32(GetLastError());
	}

	// Delete the following:
	// Page files
	swprintf_s(pathBuffers[0], L"%spagefile.sys", fileSystemRoot.c_str());
	if (!DeleteFileW(pathBuffers[0]) && GetLastError() != ERROR_FILE_NOT_FOUND) 
		return HRESULT_FROM_WIN32(GetLastError());
	swprintf_s(pathBuffers[0], L"%shiberfil.sys", fileSystemRoot.c_str());
	if (!DeleteFileW(pathBuffers[0]) && GetLastError() != ERROR_FILE_NOT_FOUND) 
		return HRESULT_FROM_WIN32(GetLastError());

	// TODO: Deleting directories is a pita and these are not really necessary
	// Documents and settings
	// $Windows-BT
	// $Windows-WS

	return S_OK;
}

HRESULT Leet::Panther2K::SetupEngine::StartInstallation()
{
	auto threadFunction = [](LPVOID pCode) -> DWORD WINAPI
	{
		HRESULT hResult;

		SetupEngine* engine = static_cast<SetupEngine*>(pCode);
		wlogc(engine->installLog, PANTHER_LL_BASIC, L"[Engine/Install thread] Starting installation.");

		wlogc(engine->installLog, PANTHER_LL_VERBOSE, L"[Engine/Install thread] Registering internal callback...");
		if (WIMRegisterMessageCallback(engine->hWimFile, (FARPROC)WimgapiCallback, pCode) == INVALID_CALLBACK_VALUE)
		{
			wlogc(engine->installLog, PANTHER_LL_NORMAL, L"[Engine/Install thread] Warning: Could not register internal WIMGAPI callback. No progress/warning information will be sent to the client during the installation.");
			PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_WARNING, HRESULT_FROM_WIN32(ERROR_IO_PENDING), reinterpret_cast<WPARAM>(L"Failed to register installation callback. The installation will still continue, but no progress information will be provided."));			
		}

		engine->hFileNameReadyEvent = CreateEventW(NULL, false, true, NULL);

		wlogc(engine->installLog, PANTHER_LL_DETAILED, L"[Engine/Install thread] Checking for old installations...");
		std::wstring path = L"\\\\?\\Volume" + engine->szSystemPartition + L"\\Windows";
		if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			path = L"\\\\?\\Volume" + engine->szSystemPartition + L"\\";
			wlogc(engine->installLog, PANTHER_LL_NORMAL, L"[Engine/Install thread] Old Windows installation detected! Creating Windows.old migration...");

			// Move existing Windows.old folder out of the way
			hResult = PrepareWindowsOld(path);
			if (FAILED(hResult))
			{
				// Cannot create installation.
				wlogerr(engine->installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to prepare Windows.old directory for migration. %s", hResult);
				PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_ERROR, hResult, 0);
				return hResult;
			}

			// Move files from volume root to Windows.old
			hResult = CreateWindowsOld(path);
			if (FAILED(hResult))
			{
				// Cannot create installation.
				// TODO: this doesn't do anything
				wlogerr(engine->installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] CRITICAL: Failed to move current installation into Windows.old. The installed operating system may have been corrupted as a result! %s", hResult);
				PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_ERROR, hResult, 0);
				return hResult;
			}
		}

		wlogc(engine->installLog, PANTHER_LL_NORMAL, L"[Engine/Install thread] Applying system image...");
		path = L"\\\\?\\Volume" + engine->szSystemPartition + L"\\";
		BOOL result = WIMApplyImage(engine->hWimImage, path.c_str(), WIM_FLAG_FILEINFO);
		wloglerr(engine->installLog, PANTHER_LL_DETAILED, MAX_PATH, engine->installLog->GetLogLevel() == PANTHER_LL_VERBOSE
			? L"[Engine/Install thread] (WIMApplyImage) %s"
			: L"[Engine/Install thread] %s");
		
		// TODO: this doesn't do anything
		if (result != TRUE)
		{
			int lastError = GetLastError();
			wloglerr(engine->installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] The system image could not be applied. The installation has failed. %s");
			PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_ERROR, HRESULT_FROM_WIN32(lastError), 0);
			return HRESULT_FROM_WIN32(lastError);
		}
		hResult = engine->createBootFiles();
		if (FAILED(hResult))
		{
			wlogerr(engine->installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to create boot files. %s", hResult);
			PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_ERROR, hResult, 0);
			return hResult;
		}
		wlogerr(engine->installLog, PANTHER_LL_DETAILED, MAX_PATH, engine->installLog->GetLogLevel() >= PANTHER_LL_VERBOSE
			? L"[Engine/Install thread] (createBootFiles) %s"
			: L"[Engine/Install thread] %s", hResult);

		wlogc(engine->installLog, PANTHER_LL_VERBOSE, L"[Engine/Install thread] Unregistering internal callback...");
		if (!WIMUnregisterMessageCallback(engine->hWimFile, (FARPROC)WimgapiCallback))
		{
			wlogc(engine->installLog, PANTHER_LL_NORMAL, L"[Engine/Install thread] Warning: Could not unregister internal WIMGAPI callback.");
		}

		if (engine->dwCallbackThread != -1)
		{
			wlogc(engine->installLog, PANTHER_LL_DETAILED, L"[Engine/Install thread] Installation finished, notifying client...");
			PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_FINISH, 0, 0);
		}

		return hResult;
	};

	DWORD threadId;
	HANDLE hThread = CreateThread(nullptr, 0, threadFunction, this, 0, &threadId);
	if (!hThread)
	{
		HRESULT result = HRESULT_FROM_WIN32(GetLastError());
		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH, L"[Engine] Failed to create installation thread: %s");
		return result;
	}

	wlogc(installLog, PANTHER_LL_BASIC, L"[Engine] Installation thread created.");
	return S_OK;
}

DWORD Leet::Panther2K::SetupEngine::WimgapiCallback(DWORD dwMessageId, WPARAM wParam, LPARAM lParam, PVOID pvUserData)
{
	SetupEngine* engine = static_cast<SetupEngine*>(pvUserData);

	switch (dwMessageId)
	{
	case WIM_MSG_PROGRESS:
		if (!(wParam % 10)) wlogf(engine->installLog, PANTHER_LL_DETAILED, MAX_PATH, L"[Engine/Install thread] Applying system image: %ud%%", (unsigned int)wParam);
		PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_PROGRESS, wParam, 0);
		break;
	case WIM_MSG_PROCESS:
		if (WaitForSingleObject(engine->hFileNameReadyEvent, 0) != WAIT_TIMEOUT)
			PostThreadMessageW(engine->dwCallbackThread, TM_PANTHER_FILENAME, wParam, (LPARAM)engine->hFileNameReadyEvent);
		break;
	case WIM_MSG_INFO:
		wlogerr(engine->installLog, PANTHER_LL_DETAILED, MAX_PATH, L"[Engine/Install thread] Information received from WIMGAPI:\r\n\tMessage:\t%s\r\nFile:\t%s", lParam, (LPWSTR)wParam);
		break;
	case WIM_MSG_WARNING:
		wlogerr(engine->installLog, PANTHER_LL_NORMAL, MAX_PATH, L"[Engine/Install thread] Warning received from WIMGAPI:\r\n\tMessage:\t%s\r\nFile:\t%s", lParam, (LPWSTR)wParam);
		break;
	case WIM_MSG_ERROR:
		wlogerr(engine->installLog, PANTHER_LL_BASIC, MAX_PATH, L"[Engine/Install thread] An error occurred while applying the system image:\r\n\tMessage:\t%s\r\nFile:\t%s", lParam, (LPWSTR)wParam);
		break;
	}
	
	return WIM_MSG_SUCCESS;
}