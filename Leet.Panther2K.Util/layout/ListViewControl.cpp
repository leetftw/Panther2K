#include "ListViewControl.h"
#include "../include/PantherConsole.h"

using namespace Leet::LibPanther::Layout;

ListViewControl::ListViewControl(int displayHeight)
{
	m_isFocusable = true;
	m_height = displayHeight;
}

void ListViewControl::AddItem(const std::wstring& item)
{
	m_items.push_back(item);
}

void ListViewControl::ClearItems()
{
	m_items.clear();
	m_selectedIndex = 0;
	m_scrollIndex = 0;
}

void ListViewControl::SetHeader(const std::wstring& header)
{
	m_header = header;
}

int ListViewControl::GetSelectedIndex() const
{
	return m_selectedIndex;
}

void ListViewControl::Draw(Leet::Panther2K::Util::Console* console)
{
	int itemsToDraw = min(m_height, (int)m_items.size() - m_scrollIndex);

	SIZE consoleSize = console->GetSize();
	console->DrawBox(m_x, m_y, consoleSize.cx - 6, m_height, true);

	console->SetPosition(6, m_y + 1);
	console->Write(m_header.c_str());

	for (int i = 0; i < itemsToDraw; i++)
	{
		console->SetPosition(m_x + 3, m_y + (m_header.empty() ? 1 : 2) + i);

		int itemIndex = i + m_scrollIndex;

		if (itemIndex < m_items.size())
		{
			if (itemIndex == m_selectedIndex && m_isFocused)
			{
				console->SetBackgroundColor(CONSOLE_COLOR_FG);
				console->SetForegroundColor(CONSOLE_COLOR_BG);
			}
			else
			{
				console->SetBackgroundColor(CONSOLE_COLOR_BG);
				console->SetForegroundColor(CONSOLE_COLOR_FG);
			}

			console->Write(m_items[itemIndex].c_str());
		}
	}
}

bool ListViewControl::HandleInput(KEY_EVENT_RECORD* key)
{
	if (m_items.empty()) return false;

	if (key->wVirtualKeyCode == VK_DOWN)
	{
		if (m_selectedIndex < m_items.size() - 1)
		{
			m_selectedIndex++;
			if (m_selectedIndex >= m_scrollIndex + m_height)
			{
				m_scrollIndex++;
			}
		}
		return true;
	}
	else if (key->wVirtualKeyCode == VK_UP)
	{
		if (m_selectedIndex > 0)
		{
			m_selectedIndex--;
			if (m_selectedIndex < m_scrollIndex)
			{
				m_scrollIndex--;
			}
		}
		return true;
	}

	return false;
}