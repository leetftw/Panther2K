#include "WinPartedDll.h"
#include "iatpatch.h"

/*
EXPORTS
   InitializeCRT   @1
   RunWinParted   @2
   ApplyP2KLayoutToDiskGPT   @3
   ApplyP2KLayoutToDiskMBR   @4
   SetPartType   @5
   ORD_MountPartition   @6
*/

#define ORD_RunWinParted              (LPCSTR)2
#define ORD_ApplyP2KLayoutToDiskGPT   (LPCSTR)3
#define ORD_ApplyP2KLayoutToDiskMBR   (LPCSTR)4
#define ORD_SetPartType               (LPCSTR)5
#define ORD_MountPartition            (LPCSTR)6
#define ORD_EnumerateDisks            (LPCSTR)7
#define ORD_PrepareDiskForWindows     (LPCSTR)8
#define ORD_EnumVolumes               (LPCSTR)9

typedef int (*RunWinPartedStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*);
typedef HRESULT(*ApplyP2KLayoutToDiskGPTStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, int, bool, wchar_t***, wchar_t***);
typedef HRESULT(*ApplyP2KLayoutToDiskMBRStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, int, bool, wchar_t***, wchar_t***);
typedef HRESULT(*SetPartTypeStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, int, unsigned long long, short);
typedef HRESULT(*MountPartitionStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, int, unsigned long long, const wchar_t*);
typedef HRESULT(*EnumerateDisksStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, DISK_INFORMATION**, int*);
typedef HRESULT(*PrepareDiskForWindowsStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, const wchar_t*, bool, unsigned long long, unsigned long long, wchar_t[2][128]);
typedef HRESULT(*EnumVolumesStub)(Leet::Panther2K::Util::Console*, Leet::Panther2K::Util::Logger*, VolumeInformation**, bool, int*);

bool WinPartedDll::partedInitialized = false;
HMODULE WinPartedDll::hWinParted = NULL;

int WinPartedDll::RunWinParted(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger)
{
	if (!partedInitialized && InitParted() != ERROR_SUCCESS)
		return ERROR_BAD_FORMAT;
	auto runWinParted = (RunWinPartedStub)GetProcAddress(hWinParted, ORD_RunWinParted);
	return runWinParted(console, logger);
}

HRESULT WinPartedDll::ApplyP2KLayoutToDiskGPT(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, bool letters, wchar_t*** mountPath, wchar_t*** volumeList)
{
	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BAD_FORMAT);

	auto applyP2kLayout = (ApplyP2KLayoutToDiskGPTStub)GetProcAddress(hWinParted, ORD_ApplyP2KLayoutToDiskGPT);
	return applyP2kLayout(console, logger, diskNumber, letters, mountPath, volumeList);
}

HRESULT WinPartedDll::ApplyP2KLayoutToDiskMBR(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, bool letters, wchar_t*** mountPath, wchar_t*** volumeList)
{
	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, res);

	auto applyP2kLayout = (ApplyP2KLayoutToDiskMBRStub)GetProcAddress(hWinParted, ORD_ApplyP2KLayoutToDiskMBR);
	return applyP2kLayout(console, logger, diskNumber, letters, mountPath, volumeList);
}

HRESULT WinPartedDll::SetPartType(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, unsigned long long partOffset, short partType)
{
	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, res);

	auto setPartType = (SetPartTypeStub)GetProcAddress(hWinParted, ORD_SetPartType);
	return setPartType(console, logger, diskNumber, partOffset, partType);
}

HRESULT WinPartedDll::MountPartition(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, int diskNumber, unsigned long long partOffset, const wchar_t* mountPoint)
{

	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, res);

	auto mountPartition = (MountPartitionStub)GetProcAddress(hWinParted, ORD_MountPartition);
	return mountPartition(console, logger, diskNumber, partOffset, mountPoint);
}

HRESULT WinPartedDll::EnumerateDisks(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, DISK_INFORMATION** disks, int* diskCount)
{
	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, res);

	auto enumerateDisks = (EnumerateDisksStub)GetProcAddress(hWinParted, ORD_EnumerateDisks);
	return enumerateDisks(console, logger, disks, diskCount);
}

HRESULT WinPartedDll::PrepareDiskForWindows(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, const wchar_t* volumeGuid, bool useLegacy,
	unsigned long long requiredBootSize, unsigned long long requiredRESize, wchar_t installVolumes[2][128])
{
	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, res);

	auto prepareDisk = (PrepareDiskForWindowsStub)GetProcAddress(hWinParted, ORD_PrepareDiskForWindows);
	return prepareDisk(console, logger, volumeGuid, useLegacy, requiredBootSize, requiredRESize, installVolumes);
}

HRESULT WinPartedDll::EnumVolumes(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, VolumeInformation** volumes, bool includeDynamic, int* count)
{
	HRESULT res;
	if (!partedInitialized && (res = InitParted()) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, res);

	auto enumVolumes = (EnumVolumesStub)GetProcAddress(hWinParted, ORD_EnumVolumes);
	return enumVolumes(console, logger, volumes, includeDynamic, count);

	return S_OK;
}

HRESULT WinPartedDll::InitParted()
{
	if (partedInitialized) return ERROR_SUCCESS;

	// Try loading WinParted
	hWinParted = LoadLibraryA("Leet.WinParted.PartitionManager.dll");
	if (!hWinParted)
	{
		//wlogf(WindowsSetup::GetLogger(), PANTHER_LL_BASIC, 60, L"Error occurred while loading WinParted (0x%08x).", GetLastError());
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
	}
	
	partedInitialized = true;
	return ERROR_SUCCESS;
}