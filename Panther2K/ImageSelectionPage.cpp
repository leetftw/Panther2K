#include "ImageSelectionPage.h"
#include "QuitingPage.h"

bool ImageSelectionPage::SetData(PantherWimInfo* wimInfo)
{
	wchar_t buffer[MAX_PATH];

	for (int i = 0; i < wimInfo->ImageCount; i++)
	{
		const wchar_t* arch;
		switch (wimInfo->Images[i].Architecture)
		{
		case 0:
			arch = L"x86";
			break;
		case 9:
			arch = L"x86_64";
			break;
		case 5:
			arch = L"ARM64";
			break;
		default:
			arch = L"UNKNWN";
			break;
		}
		
		SYSTEMTIME st;
		if (!FileTimeToSystemTime(&wimInfo->Images[i].CreationTime, &st))
		{
			return false;
		}

		swprintf_s(buffer, L"%-*s %-6s %02d/%02d/%04d", console->GetSize().cx - 16 - 18, 
			wimInfo->Images[i].DisplayName, arch, st.wDay, st.wMonth, st.wYear);
		
		FormattedStrings.push_back(std::wstring(buffer));
	}
}

int ImageSelectionPage::GetResult()
{
	return scrollIndex + selectionIndex + 1;
}

void ImageSelectionPage::Init()
{
	statusText = L"  ENTER=Select  ESC=Back  F3=Quit";
}

void ImageSelectionPage::Drawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);
	console->SetPosition(3, 4);
	console->Write(L"Select the operating system to be installed.");

	console->SetForegroundColor(CONSOLE_COLOR_FG);
	DrawTextLeft(L"Multiple operating systems were detected inside the WIM or ESD image. Please select the copy of Microsoft(R) Windows(TM) you would like to install onto your computer.", console->GetSize().cx - 6, 6);
	DrawTextLeft(L"Use the UP and DOWN arrow keys to select an operating system.", console->GetSize().cx - 6, console->GetPosition().y + 2);

	SIZE consoleSize = console->GetSize();
	int boxX = 3;
	int boxWidth = consoleSize.cx - 6;
	boxY = console->GetPosition().y + 2;
	int boxHeight = consoleSize.cy - boxY - 2;
	int maxItems = boxHeight - 3;
	DrawBox(boxX, boxY, boxWidth, boxHeight, false);

	DrawTextLeft(L"Name", console->GetSize().cx - 16, boxY + 1);
	DrawTextRight(L"Arch   Date      ", console->GetSize().cx - 16, boxY + 1);
}

void ImageSelectionPage::Redrawer()
{
	SIZE consoleSize = console->GetSize();
	int boxHeight = consoleSize.cy - boxY - 2;
	int maxItems = boxHeight - 3;

	bool canScrollDown = (scrollIndex + maxItems) < FormattedStrings.size();
	bool canScrollUp = scrollIndex != 0;

	for (int i = 0; i < min(maxItems, FormattedStrings.size()); i++)
	{
		int j = i + scrollIndex;
		const wchar_t* text = FormattedStrings[j].c_str();
		/*wchar_t buffer[100];
		swprintf(buffer, 100, L"maxItems: %i\nimageCount: %i\nstringCount: %i\ni: %i\nscroll: %i\nj: %i", maxItems, WindowsSetup::WimImageCount, FormattedStrings.size(), i, scrollIndex, j);
		MessageBoxW(console->WindowHandle, buffer, L"", MB_OK);*/

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
		console->SetPosition(8, boxY + 2 + i);
		console->Write(text);
	}

	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->SetPosition(console->GetSize().cx - 6, boxY + boxHeight - 2);
	if (canScrollDown) console->Write(L"↓");
	else console->Write(L" ");
	
	console->SetPosition(console->GetSize().cx - 6, boxY + 2);
	if (canScrollUp) console->Write(L"↑");
	else console->Write(L" ");
}

bool ImageSelectionPage::KeyHandler(WPARAM wParam)
{
	SIZE consoleSize = console->GetSize();
	int boxHeight = consoleSize.cy - boxY - 2;
	int maxItems = boxHeight - 3;
	int totalItems = FormattedStrings.size();

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
	case VK_RETURN:
		//WindowsSetup::WimImageIndex = scrollIndex + selectionIndex + 1;
		//WindowsSetup::LoadPhase(3);
		return false;
	case VK_F3:
		AddPopup(new QuitingPage());
		break;
	case VK_ESCAPE:
		//WindowsSetup::LoadPhase(1);
		break;
	}
	return true;
}
