#include "SetupEngine.h"

#include "BootSector.h"

BOOL SetPrivilege(
	HANDLE hToken,              // access token handle
	LPCWSTR nameOfPrivilege,   // name of privilege to enable/disable
	BOOL bEnablePrivilege     // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,               // lookup privilege on local system
		nameOfPrivilege,   // privilege to lookup 
		&luid))           // receives LUID of privilege
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		return FALSE;
	}

	return TRUE;
}

int CreateProcessPiped(char* outputBuffer, int bufferSize, wchar_t* commandLine)
{
	// Create pipe for redirecting STDOUT
	HANDLE stdOutRd;
	HANDLE stdOutWr;
	SECURITY_ATTRIBUTES attributes = { 0 };
	attributes.bInheritHandle = TRUE;
	attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	if (!CreatePipe(&stdOutRd, &stdOutWr, &attributes, 0))
	{
		// Failed
		int temp = GetLastError();
		return temp;
	}
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = stdOutWr;

	PROCESS_INFORMATION pi = { 0 };
	if (!CreateProcessW(NULL, commandLine, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
	{
		// Failed
		int temp = GetLastError();
		return temp;
	}

	// Close all handles except stdOutRd
	// We know bcdedit is done when stdOutRd receives an EOF
	CloseHandle(stdOutWr);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// Read until the end of the pipe is reached or when the buffer is full
	DWORD bytesRead;
	BOOL retCode;
	for (char* buffer = outputBuffer; (buffer - outputBuffer < bufferSize) && (retCode = ReadFile(stdOutRd, buffer, 1, &bytesRead, NULL)); buffer += bytesRead);

	// Check the error code
	if (int temp = GetLastError() != ERROR_BROKEN_PIPE)
		return temp;

	// If the end of the pipe was not reached but the buffer is full, read it to the end
	CHAR voidBuffer;
	while (retCode) retCode = ReadFile(stdOutRd, &voidBuffer, 1, &bytesRead, NULL);

	// Check the error code again to make sure nothing went wrong
	if (int temp = GetLastError() != ERROR_BROKEN_PIPE)
		return temp;

	return ERROR_SUCCESS;
}

void EnsureTrailingBackslash(std::wstring& path) 
{
	if (!path.empty() && path.back() != L'\\') 
	{
		path += L'\\';
	}
}

bool CreateDirectoryRecursive(const std::wstring& path)
{
	if (CreateDirectoryW(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) 
	{
		return true;
	}

	size_t pos = path.find_last_of(L'\\');
	if (pos == std::wstring::npos) 
	{
		return false;
	}

	if (!CreateDirectoryRecursive(path.substr(0, pos))) 
	{
		return false;
	}

	return CreateDirectoryW(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool CopyDirectoryRecursive(std::wstring& srcPath, std::wstring& dstPath) 
{
	EnsureTrailingBackslash(srcPath);
	EnsureTrailingBackslash(dstPath);

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFileW((srcPath + L"*").c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		return false;
	}

	// Create destination directory
	if (!CreateDirectoryRecursive(dstPath)) 
	{
		FindClose(hFind);
		return false;
	}

	do 
	{
		std::wstring fileName = findFileData.cFileName;
		if (fileName == L"." || fileName == L"..") {
			continue;
		}

		std::wstring fullSrcPath = srcPath + fileName;
		std::wstring fullDstPath = dstPath + fileName;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			// Recursively copy subdirectory
			if (!CopyDirectoryRecursive(fullSrcPath, fullDstPath)) 
			{
				FindClose(hFind);
				return false;
			}
		}
		else 
		{
			// Copy file
			if (!CopyFileW(fullSrcPath.c_str(), fullDstPath.c_str(), FALSE)) 
			{
				FindClose(hFind);
				return false;
			}
		}
	} while (FindNextFileW(hFind, &findFileData));

	FindClose(hFind);
	return true;
}

bool CopyDirectoryRecursive(const wchar_t* source, const wchar_t* destination)
{
	std::wstring srcStr(source);
	std::wstring dstStr(destination);
	return CopyDirectoryRecursive(srcStr, dstStr);
}

HRESULT GetVolumeDiskExtents(std::wstring& volumePath, VOLUME_DISK_EXTENTS** diskExtents)
{
	HANDLE hVolume = CreateFileW(volumePath.c_str(), FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		HRESULT res = HRESULT_FROM_WIN32(GetLastError());
		return res;
	}

	size_t size = sizeof(VOLUME_DISK_EXTENTS);
	VOLUME_DISK_EXTENTS* extents = static_cast<VOLUME_DISK_EXTENTS*>(malloc(size));
	if (!extents) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OUTOFMEMORY);
	BOOL ioResult = DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, extents, size, NULL, NULL);
	while (!ioResult && GetLastError() == ERROR_MORE_DATA)
	{
		size = (extents->NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT) + sizeof(VOLUME_DISK_EXTENTS);
		extents = static_cast<VOLUME_DISK_EXTENTS*>(realloc(extents, size));
		if (!extents) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OUTOFMEMORY);

		ioResult = DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, extents, size, NULL, NULL);
	}

	HRESULT hResult = S_OK;
	if (!ioResult) hResult = HRESULT_FROM_WIN32(GetLastError());
	else *diskExtents = extents;

	CloseHandle(hVolume);
	return hResult;
}

HRESULT Leet::Panther2K::SetupEngine::createBootFiles()
{
	installLog->Write(PANTHER_LL_NORMAL, L"[Engine/Install thread] Generating Windows Boot Manager files...");

	wchar_t pathBuffers[2][MAX_PATH];
	std::wstring systemVolumePath = L"\\\\?\\Volume" + szSystemPartition;
	std::wstring bootVolumePath = L"\\\\?\\Volume" + szBootPartition;
	std::wstring recoveryVolumePath = L"\\\\?\\Volume" + szRecoveryPartition;

	/*
	* Determine if all partitions are on the same disk
	* (i.e. If the system partition is on the same disk as the boot partition.)
	*
	* If they are, a different bcdedit flag (hd_partition=) must be used,
	* otherwise the BCD might reference a VHD file that is actually mounted
	* in the VM instead of the partitions themselves.
	*
	* This was found during test installs on a VHD. It ouright refused to
	* boot until I realized it was trying to find a non-existent VHD.
	*/

	installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Determining whether the system will be installed on a single disk...");
	
	int diskNumber = -1;
	bool singleDisk = true;

	VOLUME_DISK_EXTENTS* extents = nullptr;
	HRESULT result = GetVolumeDiskExtents(bootVolumePath, &extents);
	if (FAILED(result))
	{
		if (result == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OUTOFMEMORY))
		{
			installLog->WriteDirect(PANTHER_LL_BASIC, L"OUT OF MEMORY!!!");
			exit(ERROR_OUTOFMEMORY);
			return result;
		}

		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to retrieve volume extents. %s (0x%08X)", result);
		return result;
	}

	if (extents->NumberOfDiskExtents != 1)
	{
		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] The operation cannot continue. The volume contains more than one extent. %s (0x%08X)", result);
		return HRESULT_FROM_WIN32(ERROR_VOLMGR_DYNAMIC_DISK_NOT_SUPPORTED);
	}

	diskNumber = extents->Extents[0].DiskNumber;
	free(extents);
	extents = nullptr;

	result = GetVolumeDiskExtents(systemVolumePath, &extents);
	if (FAILED(result))
	{
		if (result == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OUTOFMEMORY))
		{
			installLog->WriteDirect(PANTHER_LL_BASIC, L"OUT OF MEMORY!!!");
			exit(ERROR_OUTOFMEMORY);
			return result;
		}

		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to retrieve volume extents. %s (0x%08X)", result);
		return result;
	}

	if (extents->NumberOfDiskExtents != 1)
	{
		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] The operation cannot continue. The volume contains more than one extent. %s (0x%08X)", result);
		return HRESULT_FROM_WIN32(ERROR_VOLMGR_DYNAMIC_DISK_NOT_SUPPORTED);
	}

	singleDisk = extents->Extents[0].DiskNumber == diskNumber;
	installLog->Write(PANTHER_LL_DETAILED, singleDisk ? L"[Engine/Install thread] The system will be installed on a single disk, not using VHD detection."
		: L"[Engine/Install thread] The system will be installed on multiple disks, using VHD detection.");

	/*
	* Copy boot files
	*/
	installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Copying boot files...");
	swprintf_s(pathBuffers[0], bUseLegacy ? L"%s\\Windows\\Boot\\PCAT\\"
			: L"%s\\Windows\\Boot\\EFI\\", systemVolumePath.c_str());
	swprintf_s(pathBuffers[1], bUseLegacy ? L"%s\\Boot"
		: L"%s\\EFI\\Microsoft\\Boot", bootVolumePath.c_str());

	if (!CopyDirectoryRecursive(pathBuffers[0], pathBuffers[1]))
	{
		HRESULT res = HRESULT_FROM_WIN32(GetLastError());
		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to copy boot files. %s (0x%08X)", res);
		return res;
	}

	/*
	* Move Windows Boot Manager to the correct location
	*/

	installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Copying boot manager to proper location...");
	if (!bUseLegacy)
	{
		swprintf_s(pathBuffers[1], L"%s\\EFI\\Boot", bootVolumePath.c_str());
		if (!CreateDirectoryRecursive(pathBuffers[1]) && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			HRESULT res = HRESULT_FROM_WIN32(GetLastError());
			wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to create directory for boot manager executable. %s (0x%08X)", res);
			return res;
		}
	}
	
	swprintf_s(pathBuffers[0], bUseLegacy ? L"%s\\Boot\\bootmgr"
		: L"%s\\EFI\\Microsoft\\Boot\\bootmgfw.efi", bootVolumePath.c_str());
	swprintf_s(pathBuffers[1], bUseLegacy ? L"%s\\bootmgr"
		: L"%s\\EFI\\Boot\\bootx64.efi", bootVolumePath.c_str());

	if (!CopyFileW(pathBuffers[0], pathBuffers[1], FALSE))
	{
		HRESULT res = HRESULT_FROM_WIN32(GetLastError());
		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to copy boot manager executable. %s (0x%08X)", res);
		return res;
	}

	/*
	* Check if BCD store already exists
	*/
	installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Looking for existing BCD...");
	wchar_t commandBuffer[MAX_PATH + 128];
	swprintf_s(commandBuffer, bUseLegacy? L"%s\\Boot\\BCD"
		: L"%s\\EFI\\Microsoft\\Boot\\BCD", bootVolumePath.c_str());
	DWORD dwAttrib = GetFileAttributes(commandBuffer);

	if (dwAttrib == INVALID_FILE_ATTRIBUTES ||
		dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
	{
		installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] No BCD exists. Creating a new store...");
	
		/*
		* Create BCD store
		*/
		swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /createstore %s\\Boot\\BCD"
			: L"bcdedit /createstore %s\\EFI\\Microsoft\\Boot\\BCD", bootVolumePath.c_str());
		if (int temp = _wsystem(commandBuffer))
		{
			wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to create BCD store. %s (0x%08X)", temp, temp);
			return temp;
		}

		/*
		* Add Windows Boot Manager entry
		*/
		swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /store %s\\Boot\\BCD /create {bootmgr}"
			: L"bcdedit /store %s\\EFI\\Microsoft\\Boot\\BCD /create {bootmgr}", bootVolumePath.c_str());
		if (int temp = _wsystem(commandBuffer))
		{
			wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to create BCD {bootmgr} entry. %s (0x%08X)", temp, temp);
			return temp;
		}

		/*
		* Configure Windows Boot Manager entry
		*/
		// device: hd_partition=Volume{}
		swprintf_s(commandBuffer, L"bcdedit /store %s\\%s /set {bootmgr} device %sVolume%s",
			bootVolumePath.c_str(),
			bUseLegacy ? L"Boot\\BCD" : L"EFI\\Microsoft\\Boot\\BCD",
			singleDisk ? L"hd_partition=" : L"partition=",
			szBootPartition.c_str());
		if (int temp = _wsystem(commandBuffer))
		{
			wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD {bootmgr} properties. %s (0x%08X)", temp, temp);
			return temp;
		}
		swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /store %s\\Boot\\BCD /set {bootmgr} path \\bootmgr"
			: L"bcdedit /store %s\\EFI\\Microsoft\\Boot\\BCD /set {bootmgr} path \\EFI\\Microsoft\\Boot\\bootmgfw.efi", bootVolumePath.c_str());
		if (int temp = _wsystem(commandBuffer))
		{
			wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD {bootmgr} properties. %s (0x%08X)", temp, temp);
			return temp;
		}

		installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Adding newly installed OS to BCD...");
	}
	else installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] A BCD already exists, adding newly installed OS to it...");

	/*
	* Add Windows Boot Loader entry
	*/

	// Call bcdedit while redirecting stdout
	swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /store %s\\Boot\\BCD /create /application osloader"
		: L"bcdedit /store %s\\EFI\\Microsoft\\Boot\\BCD /create /application osloader", bootVolumePath.c_str());

	// Read the GUID of the created osloader entry
	char guidBuffer[256];
	if (int temp = CreateProcessPiped(guidBuffer, 256, commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to create BCD OS Loader entry. %s (0x%08X)", temp, temp);
		return temp;
	}

	// Convert the string containing the GUID into a wide char string
	wchar_t guidString[128];
	MultiByteToWideChar(GetConsoleCP(), MB_PRECOMPOSED, guidBuffer, 128, guidString, 128);

	// Find GUID of created entry
	guidString[127] = 0;
	wchar_t* guid = wcschr(guidString, L'{');
	if (guid == 0)
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Could not determine BCD OS Loader GUID. %s (0x%08X)", ERROR_INVALID_DATA, ERROR_INVALID_DATA);
		return ERROR_INVALID_DATA;
	}
	guid[38] = 0;

	/*
	* Configure Windows Boot Loader entry
	*/

	installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Configuring Windows Boot Loader...");

	// Set it to default
	swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /store %s\\Boot\\BCD /default %s"
		: L"bcdedit /store %s\\EFI\\Microsoft\\Boot\\BCD /default %s", bootVolumePath.c_str(), guid);
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set default BCD OS Loader entry. %s (0x%08X)", temp, temp);
		return temp;
	}

	// osdevice = W:
	swprintf_s(commandBuffer, L"bcdedit /store %s\\%s /set {default} osdevice %sVolume%s",
		bootVolumePath.c_str(),
		bUseLegacy ? L"Boot\\BCD" : L"EFI\\Microsoft\\Boot\\BCD",
		singleDisk ? L"hd_partition=" : L"partition=",
		szSystemPartition.c_str());
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD OS Loader osdevice property. %s (0x%08X)", temp, temp);
		return temp;
	}

	// device = W:
	swprintf_s(commandBuffer, L"bcdedit /store %s\\%s /set {default} device %sVolume%s",
		bootVolumePath.c_str(),
		bUseLegacy ? L"Boot\\BCD" : L"EFI\\Microsoft\\Boot\\BCD",
		singleDisk ? L"hd_partition=" : L"partition=",
		szSystemPartition.c_str());
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD OS Loader device property. %s (0x%08X)", temp, temp);
		return temp;
	}
	
	// systemroot = \Windows
	swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /store %s\\Boot\\BCD /set {default} systemroot \\Windows"
		: L"bcdedit /store %s\\EFI\\Microsoft\\Boot\\BCD /set {default} systemroot \\Windows", bootVolumePath.c_str());
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD OS Loader systemroot property. %s (0x%08X)", temp, temp);
		return temp;
	}

	// path = \Windows\System32\winload.exe / .efi
	swprintf_s(commandBuffer, bUseLegacy ? L"bcdedit /store %s\\Boot\\BCD /set {default} path \\Windows\\System32\\winload.exe"
		: L"bcdedit /store %s\\EFI\\Microsoft\\Boot\\BCD /set {default} path \\Windows\\System32\\winload.efi", bootVolumePath.c_str());
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD OS Loader path property. %s (0x%08X)", temp, temp);
		return temp;
	}

	// Set a name and display order, otherwise Boot Manager will refuse to boot the entry
	swprintf_s(commandBuffer, L"bcdedit /store %s\\%s /set {default} description \"Windows\"",
		bootVolumePath.c_str(),
		bUseLegacy ? L"Boot\\BCD" : L"EFI\\Microsoft\\Boot\\BCD");
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD OS Loader description property. %s (0x%08X)", temp, temp);
		return temp;
	}

	swprintf_s(commandBuffer, L"bcdedit /store %s\\%s /displayorder {default} /addfirst",
		bootVolumePath.c_str(),
		bUseLegacy ? L"Boot\\BCD" : L"EFI\\Microsoft\\Boot\\BCD");
	if (int temp = _wsystem(commandBuffer))
	{
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD display order. %s (0x%08X)", temp, temp);
		return temp;
	}

	/*
	* TODO: SETUP WINDOWS RECOVERY ENVIRONMENT
	* 1. Create files
	* 2. Create recovery entry
	* 3. Configure using reagentc
	* 4. Set partition type to recovery
	*/
	if (false)
	{

	}

	/*
	* Convert the hive into a system hive
	* This is done through the registry as bcdedit does not allow this
	* This is REQUIRED for bcdedit to work in the installed system
	* and thus for sysprep to succeed in specializing the system
	*/

	installLog->Write(PANTHER_LL_DETAILED, L"[Engine/Install thread] Converting BCD into a system boot configuration store...");

	// Enable registry access first
	HANDLE proccessHandle = GetCurrentProcess();
	DWORD typeOfAccess = TOKEN_ADJUST_PRIVILEGES;
	HANDLE tokenHandle;
	if (!OpenProcessToken(proccessHandle, typeOfAccess, &tokenHandle)
		|| !SetPrivilege(tokenHandle, SE_RESTORE_NAME, TRUE)
		|| !SetPrivilege(tokenHandle, SE_BACKUP_NAME, TRUE))
	{
		int res = HRESULT_FROM_WIN32(GetLastError());
		wloglerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to elevate process registry privileges. %s (0x%08X)", res);
		return res;
	}

	// Set the settings for system hives
	LSTATUS status;
	swprintf_s(commandBuffer, bUseLegacy ? L"%s\\Boot\\BCD" : L"%s\\EFI\\Microsoft\\Boot\\BCD", bootVolumePath.c_str());
	HKEY bcdKey;
	status = RegLoadKeyW(HKEY_LOCAL_MACHINE, L"p2k_bcd", commandBuffer);
	if (status) 
	{ 
		int res = HRESULT_FROM_WIN32(status);
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to load BCD into registry. %s (0x%08X)", res, res);
		return res;
	}
	status = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"p2k_bcd", &bcdKey);
	if (status)
	{
		int res = HRESULT_FROM_WIN32(status);
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to open BCD registry. %s (0x%08X)", res, res);
		return res;
	}
	status = RegDeleteKeyValueW(bcdKey, L"Description", L"FirmwareModified");
	if (status)
	{
		// TODO: this one might be redundant
		int res = HRESULT_FROM_WIN32(status);
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to delete BCD FirmwareModified value. %s (0x%08X)", res, res);
		return res;
	}
	DWORD value = 1;
	status = RegSetKeyValueW(bcdKey, L"Description", L"System", REG_DWORD, &value, sizeof(DWORD));
	if (status)
	{
		int res = HRESULT_FROM_WIN32(status);
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD System value. %s (0x%08X)", res, res);
		return res;
	}
	status = RegSetKeyValueW(bcdKey, L"Description", L"TreatAsSystem", REG_DWORD, &value, sizeof(DWORD));
	if (status)
	{
		int res = HRESULT_FROM_WIN32(status);
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set BCD TreatAsSystem value. %s (0x%08X)", res, res);
		return res;
	}
	status = RegFlushKey(bcdKey); 
	if (status)
	{
		int res = HRESULT_FROM_WIN32(status);
		wlogerr(installLog, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Engine/Install thread] Failed to set flush BCD registry changes to disk. %s (0x%08X)", res, res);
		return res;
	}

	// Close everything and restore process elevation
	// If this fails, there is no problem.
	status = RegCloseKey(bcdKey);
	status = RegUnLoadKeyW(HKEY_LOCAL_MACHINE, L"p2k_bcd");
	SetPrivilege(tokenHandle, SE_RESTORE_NAME, FALSE);
	SetPrivilege(tokenHandle, SE_BACKUP_NAME, FALSE);
	CloseHandle(tokenHandle);

	/*
	* Configure boot sector for Legacy
	*/
	if (bUseLegacy)
	{	
		// For MBR: open disk
		wchar_t buffer[MAX_PATH];
		swprintf_s(buffer, L"\\\\.\\PhysicalDrive%d", diskNumber);
		HANDLE hDisk = CreateFileW(buffer, FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, 0);
		if (hDisk == INVALID_HANDLE_VALUE)
		{
			// Throw error
			DebugBreak();
		}

		// 1. Read MBR
		unsigned char sectorBuffer[512];
		DWORD bytesRead;
		if (!ReadFile(hDisk, sectorBuffer, 512, &bytesRead, NULL)
			|| bytesRead != 512)
		{
			DebugBreak();
		}

		// 2. Copy partition table into template MBR
		unsigned char templateMBR[512];
		if (memcpy_s(templateMBR, 512, masterBootRecord, 512) // Get template MBR
			|| memcpy_s(templateMBR + 440, 70, sectorBuffer + 440, 70)) // Copy signature + partition table
		{
			DebugBreak();
		}

		// 3. Copy back MBR
		if (SetFilePointer(hDisk, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			DebugBreak();
		}
		if (!WriteFile(hDisk, templateMBR, 512, &bytesRead, NULL)
			|| bytesRead != 512)
		{
			DebugBreak();
		}

		CloseHandle(hDisk);

		// For NTFS:
		// 1. Read first partition sector
		// 2. Verify it is valid NTFS data
		// 3. Copy boot code into NTFS data
		// 4. Calculate the header checksum
		// 5. Copy back NTFS header
		// Without explicitly creating an NTFS boot sector it works fine in testing
		// I think windows might pre-format NTFS drives with a NT60 boot sector
	}

	SetLastError(ERROR_SUCCESS);
	return ERROR_SUCCESS;
}