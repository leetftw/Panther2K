#include "BootPreparationPage.h"

#include <iostream>
#include "shlobj.h"
#include "MessageBoxPage.h"
#include "WinPartedDll.h"

void BootPreparationPage::Init()
{
	statusText = L"";
}

void BootPreparationPage::Drawer()
{
	SIZE cSize = console->GetSize();

	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->DrawTextCenter(L"Please wait while Panther2K prepares Windows Boot Manager to boot Windows on your computer. This should not take more than a minute to complete.", cSize.cx, 6);
}

void BootPreparationPage::Redrawer()
{
}

bool BootPreparationPage::KeyHandler(WPARAM wParam)
{
	return true;
}
