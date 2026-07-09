#include "HeaderControl.h"
#include "../include/PantherConsole.h"

using namespace Leet::LibPanther::Layout;

HeaderControl::HeaderControl(const std::wstring& text) : m_headerText(text)
{
	m_isFocusable = false;
	m_height = 3;
}

void HeaderControl::Draw(Leet::Panther2K::Util::Console* console)
{
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(CONSOLE_COLOR_LIGHTFG);

	console->SetPosition(1, 1);
	console->WriteLine(m_headerText.c_str());

	for (size_t i = 0; i < m_headerText.length() + 2; i++)
	{
		console->Write(L"\x2550");
	}
}