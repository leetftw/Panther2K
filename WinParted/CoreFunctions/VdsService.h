#pragma once

#include <Windows.h>
#include <vds.h>

HRESULT VdsStartSession(IVdsService** session);
HRESULT VdsFindDisk(IVdsService* pVdsService, int diskNumber, IVdsDisk** pVdsDisk);
HRESULT FormatPartition(int diskNumber, unsigned long long partOffset, const wchar_t* fileSystem, const wchar_t* volumeName = L"");
HRESULT SetPartitionAccessPoint(int diskNumber, unsigned long long partOffset, const wchar_t* mountPoint, bool unmountPrevious = true);
HRESULT ShrinkPartition(int diskNumber, unsigned long long partOffset, unsigned long long sizeToShrink);