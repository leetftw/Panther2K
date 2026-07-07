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
		wprintf(L"Disk %d: %s\n", diskInfo.DiskNumber, diskInfo.DiskPath);

		std::weak_ptr<Leet::WinParted::PartitionManager::DiskPartitionTable> partTable = diskLock->OpenPartitionTable(Leet::WinParted::WP_OPERATING_MODE::GPT);
		if (std::shared_ptr<Leet::WinParted::PartitionManager::DiskPartitionTable> partTableLock = partTable.lock())
		{
			for (int i = 0; i < partTableLock->GetPartitionCount(); i++)
			{
				WP_PART_INFO partitionInfo;
				if (partTableLock->GetPartition(i, partitionInfo))
				{
					wprintf(L"Partition %d: Name %s, Start LBA %llu, End LBA %llu\n",
						partitionInfo.PartitionNumber,
						partitionInfo.Name,
						partitionInfo.StartLBA.ULL,
						partitionInfo.EndLBA.ULL);
				}
			}
		}
		else
		{
			wprintf(L"Failed to open partition table!\n");
		}
	}
	else
	{
		wprintf(L"Failed to lock disk!\n");
	}
}