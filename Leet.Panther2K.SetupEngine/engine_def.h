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

#define TM_PANTHER_BASE WM_USER + 0x1337
#define TM_PANTHER_PROGRESS TM_PANTHER_BASE + 0
#define TM_PANTHER_ERROR TM_PANTHER_BASE + 1
#define TM_PANTHER_FINISH TM_PANTHER_BASE + 2
#define TM_PANTHER_FILENAME TM_PANTHER_BASE + 3
#define TM_PANTHER_BOOTSTEP TM_PANTHER_BASE + 4

#define PANTHER_BOOTSTEP_BOOT 1
#define PANTHER_BOOTSTEP_RECOVERY 2