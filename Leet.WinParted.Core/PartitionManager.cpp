#include "PartitionManager.h"

#include <vector>
#include <Windows.h>
#include "Win32Handle.h"

extern wchar_t* CleanString(const wchar_t* string);

bool GetPhysicalDriveInfo(const wchar_t* dosName, Leet::WinParted::WP_DISK_INFO& diskInfo, Leet::Panther2K::Util::Logger* logger)
{
	diskInfo.DiskNumber = wcstol(dosName + 13, nullptr, 10);
	swprintf(diskInfo.DiskPath, 64, L"\\\\.\\PHYSICALDRIVE%d", diskInfo.DiskNumber);
	Win32Handle diskFileHandle(CreateFileW(diskInfo.DiskPath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
	if (diskFileHandle == INVALID_HANDLE_VALUE)
		return false;
	
	DWORD byteCount = 0;
	STORAGE_PROPERTY_QUERY spq = { };
	spq.PropertyId = StorageDeviceProperty;
	spq.QueryType = PropertyStandardQuery;

	STORAGE_DEVICE_DESCRIPTOR sdd = { 0 };
	if (!DeviceIoControl(diskFileHandle, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), &sdd, sizeof(sdd), &byteCount, nullptr)
		&& GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
		return false;

	std::vector<char> sddBuffer(sdd.Size);
	STORAGE_DEVICE_DESCRIPTOR* psdd = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(sddBuffer.data());
	if (!DeviceIoControl(diskFileHandle, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), psdd, sdd.Size, &byteCount, nullptr))
		return false;

	size_t bytesWritten = 0;
	wchar_t nameBuffer[256];

	if (psdd->VendorIdOffset > 0)
	{
		char* vendorStr = reinterpret_cast<char*>(psdd) + psdd->VendorIdOffset;
		mbstowcs_s(&bytesWritten, nameBuffer, 256, vendorStr, _TRUNCATE);
		if (bytesWritten > 0) 
			nameBuffer[bytesWritten - 1] = L' ';
	}

	if (psdd->ProductIdOffset > 0) 
	{
		char* productStr = reinterpret_cast<char*>(psdd) + psdd->ProductIdOffset;
		mbstowcs_s(&bytesWritten, nameBuffer + bytesWritten, 256 - bytesWritten, productStr, _TRUNCATE);
	}
	
	wchar_t* temp = CleanString(nameBuffer);
	if (temp) 
	{
		lstrcpyW(diskInfo.DeviceName, temp);
		free(temp);
	}

	DISK_GEOMETRY dg = { };
	if (!DeviceIoControl(diskFileHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg), &byteCount, nullptr))
		return false;

	diskInfo.SectorSize = dg.BytesPerSector;
	diskInfo.SectorCount = dg.Cylinders.QuadPart * dg.TracksPerCylinder * dg.SectorsPerTrack;
	diskInfo.MediaType = dg.MediaType;

	int partitionCount = 1;
	size_t dliSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * (partitionCount - 1);
	std::vector<char> layoutBuffer;
	DRIVE_LAYOUT_INFORMATION_EX* dli = nullptr;

	while (true) 
	{
		layoutBuffer.resize(dliSize);
		dli = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX*>(layoutBuffer.data());

		if (DeviceIoControl(diskFileHandle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, nullptr, 0, dli, dliSize, &byteCount, nullptr))
			break;

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
			return false;

		partitionCount++;
		dliSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * (partitionCount - 1);
	}

	diskInfo.PartitionCount = dli->PartitionCount;
}

void Leet::WinParted::PartitionManager::Refresh()
{
	m_openHandles.clear();
	m_diskInfos.clear();

	unsigned int bufferSize = 8192;
	wchar_t* dosdevs = static_cast<wchar_t*>(safeMalloc(logger, sizeof(wchar_t*) * bufferSize));
	DWORD result = QueryDosDeviceW(nullptr, dosdevs, bufferSize);
	while (result == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		bufferSize *= 2;
		safeFree(logger, dosdevs);
		dosdevs = static_cast<wchar_t*>(safeMalloc(logger, sizeof(wchar_t*) * bufferSize));
		result = QueryDosDeviceW(nullptr, dosdevs, bufferSize);
	}

	for (const wchar_t* pos = dosdevs; *pos; pos += lstrlenW(pos) + 1)
	{
		if (wcsncmp(pos, L"PhysicalDrive", 13) != 0)
			continue;

		WP_DISK_INFO diskInfo = {};

		if (GetPhysicalDriveInfo(pos, diskInfo, logger))
			m_diskInfos.push_back(diskInfo);
	}
}