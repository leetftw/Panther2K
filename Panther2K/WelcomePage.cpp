#include "WelcomePage.h"
#include "QuittingPage.h"
#include "ImageSelectionPage.h"

void WelcomePage::Init()
{
	statusText = L"  ENTER=Continue  R=Repair  F3=Quit";
}

void WelcomePage::Drawer()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);
	console->SetPosition(3, 4);
	console->Write(L"Welcome to Panther2K.");

	console->SetForegroundColor(CONSOLE_COLOR_FG);
	console->DrawTextLeft(L"The Setup portion of the Panther2K utility prepares Microsoft(R) Windows to run on your computer.", console->GetSize().cx - 6, 6);
	
	console->SetPosition(6, console->GetPosition().y + 2);
	console->Write(L"•");
	console->DrawTextLeft(L"To launch Setup, press ENTER", console->GetSize().cx - 18, console->GetPosition().y);

	console->SetPosition(6, console->GetPosition().y + 2);
	console->Write(L"•");
	console->DrawTextLeft(L"To repair a Windows installation, press R", console->GetSize().cx - 18, console->GetPosition().y);

	console->SetPosition(6, console->GetPosition().y + 2);
	console->Write(L"•");
	console->DrawTextLeft(L"To quit Panther2K without installing Windows, press F3", console->GetSize().cx - 18, console->GetPosition().y);
}

void WelcomePage::Redrawer()
{
}

PageResult WelcomePage::KeyHandler(WPARAM wParam)
{
	switch (wParam) 
	{
	case VK_RETURN:
		return PageContinue;
	case VK_ESCAPE:
	case VK_F3:
		AddPopup(new QuittingPage());
		break;
	}
	return PageSuccess;
}
