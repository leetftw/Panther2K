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

class WinPartedDll 
{
public:
	static int RunWinParted(Console*, LibPanther::Logger*);
	static HRESULT ApplyP2KLayoutToDiskGPT(Console*, LibPanther::Logger*, int, bool, wchar_t***, wchar_t***);
	static HRESULT ApplyP2KLayoutToDiskMBR(Console*, LibPanther::Logger*, int, bool, wchar_t***, wchar_t***);
	static HRESULT SetPartType(Console*, LibPanther::Logger*, int, unsigned long long, short);
	static HRESULT MountPartition(Console*, LibPanther::Logger*, int, unsigned long long, const wchar_t*);
	static HRESULT EnumerateDisks(Console* console, LibPanther::Logger* logger, DISK_INFORMATION** disks, int* diskCount);
private:
	static HMODULE hWinParted;
	static HRESULT InitParted();
	static bool partedInitialized;
};