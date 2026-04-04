#include "BootMethodSelectionPage.h"
#include "QuittingPage.h"

BootMethodSelectionPage::~BootMethodSelectionPage()
{

}

bool BootMethodSelectionPage::GetResult()
{
	return legacy;
}

void BootMethodSelectionPage::Init()
{
	/*wchar_t* displayName = WindowsSetup::WimImageInfos[WindowsSetup::WimImageIndex - 1].DisplayName;
	int length = lstrlenW(displayName);
	wchar_t* textBuffer = (wchar_t*)safeMalloc(WindowsSetup::GetLogger(), length * sizeof(wchar_t) + 14);
	memcpy(textBuffer, displayName, length * sizeof(wchar_t));
	memcpy(textBuffer + length, L" Setup", 14);
	text = textBuffer;*/
	statusText = L"  ENTER=Select  ESC=Back  F3=Quit";
}

void BootMethodSelectionPage::Drawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);

	console->SetPosition(3, 4);
	console->Write(L"Select how you want to boot your computer.");

	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->SetPosition(3, 6);
	console->Write(L"Windows can be set up to boot in two ways:");

	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	y = console->GetPosition().y + 2;
	console->SetPosition(6, y);
	console->Write(L"•");
	console->SetForegroundColor(legacy ? CONSOLE_COLOR_FG : CONSOLE_COLOR_BG);
	console->SetBackgroundColor(legacy ? CONSOLE_COLOR_BG : CONSOLE_COLOR_FG);
	console->DrawTextLeft(L"UEFI (Recommended): Modern method of booting. Uses a separate partition to store files required for booting the computer. (Required for Windows 11 and up)", console->GetSize().cx - 18, console->GetPosition().y);

	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetPosition(6, console->GetPosition().y + 2);
	console->Write(L"•");
	console->SetForegroundColor(legacy ? CONSOLE_COLOR_BG : CONSOLE_COLOR_FG);
	console->SetBackgroundColor(legacy ? CONSOLE_COLOR_FG : CONSOLE_COLOR_BG);
	console->DrawTextLeft(L"Legacy/BIOS: Traditional method of booting. Uses the first sector of your hard drive to store code for loading Windows.", console->GetSize().cx - 18, console->GetPosition().y);

	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->DrawTextLeft(L"To select the boot method, use the UP and DOWN arrow keys.", console->GetSize().cx - 6, console->GetPosition().y + 2);
	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->DrawTextLeft(L"To continue with the installation, press Enter.", console->GetSize().cx - 6, console->GetPosition().y + 2);
}

void BootMethodSelectionPage::Redrawer()
{
	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetPosition(6, y);
	console->Write(L"•");
	console->SetForegroundColor(legacy ? CONSOLE_COLOR_FG : CONSOLE_COLOR_BG);
	console->SetBackgroundColor(legacy ? CONSOLE_COLOR_BG : CONSOLE_COLOR_FG);
	console->DrawTextLeft(L"UEFI (Recommended): Modern method of booting. Uses a separate partition to store files required for booting the computer. (Required for Windows 11 and up)", console->GetSize().cx - 18, console->GetPosition().y);

	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetPosition(6, console->GetPosition().y + 2);
	console->Write(L"•");
	console->SetForegroundColor(legacy ? CONSOLE_COLOR_BG : CONSOLE_COLOR_FG);
	console->SetBackgroundColor(legacy ? CONSOLE_COLOR_FG : CONSOLE_COLOR_BG);
	console->DrawTextLeft(L"Legacy/BIOS: Traditional method of booting. Uses the first sector of your hard drive to store code for loading Windows.", console->GetSize().cx - 18, console->GetPosition().y);
}

PageResult BootMethodSelectionPage::KeyHandler(WPARAM wParam)
{
	switch (wParam)
	{
	case VK_UP:
	case VK_DOWN:
		legacy = !legacy;
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
