#include "PartitionSelectionPage.h"
#include "QuitingPage.h"
#include "MessageBoxPage.h"
#include "WinPartedDll.h"

const wchar_t* const part1Strings[] = 
{ 
	L"Select the System partition.", 
	L"Select the EFI System Partition partition.", 
	L"Select the System Reserved partition.", 
	L"Select the Recovery partition." 
};
const wchar_t* const part2Strings[] = 
{
	L"Panther2K will use this partition to store the system files for the Microsoft Windows operating system. This partition will be available as the C: drive from within Windows.", 
	L"Panther2K will use this partition to store the files required for booting Microsoft Windows on your computer. This is a system reserved partition that will not be available for use inside Windows after installation.",
	L"Panther2K will use this partition to store the files required for booting Microsoft Windows on your computer. This is a system reserved partition that will not be available for use inside Windows after installation.", 
	L"Panther2K will use this partition to store the files required for booting into the Windows Recovery Environment (WinRE). WinRE can be used whenever Windows fails to load, for example due to an incompatible driver update."
};

PartitionSelectionPage::PartitionSelectionPage(const wchar_t* fileSystem, long long minimumSize, long long minimumBytesAvailable, int stringIndex, int displayIndex)
{
	requirements.fileSystem = fileSystem;
	requirements.partitionSize = minimumSize;
	requirements.partitionFree = minimumBytesAvailable;
	stringTableIndex = stringIndex;
	dispIndex = displayIndex;
}

void PartitionSelectionPage::EnumeratePartitions()
{
	BOOL success;
	wchar_t szNextVolName[MAX_PATH + 1];
	HANDLE volume;
	
	volume = FindFirstVolume(szNextVolName, MAX_PATH);
	success = (volume != INVALID_HANDLE_VALUE);
	while (success)
	{
	/*
		VOLUME_INFO vi;
		if (!WindowsSetup::GetVolumeInfoFromName(szNextVolName, &vi))
			goto nextVol;
		if (!showAll && !WindowsSetup::AllowSmallVolumes && (vi.bytesFree < requirements.partitionFree || vi.totalBytes < requirements.partitionSize))
			goto nextVol;
		if (!showAll && !WindowsSetup::AllowOtherFileSystems && lstrcmpW(vi.fileSystem, requirements.fileSystem) != 0)
			goto nextVol;

		volumeInfo.push_back(vi);
	nextVol:
		success = FindNextVolume(volume, szNextVolName, MAX_PATH) != 0;
		*/
	}

	Draw();
}

VOLUME_INFO PartitionSelectionPage::GetSelectedVolume()
{
	return volumeInfo[scrollIndex + selectionIndex];
}

void PartitionSelectionPage::Init()
{
	statusText = L"  ENTER=Select  F8=Run WinParted  F9=Display all  ESC=Back  F3=Quit";
}

void PartitionSelectionPage::Drawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);

	DrawTextLeft(part1Strings[stringTableIndex], console->GetSize().cx - 6, 4);

	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	DrawTextLeft(part2Strings[stringTableIndex], console->GetSize().cx - 6, console->GetPosition().y + 2);

	DrawTextLeft(L"Use the UP and DOWN ARROW keys to select the partition.", console->GetSize().cx - 6, console->GetPosition().y + 2);

	DrawTextLeft(L"Press F8 to modify your partition layout using DiskPart.", console->GetSize().cx - 6, console->GetPosition().y + 1);

	int boxX = 3;
	boxY = console->GetPosition().y + 2;
	int boxWidth = console->GetSize().cx - 6;
	int boxHeight = console->GetSize().cy - (boxY + 2);
	DrawBox(boxX, boxY, boxWidth, boxHeight, true);

	wchar_t buffer[MAX_PATH];

	swprintf(buffer, boxWidth - 2, L"   Disk  Partition  Volume Name%*sSize (GB)  Mount Point  ", boxWidth - 58, L"");
	console->SetPosition(boxX + 1, boxY + 1);
	console->Write(buffer);
	free(buffer);
}

void PartitionSelectionPage::Redrawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	int boxX = 3;
	int boxWidth = console->GetSize().cx - 6;
	int boxHeight = console->GetSize().cy - (boxY + 2);
	int maxItems = boxHeight - 3;

	bool canScrollDown = (scrollIndex + maxItems) < volumeInfo.size();
	bool canScrollUp = scrollIndex != 0;

	wchar_t buffer[MAX_PATH];
	for (int i = 0; i < min(volumeInfo.size(), maxItems); i++)
	{
		if (i == selectionIndex)
		{
			console->SetBackgroundColor(CONSOLE_COLOR_FG);
			console->SetForegroundColor(CONSOLE_COLOR_BG);
		}
		else
		{
			console->SetBackgroundColor(CONSOLE_COLOR_BG);
			console->SetForegroundColor(CONSOLE_COLOR_FG);
		}

		console->SetPosition(boxX + 4, boxY + i + 2);
		swprintf(buffer, boxWidth - 2, L"%4d  %-9d  %-*s%10.1F  %-11s", volumeInfo[i].diskNumber, volumeInfo[i].partitionNumber, boxWidth - 48, volumeInfo[i].name, static_cast<float>(volumeInfo[i].totalBytes / 1000) / 1000.0, volumeInfo[i].mountPoint);
		console->Write(buffer);
	}
	free(buffer);
}

bool PartitionSelectionPage::KeyHandler(WPARAM wParam)
{
	int boxX = 3;
	int boxWidth = console->GetSize().cx - 6;
	int boxHeight = console->GetSize().cy - (boxY + 2);
	int maxItems = boxHeight - 3;
	int totalItems = volumeInfo.size();
	switch (wParam)
	{
	case VK_DOWN:
		if (selectionIndex + 1 < min(maxItems, totalItems))
			selectionIndex++;
		else if (selectionIndex + scrollIndex + 1 < totalItems)
			scrollIndex++;
		Redraw();
		break;
	case VK_UP:
		if (selectionIndex - 1 >= 0)
			selectionIndex--;
		else if (selectionIndex + scrollIndex - 1 >= 0)
			scrollIndex--;
		Redraw();
		break;
	case VK_F3:
		AddPopup(new QuitingPage());
		break;
	case VK_F8:
		//WinPartedDll::RunWinParted(console, WindowsSetup::GetLogger());
		Draw();
		break;
	case VK_F9:
		showAll = !showAll;
		EnumeratePartitions();
		break;
	case VK_ESCAPE:
		break;
	case VK_RETURN:
		break;
	}
	return true;
}
