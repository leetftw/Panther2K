#pragma once

#include <Windows.h>
#include <vds.h>

// Generic VDS routines
HRESULT VdsStartSession(IVdsService** session);
HRESULT VdsFindDisk(IVdsService* pVdsService, int diskNumber, IVdsDisk** pVdsDisk);
HRESULT VdsIsPartitionOEM(IVdsDisk* pVdsDisk, unsigned long long partOffset, bool* isOEM);
HRESULT VdsGetVolumeOnDisk(IVdsService* pVdsService, IVdsDisk* pVdsDisk, unsigned long long partOffset, IUnknown** pVdsVolume);

// WinParted specific
HRESULT FormatPartition(int diskNumber, unsigned long long partOffset, const wchar_t* fileSystem, const wchar_t* volumeName = L"");
HRESULT SetPartitionAccessPoint(int diskNumber, unsigned long long partOffset, const wchar_t* mountPoint, bool unmountPrevious = true);
HRESULT ShrinkPartition(int diskNumber, unsigned long long partOffset, unsigned long long sizeToShrink);
HRESULT QueryPartitionSupportedFilesystems(int diskNumber, unsigned long long partOffset, wchar_t*** queryResult, int* queryCount);
