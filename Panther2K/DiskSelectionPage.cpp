#include "DiskSelectionPage.h"

#include "QuittingPage.h"
#include "MessageBoxPage.h"

void GetSizeStringFromBytes(unsigned long long bytes, wchar_t buffer[10])
{
	// Overkill
	bool useSI = false;
	double factor = useSI ? 1000.0 : 1024.0;
	const wchar_t valueStringsSI[9][4] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB", L"ZB", L"YB" };
	const wchar_t valueStringsBinary[9][4] = { L"B", L"KiB", L"MiB", L"GiB", L"TiB", L"PiB", L"EiB", L"ZiB", L"YiB" };

	int index = 0;
	double bytesFp = bytes;
	while (bytesFp > factor && index < 9)
	{
		bytesFp /= factor;
		index++;
	}
	swprintf(buffer, 10, L"%.1f%s", bytesFp, useSI ? valueStringsSI[index] : valueStringsBinary[index]);
}

DiskSelectionPage::~DiskSelectionPage()
{
	if (diskInfo) LocalFree(diskInfo);
}

HRESULT DiskSelectionPage::LoadData(Console* console, LibPanther::Logger* logger)
{
	return WinPartedDll::EnumerateDisks(console, logger, &diskInfo, &diskCount);
}

int DiskSelectionPage::GetResult()
{
	if (scrollIndex + selectionIndex == diskCount) return -1;
	return diskInfo[scrollIndex + selectionIndex].DiskNumber;
}

void DiskSelectionPage::Init()
{
	statusText = L"  ENTER=Select  ESC=Back  F3=Quit";
}

void DiskSelectionPage::Drawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);

	console->DrawTextLeft(L"Select the disk to install Windows to.", console->GetSize().cx - 6, 4);

	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->DrawTextLeft(L"Windows will be installed to the disk specified. All data on the disk will be destroyed and Panther2K will create a bootable Windows installation on the disk. To install Windows without wiping a disk or while using a custom partition layout, select \"Custom\"", console->GetSize().cx - 6, console->GetPosition().y + 2);

	console->DrawTextLeft(L"Use the UP and DOWN ARROW keys to select the disk.", console->GetSize().cx - 6, console->GetPosition().y + 2);

	int boxX = 3;
	boxY = console->GetPosition().y + 2;
	int boxWidth = console->GetSize().cx - 6;
	int boxHeight = console->GetSize().cy - (boxY + 2);
	DrawBox(boxX, boxY, boxWidth, boxHeight, true);

	wchar_t* buffer = (wchar_t*)safeMalloc(nullptr, sizeof(wchar_t) * (boxWidth - 2));
	if (!buffer) return;

	swprintf(buffer, boxWidth - 2, L"   Disk  Device Name                        %*sType      Disk size   ", boxWidth - 68, L"");
	console->SetPosition(boxX + 1, boxY + 1);
	console->Write(buffer);
	safeFree(nullptr, buffer);
}

void DiskSelectionPage::Redrawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	int boxX = 3;
	int boxWidth = console->GetSize().cx - 6;
	int boxHeight = console->GetSize().cy - (boxY + 2);
	int maxItems = boxHeight - 3;

	bool canScrollDown = (scrollIndex + maxItems) < diskCount;
	bool canScrollUp = scrollIndex != 0;

	wchar_t buffer[MAX_PATH];
	wchar_t sizeBuffer[10];
	for (int i = 0; i < min(diskCount + 1, maxItems); i++)
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
		
		int j = i + scrollIndex;
		if (j == diskCount) {
			swprintf_s(buffer, MAX_PATH, L"  Custom...%*s", boxWidth - 19, L"");
		}
		else {
			GetSizeStringFromBytes(diskInfo[j].SectorCount * diskInfo[j].SectorSize, sizeBuffer);
			swprintf_s(buffer, MAX_PATH, L"%4d  %-*s  %-8s  %-9s", diskInfo[j].DiskNumber, boxWidth - 35, diskInfo[j].DeviceName, L"Standard", sizeBuffer);
		}

		console->Write(buffer);
	}
}

PageResult DiskSelectionPage::KeyHandler(WPARAM wParam)
{
	int boxX = 3;
	int boxWidth = console->GetSize().cx - 6;
	int boxHeight = console->GetSize().cy - (boxY + 2);
	int maxItems = boxHeight - 3;
	switch (wParam)
	{
	case VK_DOWN:
		if (selectionIndex + 1 < min(maxItems, diskCount + 1))
			selectionIndex++;
		else if (selectionIndex + scrollIndex + 1 < diskCount + 1)
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
	case VK_ESCAPE:
		return PageGoBack;
	case VK_RETURN:
		return PageContinue;
	case VK_F3:
		AddPopup(new QuittingPage());
		break;
	}
	return PageSuccess;
}
