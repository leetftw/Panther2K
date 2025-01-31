#pragma once
#include <windows.h>

struct PantherImageInfo
{
	unsigned int Architecture;
	FILETIME CreationTime;
	unsigned long long TotalSize;
	wchar_t DisplayName[MAX_PATH];
};

struct PantherWimInfo
{
	size_t ImageCount;
	PantherImageInfo Images[0];
};