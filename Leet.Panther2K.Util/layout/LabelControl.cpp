#include "LabelControl.h"
#include "../include/PantherConsole.h"

using namespace Leet::LibPanther::Layout;

LabelControl::LabelControl(const std::wstring& labelText, bool light) : m_text(labelText), m_light(light)
{
	m_isFocusable = false;
	m_height = 1;
}

void LabelControl::Draw(Leet::Panther2K::Util::Console* console)
{
	console->SetPosition(m_x, m_y);
	console->SetBackgroundColor(CONSOLE_COLOR_BG);
	console->SetForegroundColor(m_light ? CONSOLE_COLOR_LIGHTFG : CONSOLE_COLOR_FG);
	console->Write(m_text.c_str());
}

void LabelControl::SetText(const std::wstring& newText)
{
	m_text = newText;
}