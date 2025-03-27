#include "Page.h"

#include <PantherVersion.h>

Page::Page()
{
	console = 0;
	page = 0;
	statusText = 0;
	text = 0;
}

void Page::Initialize(Console* con)
{
	console = con;

	text = L"Panther2K " PANTHER_VERSION;

	Init();
	Draw();
}

void Page::Draw()
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->Clear();

	Drawer();
	Redraw(false);

	if (page != 0)
		page->Draw();

	//console->RedrawImmediately();
}

void Page::Redraw(bool redraw)
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_FG);

	console->SetPosition(1, 1);
	console->WriteLine(text);
	for (int i = 0; i < lstrlen(text) + 2; i++)
		console->Write(L"═");

	console->SetBackgroundColor(CONSOLE_COLOR_FG);
	console->SetForegroundColor(CONSOLE_COLOR_DARKFG);

	SIZE f = console->GetSize();
	console->SetPosition(0, f.cy - 1);
	for (int i = 0; i < f.cx; i++)
		console->Write(L" ");

	console->SetPosition(0, f.cy - 1);
	console->Write(statusText);

	Redrawer();
	//if (redraw)
	//	console->RedrawImmediately();
}

bool Page::HandleKey(WPARAM wParam)
{
	if (wParam == VK_F5)
		Draw();
	else if (page != 0)
		return page->HandleKey(wParam);
	else
		return KeyHandler(wParam);
}

void Page::DrawBox(int boxX, int boxY, int boxWidth, int boxHeight, bool useDouble)
{
	for (int i = 0; i < boxHeight; i++)
	{
		console->SetPosition(boxX, boxY + i);
		for (int j = 0; j < boxWidth; j++)
			console->Write(L" ");
	}

	console->SetPosition(boxX, boxY);
	console->Write(useDouble ? (L"╔") : (L"┌"));
	console->SetPosition(boxX + (boxWidth - 1), boxY);
	console->Write(useDouble ? (L"╗") : (L"┐"));
	console->SetPosition(boxX, boxY + (boxHeight - 1));
	console->Write(useDouble ? (L"╚") : (L"└"));
	console->SetPosition(boxX + (boxWidth - 1), boxY + (boxHeight - 1));
	console->Write(useDouble ? (L"╝") : (L"┘"));

	console->SetPosition(boxX + 1, boxY);
	for (int i = 0; i < boxWidth - 2; i++)
		console->Write(useDouble ? (L"═") : (L"─"));

	console->SetPosition(boxX + 1, boxY + boxHeight - 1);
	for (int i = 0; i < boxWidth - 2; i++)
		console->Write(useDouble ? (L"═") : (L"─"));

	for (int i = 0; i < boxHeight - 2; i++)
	{
		console->SetPosition(boxX, boxY + 1 + i);
		console->Write(useDouble ? (L"║") : (L"│"));
	}

	for (int i = 0; i < boxHeight - 2; i++)
	{
		console->SetPosition(boxX + boxWidth - 1, boxY + 1 + i);
		console->Write(useDouble ? (L"║") : (L"│"));
	}
}

void Page::AddPopup(PopupPage* popup)
{
	page = popup;
	page->Initialize(console, this);
	Draw();
}

void Page::RemovePopup()
{
	page = 0;
	Draw();
}

void Page::Init()
{
}

void Page::Drawer()
{
}

void Page::Redrawer()
{
}

bool Page::KeyHandler(WPARAM wParam)
{
	return true;
}
