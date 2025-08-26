#include "PartitionManager.h"
#include <Shlwapi.h>
#include <shlobj_core.h>

#include <vector>

#include "VdsService.h"

using namespace Leet::WinParted;

// TODO: Make all exports return HRESULT codes

extern "C" int _stdcall PartedEntryPoint()
{
	int result = PartitionManager::RunWinParted(NULL);
	safeCleanup(PartitionManager::GetLogger());
	return result;
};

extern "C" int _stdcall RunWinParted(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger)
{
	PartitionManager::SetLogger(logger);
	int result = PartitionManager::RunWinParted(console);
	return result;
};

extern "C" HRESULT _stdcall EnumerateDisks(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, DISK_INFORMATION** disks, int* diskCount)
{
	PartitionManager::ShowNoInfoDialogs = true;
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);

	PartitionManager::CurrentPage = new Page();
	PartitionManager::CurrentPage->Initialize(console);
	PartitionManager::CurrentPage->Update();

	PartitionManager::PopulateDiskInformation();
	size_t tableSize = PartitionManager::DiskInformationTableSize * sizeof(DISK_INFORMATION);
	*disks = static_cast<DISK_INFORMATION*>(LocalAlloc(LPTR, tableSize));
	memcpy_s(*disks, tableSize, PartitionManager::DiskInformationTable, tableSize);
	*diskCount = PartitionManager::DiskInformationTableSize;
	return S_OK;
}

// TODO: Cleanup this mess
// TODO: Merge GPT and MBR into a single function with a flag
// TODO: Move partition layout information to Panther2K
extern "C" HRESULT _stdcall ApplyP2KLayoutToDiskGPT(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, bool letters, wchar_t*** mountPath, wchar_t*** volumeList)
{
	HRESULT ret;
	HRESULT hResult;
	int volIndex = 0;
	int totalPartitions = 0;
	long structSize = 0;
	WP_PART_LAYOUT* layout = nullptr;

	PartitionManager::ShowNoInfoDialogs = true;
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);

	// Show loading screen
	PartitionManager::CurrentPage = new Page();
	PartitionManager::CurrentPage->Initialize(console);
	PartitionManager::CurrentPage->Update();

	if (PartitionManager::ShowMessagePage(L"Warning: All data on the drive will be lost and a new partition table will be written. Would you like to continue?", MessagePageType::YesNo, MessagePageUI::Warning) != MessagePageResult::Yes)
	{
		ret = ERROR_CANCELLED;
		goto exit;
	}
	
	PartitionManager::PopulateDiskInformation();
	PartitionManager::CurrentDisk.DiskNumber = -1;
	for (int i = 0; i < PartitionManager::DiskInformationTableSize; i++)
		if (PartitionManager::DiskInformationTable[i].DiskNumber == diskNumber)
			PartitionManager::CurrentDisk = PartitionManager::DiskInformationTable[diskNumber];
	if (PartitionManager::CurrentDisk.DiskNumber == -1)
	{
		ret = ERROR_FILE_NOT_FOUND;
		goto exit;
	}

	if (mountPath)
	{
		wchar_t rootCwdPath[MAX_PATH];
		if (_wgetcwd(rootCwdPath, MAX_PATH) == NULL)
		{
			ret = ERROR_PATH_NOT_FOUND;
			goto exit;
		}

		PathStripToRootW(rootCwdPath);

		for (int i = 0; i < 3; i++)
		{
			wcscpy_s((*mountPath)[i], MAX_PATH, rootCwdPath);
		}

		if (letters)
		{
			int mountIndex = 0;
			DWORD drives = GetLogicalDrives();
			const wchar_t* letters = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			for (int i = 25; i >= 0 && mountIndex < 3; i--)
			{
				if (drives ^ 0b10000000000000000000000000
					&& letters[i] != L'X'
					&& letters[i] != L'A'
					&& letters[i] != L'B')
				{
					(*mountPath)[mountIndex][0] = letters[i];
					(*mountPath)[mountIndex++][1] = L'\0';
				}
				drives <<= 1;
			}

			if (mountIndex != 3)
			{
				ret = ERROR_BUSY;
				goto exit;
			}
		}
		else
		{
			wcscat_s((*mountPath)[0], MAX_PATH, L"$Panther2K\\Sys\\");
			wcscat_s((*mountPath)[1], MAX_PATH, L"$Panther2K\\Win\\");
			wcscat_s((*mountPath)[2], MAX_PATH, L"$Panther2K\\Rec\\");

			for (int i = 0; i < 3; i++)
			{
				hResult = SHCreateDirectoryExW(NULL, (*mountPath)[i], NULL);
				if (hResult != ERROR_SUCCESS && hResult != ERROR_ALREADY_EXISTS)
				{
					ret = hResult;
					goto exit;
				}
			}
		}
	}

	// TODO: Move this to Panther2K
	totalPartitions = 3;
	structSize = (sizeof(WP_PART_LAYOUT) + ((totalPartitions - 1) * sizeof(WP_PART_DESCRIPTION)));
	layout = (WP_PART_LAYOUT*)safeMalloc(logger, structSize);
	ZeroMemory(layout, structSize);
	layout->PartitionCount = totalPartitions;

	layout->Partitions[0].PartitionNumber = 1;
	layout->Partitions[0].PartitionType = 0xEF00;
	if (PartitionManager::CurrentDisk.SectorSize == 4096)
		wcscpy_s(layout->Partitions[0].PartitionSize, L"300M");
	else
		wcscpy_s(layout->Partitions[0].PartitionSize, L"150M");
	wcscpy_s(layout->Partitions[0].FileSystem, L"FAT32");
	if (mountPath) layout->Partitions[0].MountPoint = (*mountPath)[0];

	layout->Partitions[1].PartitionNumber = 2;
	layout->Partitions[1].PartitionType = 0x0C01;
	wcscpy_s(layout->Partitions[1].PartitionSize, L"16M");
	wcscpy_s(layout->Partitions[1].FileSystem, L"RAW");

	layout->Partitions[2].PartitionNumber = 3;
	layout->Partitions[2].PartitionType = 0x0700;
	wcscpy_s(layout->Partitions[2].PartitionSize, L"100%");
	wcscpy_s(layout->Partitions[2].FileSystem, L"NTFS");
	if (mountPath) layout->Partitions[2].MountPoint = (*mountPath)[1];

	ret = PartitionManager::ApplyPartitionLayoutGPT(layout);
	for (int i = 0; i < 3; i++)
	{
		if (i == 1) continue;
		PartitionManager::LoadPartition(&PartitionManager::CurrentDiskPartitions[i]);
		lstrcpyW((*volumeList)[volIndex++], PartitionManager::CurrentPartition.VolumeInformation.VolumeFile);
	}
	free(layout);
exit:
	PartitionManager::ShowNoInfoDialogs = false;
	return ret;
}

extern "C" HRESULT _stdcall ApplyP2KLayoutToDiskMBR(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, bool letters, wchar_t*** mountPath, wchar_t*** volumeList)
{
	HRESULT ret;
	HRESULT hResult;
	int volIndex = 0;
	int totalPartitions = 0;
	long structSize = 0;
	WP_PART_LAYOUT* layout = nullptr;

	PartitionManager::ShowNoInfoDialogs = true;
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);

	// Show loading screen
	PartitionManager::CurrentPage = new Page();
	PartitionManager::CurrentPage->Initialize(console);
	PartitionManager::CurrentPage->Update();

	if (PartitionManager::ShowMessagePage(L"Warning: All data on the drive will be lost and a new partition table will be written. Would you like to continue?", MessagePageType::YesNo, MessagePageUI::Warning) != MessagePageResult::Yes)
	{
		ret = ERROR_CANCELLED;
		goto exit;
	}

	PartitionManager::PopulateDiskInformation();
	PartitionManager::CurrentDisk.DiskNumber = -1;
	for (int i = 0; i < PartitionManager::DiskInformationTableSize; i++)
		if (PartitionManager::DiskInformationTable[i].DiskNumber == diskNumber)
			PartitionManager::CurrentDisk = PartitionManager::DiskInformationTable[diskNumber];
	if (PartitionManager::CurrentDisk.DiskNumber == -1)
	{
		ret = ERROR_FILE_NOT_FOUND;
		goto exit;
	}

	if (mountPath)
	{
		wchar_t rootCwdPath[MAX_PATH];
		if (_wgetcwd(rootCwdPath, MAX_PATH) == NULL)
		{
			ret = ERROR_PATH_NOT_FOUND;
			goto exit;
		}

		PathStripToRootW(rootCwdPath);

		for (int i = 0; i < 3; i++)
		{
			wcscpy_s((*mountPath)[i], MAX_PATH, rootCwdPath);
		}

		if (letters)
		{
			int mountIndex = 0;
			DWORD drives = GetLogicalDrives();
			const wchar_t* letters = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			for (int i = 25; i >= 0 && mountIndex < 3; i--)
			{
				if (drives ^ 0b10000000000000000000000000
					&& letters[i] != L'X'
					&& letters[i] != L'A'
					&& letters[i] != L'B')
				{
					(*mountPath)[mountIndex][0] = letters[i];
					(*mountPath)[mountIndex++][1] = L'\0';
				}
				drives <<= 1;
			}

			if (mountIndex != 3)
			{
				ret = ERROR_BUSY;
				goto exit;
			}
		}
		else
		{
			wcscat_s((*mountPath)[0], MAX_PATH, L"$Panther2K\\Sys\\");
			wcscat_s((*mountPath)[1], MAX_PATH, L"$Panther2K\\Win\\");
			wcscat_s((*mountPath)[2], MAX_PATH, L"$Panther2K\\Rec\\");

			for (int i = 0; i < 3; i++)
			{
				hResult = SHCreateDirectoryExW(NULL, (*mountPath)[i], NULL);
				if (hResult != ERROR_SUCCESS && hResult != ERROR_ALREADY_EXISTS)
				{
					ret = hResult;
					goto exit;
				}
			}
		}
	}

	// TODO: Move this to Panther2K
	totalPartitions = 3;
	structSize = (sizeof(WP_PART_LAYOUT) + ((totalPartitions - 1) * sizeof(WP_PART_DESCRIPTION)));
	layout = (WP_PART_LAYOUT*)safeMalloc(logger, structSize);
	ZeroMemory(layout, structSize);
	layout->PartitionCount = totalPartitions;

	layout->Partitions[0].PartitionNumber = 1;
	layout->Partitions[0].PartitionType = 0x0780;
	if (PartitionManager::CurrentDisk.SectorSize == 4096)
		wcscpy_s(layout->Partitions[0].PartitionSize, L"500M");
	else
		wcscpy_s(layout->Partitions[0].PartitionSize, L"150M");
	wcscpy_s(layout->Partitions[0].FileSystem, L"NTFS");
	if (mountPath) layout->Partitions[0].MountPoint = (*mountPath)[0];

	layout->Partitions[1].PartitionNumber = 2;
	layout->Partitions[1].PartitionType = 0x0700;
	wcscpy_s(layout->Partitions[1].PartitionSize, L"100%");
	wcscpy_s(layout->Partitions[1].FileSystem, L"NTFS");
	if (mountPath) layout->Partitions[1].MountPoint = (*mountPath)[1];

	ret = PartitionManager::ApplyPartitionLayoutMBR(layout);
	volIndex = 0;
	for (int i = 0; i < 2; i++)
	{
		PartitionManager::LoadPartition(&PartitionManager::CurrentDiskPartitions[i]);
		lstrcpyW((*volumeList)[volIndex++], PartitionManager::CurrentPartition.VolumeInformation.VolumeFile);
	}
	free(layout);

exit:
	PartitionManager::ShowNoInfoDialogs = false;
	return ret;
}

bool LoadPartitionFromOffset(int diskNumber, unsigned long long partOffset)
{
	PartitionManager::PopulateDiskInformation();
	if (!PartitionManager::LoadDisk(&PartitionManager::DiskInformationTable[diskNumber], false)) return false;
	
	for (int i = 0; i < PartitionManager::CurrentDiskPartitionCount; i++)
	{
		if (PartitionManager::CurrentDiskPartitions[i].StartLBA.ULL * PartitionManager::CurrentDisk.SectorSize == partOffset)
		{
			if (!PartitionManager::LoadPartition(&PartitionManager::CurrentDiskPartitions[i]))
				return false;
			return true;
		}
	}
	return false;
}

extern "C" HRESULT _stdcall SetPartType(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, unsigned long long partOffset, short partType)
{
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);
	PartitionManager::ShowNoInfoDialogs = true;
	HRESULT returnValue = ERROR_INVALID_HANDLE;

	PartitionManager::CurrentPage = new Page();
	PartitionManager::CurrentPage->Initialize(console);
	PartitionManager::CurrentPage->Update();

	if (!LoadPartitionFromOffset(diskNumber, partOffset)) goto retFalse;
	if (!PartitionManager::SetCurrentPartitionType(partType)) goto retFalse;
	if (!PartitionManager::SavePartitionTableToDisk()) goto retFalse;
	returnValue = ERROR_SUCCESS;

retFalse:
	PartitionManager::ShowNoInfoDialogs = false;
	return returnValue;
}

extern "C" HRESULT _stdcall MountPartition(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, unsigned long long partOffset, const wchar_t* mountPoint)
{
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);
	PartitionManager::ShowNoInfoDialogs = true;
	HRESULT result = ERROR_BAD_UNIT;

	if (!LoadPartitionFromOffset(diskNumber, partOffset)) goto retFalse;
	result = ::SetPartitionAccessPoint(PartitionManager::CurrentPartition.DiskNumber, PartitionManager::CurrentPartition.StartLBA.ULL * PartitionManager::CurrentDisk.SectorSize, mountPoint);

retFalse:
	PartitionManager::ShowNoInfoDialogs = false;
	return result;
}

// DUPLICATE CODE: SetupEngine_Boot.cpp
HRESULT GetVolumeDiskExtents(const wchar_t* volumePath, VOLUME_DISK_EXTENTS** diskExtents)
{
	HANDLE hVolume = CreateFileW(volumePath, FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		HRESULT res = HRESULT_FROM_WIN32(GetLastError());
		return res;
	}

	size_t size = sizeof(VOLUME_DISK_EXTENTS);
	VOLUME_DISK_EXTENTS* extents = static_cast<VOLUME_DISK_EXTENTS*>(safeMalloc(nullptr, size));
	//if (!extents) return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_OUTOFMEMORY);
	BOOL ioResult = DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, extents, size, NULL, NULL);
	while (!ioResult && GetLastError() == ERROR_MORE_DATA)
	{
		size = (extents->NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT) + sizeof(VOLUME_DISK_EXTENTS);
		extents = static_cast<VOLUME_DISK_EXTENTS*>(realloc(extents, size));
		if (!extents) return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

		ioResult = DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, extents, size, NULL, NULL);
	}

	HRESULT hResult = S_OK;
	if (!ioResult) hResult = HRESULT_FROM_WIN32(GetLastError());
	else *diskExtents = extents;

	CloseHandle(hVolume);
	return hResult;
}

class VdsPartitionWaitSink : public IVdsAdviseSink 
{
private:
	IVdsService* _pService;
	long _refCount;
	int _diskNumber;
	VDS_OBJECT_ID _diskId;
	unsigned long long _partOffset;
	boolean _foundPart = false;
public:
	VdsPartitionWaitSink(IVdsService* service, int diskNumber, unsigned long long offset) : _refCount(1), _pService(service), _diskNumber(diskNumber), _partOffset(offset)
	{
		_pService->AddRef(); 

		IVdsDisk* pVdsDisk;
		HRESULT hResult = VdsFindDisk(service, diskNumber, &pVdsDisk);
		if (FAILED(hResult)) DebugBreak();
		VDS_DISK_PROP properties;
		hResult = pVdsDisk->GetProperties(&properties);
		if (FAILED(hResult)) DebugBreak();
		_diskId = properties.id;
	}

	~VdsPartitionWaitSink()
	{
		_pService->Release();
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override 
	{
		if (riid == IID_IUnknown || riid == IID_IVdsAdviseSink) 
		{
			*ppvObject = static_cast<IVdsAdviseSink*>(this);
			AddRef();
			return S_OK;
		}
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return InterlockedIncrement(&_refCount);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		LONG count = InterlockedDecrement(&_refCount);
		if (count == 0) {
			delete this;
		}
		return count;
	}

	HRESULT STDMETHODCALLTYPE OnNotify(LONG numberOfNotifications, VDS_NOTIFICATION* pNotificationArray) override 
	{
		for (LONG i = 0; i < numberOfNotifications; ++i) 
		{
			VDS_NOTIFICATION& notification = pNotificationArray[i];
			switch (notification.objectType)
			{
			case VDS_NTT_PARTITION:
				if (notification.Partition.ulEvent == VDS_NF_PARTITION_ARRIVE)
				{
					if (_diskId == notification.Partition.diskId
						&& _partOffset == notification.Partition.ullOffset)
						_foundPart = true;
				}
				break;
			default:
				//DebugBreak();
				break;
			}
		}
		return S_OK;
	}

	bool HasPartitionArrived()
	{
		return _foundPart;
	}
};

extern "C" HRESULT _stdcall PrepareDiskForWindows(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, const wchar_t* volumeGuid, bool useLegacy,
	unsigned long long requiredBootSize, unsigned long long requiredRESize, wchar_t installVolumes[2][128])
{
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);
	PartitionManager::ShowNoInfoDialogs = true;

	PartitionManager::CurrentPage = new Page();
	PartitionManager::CurrentPage->SetStatusText(L"The partition manager is preparing the disk for installation...");
	PartitionManager::CurrentPage->Initialize(console);
	PartitionManager::CurrentPage->Update();

	wlogc(logger, PANTHER_LL_DETAILED, L"[WinPartedDll] Preparing disk for Windows...");
	wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll] The GUID of the volume is %s", volumeGuid);
	wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] Determining on which disk the volume resides...");

	wchar_t volumeBuffer[128];
	wcscpy_s(volumeBuffer, volumeGuid);
	volumeBuffer[lstrlenW(volumeBuffer) - 1] = 0;

	VOLUME_DISK_EXTENTS* extents;
	HRESULT result = GetVolumeDiskExtents(volumeBuffer, &extents);
	if (FAILED(result)) return result;

	// Volume must be basic
	// Also get the partition corresponding to the volume
	if (extents->NumberOfDiskExtents != 1)
	{
		// dynamic not supported
		wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Cannot determine the disk belonging to a logical volume. Installing to dynamic disks is unsupported.");
		DebugBreak();
	}

	DISK_EXTENT extent = extents->Extents[0];
	free(extents);

	// Load disk into WinParted
	PartitionManager::PopulateDiskInformation();
	int diskNumber = -1;
	for (int i = 0; i < PartitionManager::DiskInformationTableSize; i++)
	{
		if (PartitionManager::DiskInformationTable[i].DiskNumber == extent.DiskNumber)
		{
			wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll] Preparing disk %u for Windows...", extent.DiskNumber);
			if (!PartitionManager::LoadDisk(&PartitionManager::DiskInformationTable[i], false))
			{
				wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to load disk information.");
				DebugBreak();
			}

			diskNumber = PartitionManager::CurrentDisk.DiskNumber;
			break;
		}
	}
	if (diskNumber == -1)
	{
		wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[WinPartedDll] Disk %u was not found.", extent.DiskNumber);
		DebugBreak();
	}

	// Verify boot mode
	if ((PartitionManager::CurrentDiskOperatingMode == OperatingMode::GPT && useLegacy)
		|| (PartitionManager::CurrentDiskOperatingMode == OperatingMode::MBR && !useLegacy))
	{
		wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[WinPartedDll] Partition table type of disk %u does not match selected boot type.", extent.DiskNumber);
		DebugBreak();
	}

	// Try to find boot partition
	wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] Looking for boot partition on disk...");
	bool foundBoot = false; unsigned long long bootOffset = 0;
	for (int i = 0; i < PartitionManager::CurrentDiskPartitionCount; i++)
	{
		if (!useLegacy)
		{
			if (!memcmp(PartitionManager::GetGUIDFromPartitionTypeCode(0xEF00), &PartitionManager::CurrentDiskPartitions[i].Type.TypeGUID, sizeof(GUID)))
			{
				bootOffset = PartitionManager::CurrentDiskPartitions[i].StartLBA.ULL;
				foundBoot = true;
				break;
			}
		}
		else
		{
			// TODO: write this
			// 0x80 attrib + NTFS + bootmgr exists
		}
	}
	if (!foundBoot && !useLegacy)
	{
		// CANNOT CREATE ESP
		wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] There is no EFI System Partition on the disk. The entire disk must be formatted to install Windows.");
		DebugBreak();
	}
	else if (!foundBoot)
	{
		if (PartitionManager::CurrentDiskPartitionCount == 4)
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] No boot partition exists on MBR disk and there are no partition slots available. The entire disk must be formatted to install Windows.");
			DebugBreak();
		}

		// Calculate the new system partition size after shrinking
		wlogc(logger, PANTHER_LL_DETAILED, L"[WinPartedDll] No boot partition exists on MBR disk, creating boot partition...");
		unsigned long long targetSysSpace = extent.ExtentLength.QuadPart - requiredBootSize;

		// Shrink system
		wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] Shrinking system volume...");
		HRESULT hResult = ShrinkPartition(extent.DiskNumber, extent.StartingOffset.QuadPart, extent.ExtentLength.QuadPart - targetSysSpace);
		if (FAILED(hResult))
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to shrink system volume, VDS returned an error (0x%08X)", hResult);
			return hResult;
		}

		// Create boot
		wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] Creating boot partition...");
		PartitionInformation info;
		info.StartLBA.ULL = extent.StartingOffset.QuadPart / PartitionManager::CurrentDisk.SectorSize;
		info.EndLBA.ULL = targetSysSpace / PartitionManager::CurrentDisk.SectorSize + info.StartLBA.ULL;

		// Index 1 is NTFS
		info.Type.SystemID = PartitionManager::GptTypes[1].gDiskType;
		info.Type.TypeGUID = PartitionManager::GptTypes[1].guid;
		wcscpy_s(info.Name, PartitionManager::GptTypes[1].display_name);

		// 0x80 sets the active flag
		if (!PartitionManager::AddPartition(&info, 0x80))
		{
			// ERRORR
			DebugBreak();
		}

		wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] Waiting for IVdsDisk to update...");
		IVdsService* session;
		hResult = VdsStartSession(&session);
		if (FAILED(hResult)) DebugBreak();
		VdsPartitionWaitSink* sink = new VdsPartitionWaitSink(session, diskNumber, info.StartLBA.ULL * PartitionManager::CurrentDisk.SectorSize);

		DWORD cookie;
		hResult = session->Advise(sink, &cookie);
		if (FAILED(hResult)) DebugBreak();

		wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] Saving partition table with new boot partition to disk...");
		if (!PartitionManager::SavePartitionTableToDisk())
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to save new partition table.");
			DebugBreak();
		}

		const int maxIter = 100;
		for (int i = 0; i < maxIter; i++)
		{
			if (i % 10 == 0) wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll] Waiting for IVdsDisk to update (%d/%d)...", i / 10, maxIter / 10);
			session->Refresh();
			if (sink->HasPartitionArrived())
				break;
			Sleep(100);
		}

		bool arrived = sink->HasPartitionArrived();
		delete sink;
		if (!arrived)
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Timed out waiting for VDS.");
			DebugBreak();
		}

		wlogc(logger, PANTHER_LL_VERBOSE, L"[WinPartedDll] VDS partition has arrived.");
		wlogc(logger, PANTHER_LL_DETAILED, L"[WinPartedDll] Reloading disk information...");
		PartitionManager::PopulateDiskInformation();
		int diskNumber = -1;
		for (int i = 0; i < PartitionManager::DiskInformationTableSize; i++)
		{
			if (PartitionManager::DiskInformationTable[i].DiskNumber == extent.DiskNumber)
			{
				if (!PartitionManager::LoadDisk(&PartitionManager::DiskInformationTable[i], false))
				{
					wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to load disk information.");
					DebugBreak();
				}
			}
		}

		// Format the boot partition
		wlogc(logger, PANTHER_LL_DETAILED, L"[WinPartedDll] Formatting boot partition...");
		int partIndex = -1;
		for (int i = 0; i < PartitionManager::CurrentDiskPartitionCount; i++)
		{
			if (PartitionManager::CurrentDiskPartitions[i].StartLBA.ULL == info.StartLBA.ULL)
			{
				partIndex = i;
				break;
			}
		}
		if (partIndex == -1)
		{
			wlogc(logger, PANTHER_LL_DETAILED, L"[WinPartedDll] Failed to locate created partition.");
			return hResult;
		}

		hResult = FormatPartition(extent.DiskNumber, PartitionManager::CurrentDiskPartitions[partIndex].StartLBA.ULL* PartitionManager::CurrentDisk.SectorSize, L"NTFS", L"WinBoot");
		if (FAILED(hResult))
		{
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[WinPartedDll] Failed to format new boot partition (0x%08X)", hResult);
			return hResult;
		}

		bootOffset = PartitionManager::CurrentDiskPartitions[partIndex].StartLBA.ULL;
	}

	// Find partitions and return GUIDs
	int systemIndex = -1;
	for (int i = 0; i < PartitionManager::CurrentDiskPartitionCount; i++)
		if (PartitionManager::CurrentDiskPartitions[i].StartLBA.ULL * PartitionManager::CurrentDisk.SectorSize == extent.StartingOffset.QuadPart)
			systemIndex = i;
	if (!PartitionManager::LoadPartition(&PartitionManager::CurrentDiskPartitions[systemIndex]))
	{
		DebugBreak();
	}
	wcscpy_s(installVolumes[0], PartitionManager::CurrentPartition.VolumeInformation.VolumeFile);

	int bootIndex = -1;
	for (int i = 0; i < PartitionManager::CurrentDiskPartitionCount; i++)
		if (PartitionManager::CurrentDiskPartitions[i].StartLBA.ULL == bootOffset)
			bootIndex = i;
	if (!PartitionManager::LoadPartition(&PartitionManager::CurrentDiskPartitions[bootIndex]))
	{
		DebugBreak();
	}
	wcscpy_s(installVolumes[1], PartitionManager::CurrentPartition.VolumeInformation.VolumeFile);

	return S_OK;
}

extern "C" HRESULT _stdcall EnumVolumes(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, VolumeInformation** volumes, bool includeDynamic, int* count)
{
	PartitionManager::SetConsole(console);
	PartitionManager::SetLogger(logger);
	PartitionManager::ShowNoInfoDialogs = true;
	HRESULT hResult = S_OK;

	PartitionManager::CurrentPage = new Page();
	PartitionManager::CurrentPage->SetStatusText(L"The partition manager is enumerating available volumes...");
	PartitionManager::CurrentPage->Initialize(console);
	PartitionManager::CurrentPage->Update();
	wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Enumerating volumes...");

	wchar_t volumePath[MAX_PATH];
	HANDLE searchHandle = FindFirstVolumeW(volumePath, MAX_PATH);

	ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	std::vector<VolumeInformation> volumeVec;
	VolumeInformation volumeInfo;
	DWORD charsRead;
	
	while (searchHandle)
	{
		wlogf(logger, PANTHER_LL_DETAILED, MAX_PATH, L"[WinPartedDll]  |-> Found volume: %s", volumePath);

		if (!GetVolumeInformationW(volumePath, volumeInfo.VolumeName, sizeof(VolumeInformation::VolumeName) / sizeof(wchar_t), NULL, NULL, NULL, volumeInfo.FileSystem, sizeof(VolumeInformation::FileSystem) / sizeof(wchar_t)))
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to retrieve volume label and filesystem name!");
			searchHandle = NULL;
			continue;
		}
		
		wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll]  |   Label:        %s", volumeInfo.VolumeName);
		wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll]  |   File system:  %s", volumeInfo.FileSystem);

		if (!GetVolumePathNamesForVolumeNameW(volumePath, volumeInfo.MountPoint, sizeof(VolumeInformation::MountPoint) / sizeof(wchar_t), &charsRead)
			&& GetLastError() != ERROR_MORE_DATA)
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to retrieve volume mount point!");
			searchHandle = NULL;
			continue;
		}

		wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll]  |   Mount:        %s", volumeInfo.MountPoint);

		if (!GetDiskFreeSpaceExW(volumePath, NULL, reinterpret_cast<PULARGE_INTEGER>(&volumeInfo.TotalSize), reinterpret_cast<PULARGE_INTEGER>(&volumeInfo.SpaceFree)))
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to retrieve volume space information!");
			searchHandle = NULL;
			continue;
		}

		wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll]  |   Size:         %llu bytes", volumeInfo.TotalSize);
		wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH, L"[WinPartedDll]  |   Free space:   %llu bytes", volumeInfo.SpaceFree);

		wcscpy_s(volumeInfo.VolumeFile, volumePath);
		volumePath[lstrlenW(volumePath) - 1] = 0;

		VOLUME_DISK_EXTENTS* extents;
		HRESULT result = GetVolumeDiskExtents(volumePath, &extents);
		// TODO: if 'Incorrect function' do not show bc its probably DVD
		if (FAILED(result) && HRESULT_CODE(result) != ERROR_INVALID_FUNCTION)
		{
			wlogc(logger, PANTHER_LL_BASIC, L"[WinPartedDll] Failed to retrieve volume partition information!", result);
			SetLastError(HRESULT_CODE(result));
			searchHandle = NULL;
			continue;
		}

		// Optical media does not have extents
		if (extents)
		{
			if (extents->NumberOfDiskExtents == 1)
			{
				volumeInfo.DiskNumber = extents->Extents[0].DiskNumber;
				volumeInfo.PartitionNumber = 69;

				//HANDLE hVolume = CreateFileW(volumePath, FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
				//if (hVolume == INVALID_HANDLE_VALUE)
				//{
				//	HRESULT res = HRESULT_FROM_WIN32(GetLastError());
				//	return res;
				//}

				//PARTITION_INFORMATION_EX partInfo;
				//BOOL ioResult = DeviceIoControl(hVolume, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partInfo, sizeof(PARTITION_INFORMATION_EX), NULL, NULL);

				//// If DeviceIoControl fails here, retrieving this info can just be skipped
				//if (ioResult)
				//{
				//	
				//}

				//CloseHandle(hVolume);
			}
			else if (!includeDynamic)
			{
				safeFree(PartitionManager::GetLogger(), extents);
				continue;
			}
		}

		volumeVec.push_back(volumeInfo);

		if (!FindNextVolumeW(searchHandle, volumePath, MAX_PATH))
			searchHandle = NULL;
	}

	int lastError = GetLastError();
	if (lastError != 0 && lastError != ERROR_NO_MORE_FILES)
	{
		wloglerr(logger, PANTHER_LL_BASIC, MAX_PATH, L"[WinPartedDll] Failed to enumerate volumes: %s");
		return HRESULT_FROM_WIN32(lastError);
	}

	*volumes = (VolumeInformation*)safeMalloc(logger, sizeof(VolumeInformation) * volumeVec.size());
	for (int i = 0; i < volumeVec.size(); i++)
	{
		(*volumes)[i] = volumeVec[i];
	}
	*count = volumeVec.size();

	return S_OK;
}