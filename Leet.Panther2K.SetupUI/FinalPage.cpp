#include "FinalPage.h"

void FinalPage::Init()
{
	statusText = L"  ENTER=Exit";
}

void FinalPage::Drawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);

	console->SetPosition(3, 4);
	console->Write(L"Setup completed successfully.");

	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->DrawTextLeft(L"Setup has finished installing Windows onto your computer. The Windows Out-Of-Box Experience (OOBE) will guide you through the rest of the installation.", console->GetSize().cx - 6, 6);

	console->SetPosition(3, console->GetPosition().y + 2);
	console->Write(L"To exit Panther2K, press the ENTER key.");
}

void FinalPage::Redrawer()
{

}

PageResult FinalPage::KeyHandler(WPARAM wParam)
{
	return wParam == VK_RETURN ? PageContinue : PageSuccess;
}
