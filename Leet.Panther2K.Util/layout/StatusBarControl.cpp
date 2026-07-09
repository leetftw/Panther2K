#include "StatusBarControl.h"
#include "../include/PantherConsole.h"

using namespace Leet::LibPanther::Layout;

StatusBarControl::StatusBarControl(const std::wstring& text) : m_statusText(text)
{
	m_isFocusable = false;
	m_height = 0;
	m_fixedHeight = 2;
}

void StatusBarControl::SetText(const std::wstring& text)
{
	m_statusText = text;
}

void StatusBarControl::Draw(Leet::Panther2K::Util::Console* console)
{
	SIZE f = console->GetSize();

	console->SetBackgroundColor(CONSOLE_COLOR_FG);
	console->SetForegroundColor(CONSOLE_COLOR_DARKFG);

	console->SetPosition(0, f.cy - 1);
	for (int i = 0; i < f.cx; i++)
	{
		console->Write(L" ");
	}

	console->SetPosition(2, f.cy - 1);
	console->Write(m_statusText.c_str());
}