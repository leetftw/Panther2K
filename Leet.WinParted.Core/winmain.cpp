#include "PartitionManager.h"

int main()
{
	Leet::Panther2K::Util::Logger logger(L"debug.log", PANTHER_LL_VERBOSE);
	Leet::WinParted::PartitionManager partitionManager(&logger);

	partitionManager.Refresh();
	auto disk = partitionManager.OpenDisk(0);
	if (std::shared_ptr<Leet::WinParted::PartitionManager::Disk> diskLock = disk.lock())
	{
		Leet::WinParted::WP_DISK_INFO diskInfo = diskLock->GetDiskInfo();
		wprintf(L"Disk %d: %s\n", diskInfo.DiskNumber, diskInfo.DeviceName);
	}
	else
	{
		wprintf(L"Failed to lock disk!\n");
	}
}