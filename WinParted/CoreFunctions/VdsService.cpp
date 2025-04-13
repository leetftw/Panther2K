#define INITGUID

#include "VdsService.h"

#define Assert(result, action) { if (FAILED(result)) { DebugBreak(); wlogf(PartitionManager::GetLogger(), PANTHER_LL_BASIC, MAX_PATH, L"Failed. (0x%08X)", result); action; }};

#define ObjectNameInformation (OBJECT_INFORMATION_CLASS)1
#define SafeRelease(x) {if (nullptr != x) { x->Release(); x = nullptr; } }
#define SafeCoFree(x) {if (nullptr != x) { CoTaskMemFree(x); x = nullptr; } }

#include <windows.h>
#include <winternl.h>
#include <vds.h>

#include "PartitionManager.h"

using namespace Leet::WinParted;

HRESULT VdsStartSession(IVdsService** session)
{
    HRESULT hResult;
    IVdsServiceLoader* pLoader = nullptr;
    IVdsService* pService = nullptr;

    wlogc(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, L"Starting VDS session...");

    // Initialize COM and IVdsLoader
    wlogc(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, L"Connecting to COM...");

    hResult = CoInitialize(nullptr);
    Assert(hResult, return hResult);
    hResult = CoCreateInstance(CLSID_VdsLoader, nullptr, CLSCTX_LOCAL_SERVER, IID_IVdsServiceLoader, (void**)&pLoader);
    Assert(hResult, goto releaseCOM);

    // Connect to IVdsService
    wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Connecting to VDS service...");
    hResult = pLoader->LoadService(nullptr, &pService);
    SafeRelease(pLoader);
    Assert(hResult, goto releaseCOM);
    hResult = pService->WaitForServiceReady();
    Assert(hResult, goto releaseCOM);

    // Refresh VDS data
    wlogc(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, L"Refreshing data...");
    hResult = pService->Reenumerate();
    Assert(hResult, goto releaseCOM);
    hResult = pService->Refresh();
    Assert(hResult, goto releaseCOM);

    *session = pService;
    return S_OK;

releaseCOM:
    SafeRelease(pService);
    SafeRelease(pLoader);
    return hResult;
}

HRESULT VdsFindDisk(IVdsService* pVdsService, int diskNumber, IVdsDisk** pVdsDisk)
{
    HRESULT hResult = S_OK;
    
    wchar_t requestedDisk[MAX_PATH];
    swprintf_s(requestedDisk, L"\\\\.\\GLOBALROOT\\Device\\Harddisk%d\\Partition0", diskNumber);
    HANDLE hFile = CreateFileW(requestedDisk, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, NULL, nullptr);
    Assert(GetLastError(), return GetLastError());

    char infoBuffer[512];
    DWORD bytesReceived;
    NTSTATUS res = NtQueryObject(hFile, ObjectNameInformation, &infoBuffer, 512, &bytesReceived);
    CloseHandle(hFile);
    Assert(res, return res);
    wcscpy_s(requestedDisk, ((UNICODE_STRING*)infoBuffer)->Buffer);

    wlogf(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, MAX_PATH, L"Finding disk in VDS namespace: %s.", requestedDisk);

    IEnumVdsObject* pEnumVdsSwProviders = nullptr;
    IEnumVdsObject* pEnumVdsPacks = nullptr;
    IEnumVdsObject* pEnumVdsDisks = nullptr;
    IUnknown* pUnknown = nullptr;

    IVdsSwProvider* pProvider = nullptr;
    IVdsPack* pPack = nullptr;
    IVdsDisk* pDisk = nullptr;
    ULONG ulFetchCount = 0;
    VDS_DISK_PROP diskProperties;

    // Query through all software providers
    wlogc(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, L"Querying software providers...");
    hResult = pVdsService->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS, &pEnumVdsSwProviders);
    Assert(hResult, goto releaseCOM);

    for (int swpIndex = 0; (hResult = pEnumVdsSwProviders->Next(1, &pUnknown, &ulFetchCount)) == S_OK; swpIndex++)
    {
        hResult = pUnknown->QueryInterface(&pProvider);
        SafeRelease(pUnknown);
        Assert(hResult, continue);
        wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"Querying packs for Software Provider #%d...", swpIndex);

        // Query through all packs
        hResult = pProvider->QueryPacks(&pEnumVdsPacks);
        SafeRelease(pProvider);
        Assert(hResult, continue);

        for (int packIndex = 0; (hResult = pEnumVdsPacks->Next(1, &pUnknown, &ulFetchCount)) == S_OK; packIndex++)
        {
            hResult = pUnknown->QueryInterface(&pPack);
            SafeRelease(pUnknown);
            Assert(hResult, continue);

            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"  Querying disks for Pack #%d...", packIndex);

            // Query through all disks
            hResult = pPack->QueryDisks(&pEnumVdsDisks);
            SafeRelease(pPack);
            Assert(hResult, continue);

            for (int diskIndex = 0; (hResult = pEnumVdsDisks->Next(1, &pUnknown, &ulFetchCount)) == S_OK; diskIndex++)
            {
                hResult = pUnknown->QueryInterface(&pDisk);
                SafeRelease(pUnknown);
                Assert(hResult, goto releaseCOM);

                wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"    Getting properties for Disk #%d...", diskIndex);

                // Determine if the disk contains the target
                hResult = pDisk->GetProperties(&diskProperties);
                Assert(hResult, goto releaseDisk);

                hFile = CreateFileW(diskProperties.pwszName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, NULL, nullptr);
                hResult = GetLastError();
                Assert(hResult, goto releaseDisk);
                res = NtQueryObject(hFile, ObjectNameInformation, &infoBuffer, 512, &bytesReceived);
                CloseHandle(hFile);
                Assert(res, goto releaseDisk);

                wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"    The path of the disk is %s.", ((UNICODE_STRING*)infoBuffer)->Buffer);

                if (lstrcmpiW(requestedDisk, ((UNICODE_STRING*)infoBuffer)->Buffer))
                {
                    SafeRelease(pDisk);
                    continue;
                }

                // From this point, no branch should continue iterating over disks
                wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Found the target disk.");
                *pVdsDisk = pDisk;
                goto releaseCOM;

            releaseDisk:
                wlogf(PartitionManager::GetLogger(), PANTHER_LL_BASIC, MAX_PATH, L"An error occurred while querying disk properties, skipping the disk. (0x%08X)", hResult);
                SafeRelease(pDisk);
            }
        }
    }

    // The disk was not found
    hResult = VDS_S_DISK_IS_MISSING;

releaseCOM:
    SafeRelease(pUnknown);
    SafeRelease(pEnumVdsSwProviders);
    SafeRelease(pEnumVdsPacks);
    SafeRelease(pEnumVdsDisks);
    return hResult;
}

HRESULT VdsIsPartitionOEM(IVdsDisk* pVdsDisk, unsigned long long partOffset, bool* isOEM)
{
    // Query through extents
    HRESULT hResult = S_OK;

    VDS_DISK_EXTENT* diskExtents = nullptr;
    LONG lFetchCount = 0;

    wlogf(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, MAX_PATH, L"Determining if volume at offset %llu is OEM...", partOffset);

    hResult = pVdsDisk->QueryExtents(&diskExtents, &lFetchCount);
    if (SUCCEEDED(hResult))
    {
        for (int i = 0; i < lFetchCount; i++)
        {
            // Test if extent matches the partition offset
            if (diskExtents[i].ullOffset != partOffset)
                continue;

            wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Found the target partition.");
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"         Extent #%d:", i);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Type: %d", diskExtents[i].type);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Offset: %llu", diskExtents[i].ullOffset);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Size: %llu", diskExtents[i].ullSize);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Volume GUID: {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
                diskExtents[i].volumeId.Data1, diskExtents[i].volumeId.Data2, diskExtents[i].volumeId.Data3,
                diskExtents[i].volumeId.Data4[0], diskExtents[i].volumeId.Data4[1], diskExtents[i].volumeId.Data4[2], diskExtents[i].volumeId.Data4[3],
                diskExtents[i].volumeId.Data4[4], diskExtents[i].volumeId.Data4[5], diskExtents[i].volumeId.Data4[6], diskExtents[i].volumeId.Data4[7]);

            const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
            *isOEM = !memcmp(&GUID_NULL, &diskExtents[i].volumeId, sizeof(GUID));

            break;
        }
    }

    SafeCoFree(diskExtents);
    return hResult;
}

HRESULT VdsGetVolumeOnDisk(IVdsService* pVdsService, IVdsDisk* pVdsDisk, unsigned long long partOffset, IUnknown** pVdsVolume)
{
    // Query through extents
    HRESULT hResult = S_OK;
    VDS_DISK_EXTENT* diskExtents = nullptr;
    LONG lFetchCount = 0;

    wlogf(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, MAX_PATH, L"Retrieving volume at offset %llu...", partOffset);
    hResult = pVdsDisk->QueryExtents(&diskExtents, &lFetchCount);
    if (SUCCEEDED(hResult))
    {
        for (int i = 0; i < lFetchCount; i++)
        {
            // Test if extent matches the partition offset
            if (diskExtents[i].ullOffset != partOffset)
                continue;

            wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Found the target partition.");
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"         Extent #%d:", i);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Type: %d", diskExtents[i].type);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Offset: %llu", diskExtents[i].ullOffset);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Size: %llu", diskExtents[i].ullSize);
            wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"            Volume GUID: {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
                diskExtents[i].volumeId.Data1, diskExtents[i].volumeId.Data2, diskExtents[i].volumeId.Data3,
                diskExtents[i].volumeId.Data4[0], diskExtents[i].volumeId.Data4[1], diskExtents[i].volumeId.Data4[2], diskExtents[i].volumeId.Data4[3],
                diskExtents[i].volumeId.Data4[4], diskExtents[i].volumeId.Data4[5], diskExtents[i].volumeId.Data4[6], diskExtents[i].volumeId.Data4[7]);

            const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
            bool isOEM = memcmp(&GUID_NULL, &diskExtents[i].volumeId, sizeof(GUID)) == 0;
            if (isOEM)
            {
                hResult = VDS_E_NOT_SUPPORTED;
                break;
            }

            // Get IVdsVolumeMF2 object
            wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Found the target volume.");
            hResult = pVdsService->GetObjectW(diskExtents[i].volumeId, VDS_OT_VOLUME, pVdsVolume);
            break;
        }
    }

    SafeCoFree(diskExtents);
    return hResult;
}

HRESULT FormatPartition(int diskNumber, unsigned long long partOffset, const wchar_t* fileSystem, const wchar_t* volumeName)
{
    wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"Starting format operation on %d@%llu", diskNumber, partOffset);

    HRESULT hResult, asyncRes;

    IVdsService* pVdsService = nullptr;
    IVdsDisk* pVdsDisk = nullptr;
    IVdsDiskPartitionMF* pVdsDiskMF = nullptr;
    IVdsVolumeMF2* pVdsVolume = nullptr;

    IUnknown* pUnknown = nullptr;
    IVdsAsync* pVdsAsync = nullptr;
    VDS_ASYNC_OUTPUT vdsAsyncOut;
    bool isOEM = false;

    hResult = VdsStartSession(&pVdsService);
    Assert(hResult, return hResult);

    hResult = VdsFindDisk(pVdsService, diskNumber, &pVdsDisk);
    Assert(hResult, goto releaseCOM);

    hResult = VdsIsPartitionOEM(pVdsDisk, partOffset, &isOEM); 
    Assert(hResult, goto releaseCOM);
    if (isOEM)
    {
        hResult = pVdsDisk->QueryInterface(&pVdsDiskMF);
        SafeRelease(pVdsDisk);
        Assert(hResult, goto releaseCOM);

        // Do DiskPartitionMF::Format
        wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Formatting the partition...");
        wchar_t fsNameBuffer[10];
        wcscpy_s(fsNameBuffer, fileSystem);
        wchar_t fsLabelBuffer[32];
        wcscpy_s(fsLabelBuffer, volumeName);
        pVdsDiskMF->FormatPartitionEx(partOffset, fsNameBuffer, 0, 0, fsLabelBuffer, true, true, false, &pVdsAsync);
    }
    else
    {
        // Get volume and do VolumeMF::FormatEx
        hResult = VdsGetVolumeOnDisk(pVdsService, pVdsDisk, partOffset, &pUnknown);
        Assert(hResult, goto releaseCOM);
        hResult = pUnknown->QueryInterface(&pVdsVolume);
        SafeRelease(pUnknown);
        Assert(hResult, goto releaseCOM);

        wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Formatting the volume...");
        wchar_t fsNameBuffer[10];
        wcscpy_s(fsNameBuffer, fileSystem);
        wchar_t fsLabelBuffer[32];
        wcscpy_s(fsLabelBuffer, volumeName);
        hResult = pVdsVolume->FormatEx(fsNameBuffer, 0, 0, fsLabelBuffer, true, true, false, &pVdsAsync);
    }
    
    Assert(hResult, goto releaseCOM);
    asyncRes = pVdsAsync->Wait(&hResult, &vdsAsyncOut);
    Assert(hResult, goto releaseCOM);
    Assert(asyncRes, hResult = asyncRes; goto releaseCOM);

releaseCOM:
    SafeRelease(pVdsAsync);
    SafeRelease(pVdsVolume);
    SafeRelease(pVdsDisk);
    SafeRelease(pVdsService);
    return hResult;
}

HRESULT SetPartitionAccessPoint(int diskNumber, unsigned long long partOffset, const wchar_t* mountPoint, bool unmountPrevious)
{
    wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"Starting mount operation on %d@%llu", diskNumber, partOffset);

    HRESULT hResult, asyncRes;

    IVdsService* pVdsService = nullptr;
    IVdsDisk* pVdsDisk = nullptr;
    IVdsAdvancedDisk* pVdsAdvDisk = nullptr;
    IVdsVolumeMF* pVdsVolumeMF = nullptr;

    IUnknown* pUnknown = nullptr;
    bool isOEM = false;

    wchar_t** accessPaths = nullptr;
    ULONG ulFetchCount = 0;

    hResult = VdsStartSession(&pVdsService);
    Assert(hResult, return hResult);

    hResult = VdsFindDisk(pVdsService, diskNumber, &pVdsDisk);
    Assert(hResult, goto releaseCOM);

    hResult = VdsIsPartitionOEM(pVdsDisk, partOffset, &isOEM);
    Assert(hResult, goto releaseCOM);
    if (isOEM)
    {
        wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Partition is an OEM partition, mounting partition...");
        if (mountPoint && lstrlenW(mountPoint) > 1)
        {
            wlogc(PartitionManager::GetLogger(), PANTHER_LL_BASIC, L"Cannot mount OEM partition to an NTFS path.");
            hResult = VDS_E_NOT_SUPPORTED; goto releaseCOM;
        }

        hResult = pVdsDisk->QueryInterface(&pVdsAdvDisk);
        SafeRelease(pVdsDisk);
        Assert(hResult, goto releaseCOM);

        // First check existing drive letter
        wchar_t letter = 0;
        hResult = pVdsAdvDisk->GetDriveLetter(partOffset, &letter);
        Assert(hResult, goto releaseCOM);
        if (letter >= 'A' && letter <= 'Z')
        {
            if (!unmountPrevious)
            {
                wlogf(PartitionManager::GetLogger(), PANTHER_LL_BASIC, MAX_PATH, L"The volume already has mount point %c, cannot assign a second letter.", letter);
                hResult = VDS_E_DRIVE_LETTER_NOT_FREE; goto releaseCOM;
            }

            // Unmount if it exists
            hResult = pVdsAdvDisk->DeleteDriveLetter(partOffset, letter);
            Assert(hResult, goto releaseCOM);
        }

        if (mountPoint && lstrlenW(mountPoint))
        {
            // Do AdvancedDisk::AssignDriveLetter
            hResult = pVdsAdvDisk->AssignDriveLetter(partOffset, *mountPoint);
            Assert(hResult, goto releaseCOM);
        }
    }
    else
    {
        wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Partition is a non-OEM partition, mounting volume...");

        // Get volume and do VolumeMF::FormatEx
        hResult = VdsGetVolumeOnDisk(pVdsService, pVdsDisk, partOffset, &pUnknown);
        Assert(hResult, goto releaseCOM);
        hResult = pUnknown->QueryInterface(&pVdsVolumeMF);
        SafeRelease(pUnknown);
        Assert(hResult, goto releaseCOM);

        if (unmountPrevious)
        {
            PartitionManager::GetLogger()->Write(PANTHER_LL_VERBOSE, L"Querying and unmounting existing volume mount points...");
            hResult = pVdsVolumeMF->QueryAccessPaths(&accessPaths, (PLONG)&ulFetchCount);
            Assert(hResult, goto releaseCOM);
            for (int i = 0; i < ulFetchCount; i++)
            {
                if (lstrlenW(accessPaths[i]) == 3)
                {
                    hResult = pVdsVolumeMF->DeleteAccessPath(accessPaths[i], true);
                    Assert(hResult, goto releaseCOM);
                }
            }
        }

        if (mountPoint && lstrlenW(mountPoint))
        {
            wlogc(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, L"Assigning the access point...");
            wchar_t mountBuffer[MAX_PATH];
            wcscpy_s(mountBuffer, mountPoint);
            if (lstrlenW(mountBuffer) == 1)
                wcscat_s(mountBuffer, L":\\");
            hResult = pVdsVolumeMF->AddAccessPath(mountBuffer);
        }
    }

releaseCOM:
    SafeCoFree(accessPaths);
    SafeRelease(pVdsVolumeMF);
    SafeRelease(pVdsAdvDisk);
    SafeRelease(pVdsDisk);
    SafeRelease(pVdsService);
    return hResult;
}

HRESULT ShrinkPartition(int diskNumber, unsigned long long partOffset, unsigned long long sizeToShrink)
{
    wlogf(PartitionManager::GetLogger(), PANTHER_LL_VERBOSE, MAX_PATH, L"Starting format operation on %d@%llu", diskNumber, partOffset);

    HRESULT hResult, asyncRes;

    IVdsService* pVdsService = nullptr;
    IVdsDisk* pVdsDisk = nullptr;
    IVdsAdvancedDisk* pVdsAdvDisk = nullptr;
    IVdsVolume* pVdsVolume = nullptr;

    IUnknown* pUnknown = nullptr;
    IVdsAsync* pVdsAsync = nullptr;
    VDS_ASYNC_OUTPUT vdsAsyncOut;
    bool isOEM = false;

    wchar_t** accessPaths = nullptr;
    ULONG ulFetchCount = 0;

    hResult = VdsStartSession(&pVdsService);
    Assert(hResult, return hResult);

    hResult = VdsFindDisk(pVdsService, diskNumber, &pVdsDisk);
    Assert(hResult, goto releaseCOM);

    hResult = VdsIsPartitionOEM(pVdsDisk, partOffset, &isOEM);
    Assert(hResult, goto releaseCOM);
    if (isOEM)
    {
        wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Partition is an OEM partition, it is not possible to shrink it.");
        hResult = VDS_E_NOT_SUPPORTED; goto releaseCOM;
    }

    hResult = VdsGetVolumeOnDisk(pVdsService, pVdsDisk, partOffset, &pUnknown);
    Assert(hResult, goto releaseCOM);
    hResult = pUnknown->QueryInterface(&pVdsVolume);
    SafeRelease(pUnknown);
    Assert(hResult, goto releaseCOM);

    wlogc(PartitionManager::GetLogger(), PANTHER_LL_DETAILED, L"Partition is a non-OEM partition, shrinking the volume...");
    pVdsVolume->Shrink(sizeToShrink, &pVdsAsync);
    Assert(hResult, goto releaseCOM);
    asyncRes = pVdsAsync->Wait(&hResult, &vdsAsyncOut);
    Assert(hResult, goto releaseCOM);
    Assert(asyncRes, hResult = asyncRes; goto releaseCOM);

releaseCOM:
    SafeCoFree(accessPaths);
    SafeRelease(pVdsAsync);
    SafeRelease(pVdsVolume);
    SafeRelease(pVdsDisk);
    SafeRelease(pVdsService);
    return hResult;
}