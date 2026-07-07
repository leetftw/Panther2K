#pragma once
#include "windows.h"

#pragma pack(push, 1)

union CHS
{
	char Bytes[3];
};

union LBA
{
	unsigned long long ULL;
	unsigned long UL[2];
	unsigned short US[4];
	unsigned char Char[8];
};

struct GPT_ENTRY
{
	GUID TypeGUID;
	GUID UniqueGUID;
	LBA StartLBA;
	LBA EndLBA;
	unsigned long long AttributeFlags;
	wchar_t Name[36];
};

struct GPT_HEADER
{
	unsigned long long Signature;
	unsigned long Revision;
	unsigned long HeaderSize;
	unsigned long HeaderCRC;
	unsigned long Reserved;
	LBA CurrentHeaderLBA;
	LBA BackupHeaderLBA;
	LBA FirstUsableLBA;
	LBA LastUsableLBA;
	GUID DiskGUID;
	LBA TableLBA;
	unsigned long TableEntryCount;
	unsigned long TableEntrySize;
	unsigned long TableCRC;
};

struct MBR_ENTRY
{
	char BootIndicator;
	CHS StartCHS;
	char SystemID;
	CHS EndCHS;
	unsigned long StartLBA;
	unsigned long SectorCount;
};

struct MBR_HEADER
{
	char Bootstrap1[440];
	unsigned long DiskSignature1;
	unsigned short DiskSignature2;
	MBR_ENTRY PartitionTable[4];
	unsigned short BootSignature;
};

struct VolumeInformation
{
	wchar_t FileSystem[16];
	wchar_t VolumeName[128];
	wchar_t VolumeFile[128];
	wchar_t MountPoint[MAX_PATH];
	unsigned long long TotalSize;
	unsigned long long SpaceFree;
	unsigned int DiskNumber;
	unsigned int PartitionNumber;
};

struct WP_PART_TYPE
{
	union
	{
		GUID TypeGUID;
		char SystemID;
	};
};

struct WP_PART_INFO
{
	long DiskNumber;
	long PartitionNumber;
	WP_PART_TYPE Type;
	LBA StartLBA;
	LBA EndLBA;
	unsigned long long SectorCount;
	wchar_t Name[36];
	bool VolumeLoaded;
	VolumeInformation VolumeInformation;
};

#pragma pack(pop)