// Leet.WinParted.Bootstrap.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <WinParted.h>
#include <PantherConsole.h>
#include <PantherLayout.h>
#include <PantherVersion.h>
#include <PantherText.h>

#include "DiskSelectionPage.h"
#include "PartitioningPage.h"

void GetSizeStringFromBytes(unsigned long long bytes, wchar_t buffer[10])
{
	// Overkill
	bool useSI = false;
	double factor = useSI ? 1000.0 : 1024.0;
	const wchar_t valueStringsSI[9][4] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB", L"ZB", L"YB" };
	const wchar_t valueStringsBinary[9][4] = { L"B", L"KiB", L"MiB", L"GiB", L"TiB", L"PiB", L"EiB", L"ZiB", L"YiB" };

	int index = 0;
	double bytesFp = bytes;
	while (bytesFp > factor)
	{
		bytesFp /= factor;
		index++;
	}

	swprintf(buffer, 10, L"%.1f%s", bytesFp, useSI ? valueStringsSI[index] : valueStringsBinary[index]);
}

int main()
{
	Leet::Panther2K::Util::Logger logger(L"debug.log", PANTHER_LL_VERBOSE);
	Leet::WinParted::PartitionManager partitionManager(&logger);
	Leet::Panther2K::Util::CustomConsole console;

	Leet::WinParted::Frontend::DiskSelectionPage page(&console, &partitionManager);

	console.Init();
	ShowWindow(console.WindowHandle, SW_SHOW);
	console.SetPixelScale(1);

	page.Show();

	auto selectedDisk = page.GetResult();
	if (selectedDisk.first == Leet::LibPanther::Layout::PageContinue)
	{
		if (auto disk = selectedDisk.second.lock())
		{
			wprintf(L"opened disk: %s", disk->GetDiskInfo().DiskPath);

			std::weak_ptr partitionTable = disk->OpenPartitionTable(Leet::WinParted::WP_OPERATING_MODE::GPT);
			Leet::WinParted::Frontend::PartitioningPage partPage(&console, &partitionManager, partitionTable);
			partPage.Show();


		}
	}
}