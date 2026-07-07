#include "PartitionManager.h"
#include "utils.h"

bool Leet::WinParted::PartitionManager::GptDiskPartitionTable::Load()
{
	Utils::Win32Handle hDisk(CreateFileW(m_diskInfo.DiskPath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
	if (!hDisk)
	{
		// TODO: report error
		return false;
	}

	std::vector<char> headerBuffer(m_diskInfo.SectorSize * 2ULL);
	if (!ReadFile(hDisk, headerBuffer.data(), static_cast<DWORD>(headerBuffer.size()), nullptr, nullptr))
	{
		// TODO: report error
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"");
		return false;
	}

	m_gptHeader = *reinterpret_cast<GPT_HEADER*>(headerBuffer.data() + m_diskInfo.SectorSize);
	m_gptEntries.clear();
	m_partitions.clear();

	if (m_gptHeader.Signature != 0x5452415020494645ULL)
	{
		// TODO: Initialize new GPT
		wlogc(m_manager.logger, PANTHER_LL_NORMAL, L"The disk does not have a valid GPT, creating new table");
		m_newTable = true;
		return true;
	}


	unsigned long long partitionTableOffset = m_gptHeader.TableLBA.ULL * m_diskInfo.SectorSize;
	if (SetFilePointer(hDisk, static_cast<long>(partitionTableOffset), reinterpret_cast<PLONG>(&partitionTableOffset) + 1, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		// TODO: report error
		return false;
	}

	unsigned long long partitionTableSize = m_gptHeader.TableEntryCount * m_gptHeader.TableEntrySize;
	if (unsigned long long remainder = partitionTableSize % m_diskInfo.SectorSize) partitionTableSize += m_diskInfo.SectorSize - remainder;

	std::vector<char> partitionTableBuffer(partitionTableSize);
	if (!ReadFile(hDisk, partitionTableBuffer.data(), static_cast<DWORD>(partitionTableBuffer.size()), nullptr, nullptr))
	{
		// TODO: report error
		return false;
	}

	WP_PART_INFO partitionInfo = { };
	for (unsigned int i = 0; i < m_gptHeader.TableEntryCount; i++)
	{
		GPT_ENTRY* entry = reinterpret_cast<GPT_ENTRY*>(partitionTableBuffer.data() + i * m_gptHeader.TableEntrySize);
		m_gptEntries.push_back(*entry);

		if (memcmp(&entry->TypeGUID, &GUID_NULL, sizeof(GUID)) == 0)
			continue;

		partitionInfo.DiskNumber = m_diskInfo.DiskNumber;
		partitionInfo.PartitionNumber = i + 1;
		partitionInfo.Type.TypeGUID = entry->TypeGUID;
		partitionInfo.StartLBA = entry->StartLBA;
		partitionInfo.EndLBA = entry->EndLBA;
		partitionInfo.SectorCount = partitionInfo.EndLBA.ULL - partitionInfo.StartLBA.ULL + 1;
		memcpy_s(partitionInfo.Name, sizeof(wchar_t) * 36, entry->Name, sizeof(wchar_t) * 36);
		m_partitions.push_back(partitionInfo);
	}

	return true;
}

int Leet::WinParted::PartitionManager::GptDiskPartitionTable::GetPartitionCount()
{
	return m_partitions.size();
}

bool Leet::WinParted::PartitionManager::GptDiskPartitionTable::GetPartition(int index, WP_PART_INFO& partitionInfo)
{
	if (index >= m_partitions.size() || index < 0)
	{
		// TODO: report error
		return false;
	}

	memcpy(&partitionInfo, &m_partitions[index], sizeof(WP_PART_INFO));
	return true;
}

bool Leet::WinParted::PartitionManager::GptDiskPartitionTable::DeletePartition(int index)
{
	if (index > m_partitions.size() || index < 0)
	{
		// TODO: report error
		return false;
	}

	WP_PART_INFO& partition = m_partitions[index];
	int gptIndex = partition.PartitionNumber - 1;
	if (gptIndex < 0 || gptIndex >= m_gptEntries.size())
	{
		// TODO: report error
		return false;
	}

	m_gptEntries[gptIndex] = { };
	m_partitions.erase(m_partitions.begin() + index);

	return true;
}

bool Leet::WinParted::PartitionManager::GptDiskPartitionTable::FlushChangesToDisk()
{
	wlogc(m_manager.logger, PANTHER_LL_DETAILED, L"Saving partition table to disk...");

	Utils::Win32Handle hDisk(CreateFileW(m_diskInfo.DiskPath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
	if (!hDisk)
	{
		// TODO: report error
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"Failed to open disk %s for writing: CreateFile returned %d", m_diskInfo.DiskPath, GetLastError());
		return false;
	}

	unsigned long long partitionTableSize = m_gptHeader.TableEntryCount * m_gptHeader.TableEntrySize;
	if (unsigned long long remainder = partitionTableSize % m_diskInfo.SectorSize) partitionTableSize += m_diskInfo.SectorSize - remainder;

	std::vector<char> gptBuffer(m_diskInfo.SectorSize);
	std::vector<char> partitionTableBuffer(partitionTableSize);

	memcpy(partitionTableBuffer.data(), m_gptEntries.data(), m_gptEntries.size() * sizeof(GPT_ENTRY));

	LBA backupGptTableLocation = m_gptHeader.TableLBA;
	if (m_newTable)
	{
		wlogc(m_manager.logger, PANTHER_LL_DETAILED, L"Creating new GPT backup header and table.", m_diskInfo.DiskPath);
		backupGptTableLocation.ULL = m_diskInfo.SectorCount
			- (partitionTableSize / m_diskInfo.SectorSize)
			- 1;
	}
	else
	{
		wlogc(m_manager.logger, PANTHER_LL_DETAILED, L"Overwriting existing GPT backup header and table.", m_diskInfo.DiskPath);
		unsigned long long backupGptLocation = m_gptHeader.BackupHeaderLBA.ULL * m_diskInfo.SectorSize;
		if (SetFilePointer(hDisk, static_cast<long>(backupGptLocation), reinterpret_cast<PLONG>(&backupGptLocation) + 1, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			wlogc(m_manager.logger, PANTHER_LL_BASIC, L"Failed to set file pointer to backup GPT header location.");
			return false;
		}
		if (!ReadFile(hDisk, gptBuffer.data(), m_diskInfo.SectorSize, nullptr, nullptr))
		{
			wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"The backup GPT header could not be read: ReadFile returned %d", GetLastError());
			return false;
		}

		backupGptTableLocation = reinterpret_cast<GPT_HEADER*>(gptBuffer.data())->TableLBA;
	}

	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Calculating CRC for main GPT header and table.");
	GPT_HEADER* gptHeader = reinterpret_cast<GPT_HEADER*>(gptBuffer.data());
	*gptHeader = m_gptHeader;

	// Use actual size of table entries for CRC calculation, not the padded size
	gptHeader->TableCRC = Utils::CalculateCRC32(partitionTableBuffer.data(), m_gptHeader.TableEntryCount * m_gptHeader.TableEntrySize);
	gptHeader->HeaderCRC = 0;
	gptHeader->HeaderCRC = Utils::CalculateCRC32(gptBuffer.data(), gptHeader->HeaderSize);

	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Writing main GPT header to disk.");
	if (SetFilePointer(hDisk, m_diskInfo.SectorSize, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"Failed to set file pointer to main GPT header location: SetFilePointer returned %d", GetLastError());
		return false;
	}
	if (!WriteFile(hDisk, gptBuffer.data(), m_diskInfo.SectorSize, nullptr, nullptr))
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"The main GPT header could not be written: WriteFile returned %d", GetLastError());
		return false;
	}

	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Writing main GPT table to disk.");
	if (SetFilePointer(hDisk, m_gptHeader.TableLBA.ULL * m_diskInfo.SectorSize, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"Failed to set file pointer to main GPT table location: SetFilePointer returned %d", GetLastError());
		return false;
	}
	if (!WriteFile(hDisk, partitionTableBuffer.data(), static_cast<DWORD>(partitionTableBuffer.size()), nullptr, nullptr))
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"The main GPT table could not be written: WriteFile returned %d", GetLastError());
		return false;
	}

	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Calculating CRC for backup GPT header.");
	gptHeader->HeaderCRC = 0;
	gptHeader->TableLBA = backupGptTableLocation;
	gptHeader->BackupHeaderLBA = m_gptHeader.CurrentHeaderLBA;
	gptHeader->CurrentHeaderLBA = m_gptHeader.BackupHeaderLBA;
	gptHeader->HeaderCRC = Utils::CalculateCRC32(gptBuffer.data(), gptHeader->HeaderSize);

	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Writing backup GPT header to disk.");
	unsigned long long backupGptHeaderLocation = backupGptTableLocation.ULL * m_diskInfo.SectorSize;
	if (SetFilePointer(hDisk, static_cast<long>(backupGptHeaderLocation), reinterpret_cast<PLONG>(&backupGptHeaderLocation) + 1, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"Failed to set file pointer to backup GPT header location: SetFilePointer returned %d", GetLastError());
		return false;
	}
	if (!WriteFile(hDisk, gptBuffer.data(), m_diskInfo.SectorSize, nullptr, nullptr))
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"The backup GPT header could not be written: WriteFile returned %d", GetLastError());
		return false;
	}
	
	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Writing backup GPT table to disk.");
	unsigned long long backupGptTableLocBytes = backupGptTableLocation.ULL * m_diskInfo.SectorSize;
	if (SetFilePointer(hDisk, static_cast<long>(backupGptTableLocBytes), reinterpret_cast<PLONG>(&backupGptTableLocBytes) + 1, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		wlogf(m_manager.logger, PANTHER_LL_BASIC, MAX_PATH, L"Failed to set file pointer to backup GPT table location: SetFilePointer returned %d", GetLastError());
		return false;
	}

	wlogc(m_manager.logger, PANTHER_LL_VERBOSE, L"Finished saving partition table.");

	return false;

}