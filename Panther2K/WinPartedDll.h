#pragma once
#include <PantherLogger.h>
#include <PantherConsole.h>

struct DISK_INFORMATION
{
	unsigned int DiskNumber;
	wchar_t DiskPath[64];
	wchar_t DeviceName[256];
	unsigned int PartitionCount;
	MEDIA_TYPE MediaType;
	unsigned int SectorSize;
	unsigned long long SectorCount;
};

struct VolumeInformation
{
	wchar_t FileSystem[16];
	wchar_t VolumeName[128];
	wchar_t VolumeFile[128];
};

class WinPartedDll 
{
public:
	static int RunWinParted(Console*, LibPanther::Logger*);
	static HRESULT ApplyP2KLayoutToDiskGPT(Console*, LibPanther::Logger*, int, bool, wchar_t***, wchar_t***);
	static HRESULT ApplyP2KLayoutToDiskMBR(Console*, LibPanther::Logger*, int, bool, wchar_t***, wchar_t***);
	static HRESULT SetPartType(Console*, LibPanther::Logger*, int, unsigned long long, short);
	static HRESULT MountPartition(Console*, LibPanther::Logger*, int, unsigned long long, const wchar_t*);
	static HRESULT EnumerateDisks(Console* console, LibPanther::Logger* logger, DISK_INFORMATION** disks, int* diskCount);
	static HRESULT PrepareDiskForWindows(Console* console, LibPanther::Logger* logger, const wchar_t* volumeGuid, bool useLegacy,
		unsigned long long requiredBootSize, unsigned long long requiredRESize, wchar_t installVolumes[2][128]);
	static HRESULT EnumVolumes(Console* console, LibPanther::Logger* logger, VolumeInformation** volumes);

private:
	static HMODULE hWinParted;
	static HRESULT InitParted();
	static bool partedInitialized;
};