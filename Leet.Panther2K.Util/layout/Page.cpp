#include "Page.h"

#include <iostream>
#include <stdexcept>

#include "../include/PantherConsole.h"
#include "SpacerControl.h"

using namespace Leet::LibPanther::Layout;

Page::Page(Leet::Panther2K::Util::Console* cons) : m_console(cons) {}

void Page::Show()
{
	InitPage();
	CreateLayout();
	RunPage();
}

void Page::InitPage() {}
void Page::CreateLayout() {}

void Page::AddControl(std::shared_ptr<Control> control)
{
	if (m_currentStackY == -1 && control->GetHeight() > 0)
		throw new std::runtime_error("Cannot add more controls to the page: no more space available.");

	if (control->GetY() == 0)
	{
		control->SetPosition(3, m_currentStackY);
		if (control->GetHeight() > 0)
			m_currentStackY += control->GetHeight();
		else if (control->GetHeight() < 0)
		{
			control->SetHeight(m_console->GetSize().cy - m_currentFixedHeight - m_currentStackY);
			m_currentStackY = -1;
		}

		m_currentFixedHeight += control->GetFixedHeight();

		std::cout << "Current stackY after adding control: " << m_currentStackY << std::endl;
	}

	m_controls.push_back(std::move(control));

	if (m_focusedControlIndex == -1 && m_controls.back()->IsFocusable())
	{
		m_focusedControlIndex = static_cast<int>(m_controls.size() - 1);
		m_controls.back()->SetFocused(true);
	}
}

void Page::Spacer(int height)
{
	AddControl(std::make_shared<SpacerControl>(height));
}

void Page::CycleFocus(bool forward)
{
	if (m_controls.empty() || m_focusedControlIndex == -1) return;

	m_controls[m_focusedControlIndex]->SetFocused(false);
	m_controls[m_focusedControlIndex]->Update(m_console);

	int startIndex = m_focusedControlIndex;
	int step = forward ? 1 : -1;
	int size = static_cast<int>(m_controls.size());

	do
	{
		m_focusedControlIndex = (m_focusedControlIndex + step + size) % size;
	} while (!m_controls[m_focusedControlIndex]->IsFocusable() && m_focusedControlIndex != startIndex);

	m_controls[m_focusedControlIndex]->SetFocused(true);
	m_controls[m_focusedControlIndex]->Update(m_console);
}

bool Page::HandleKey(KEY_EVENT_RECORD* key)
{
	if (key->wVirtualKeyCode == VK_ESCAPE)
	{
		return false;
	}
	return true;
}

void Page::RunPage()
{
	if (!m_console) return;

	m_console->SetBackgroundColor(CONSOLE_COLOR_BG);
	m_console->SetForegroundColor(CONSOLE_COLOR_FG);
	m_console->Clear();

	for (auto& ctrl : m_controls)
	{
		m_console->SetBackgroundColor(CONSOLE_COLOR_BG);
		m_console->SetForegroundColor(CONSOLE_COLOR_FG);
		ctrl->Draw(m_console);
	}

	while (KEY_EVENT_RECORD* key = m_console->Read())
	{
		if (key->bKeyDown)
		{
			bool handledByControl = false;

			if (key->wVirtualKeyCode == VK_TAB)
			{
				CycleFocus(true);
				handledByControl = true;
			}

			else if (m_focusedControlIndex >= 0 && m_focusedControlIndex < m_controls.size())
			{
				handledByControl = m_controls[m_focusedControlIndex]->HandleInput(key);
			}

			if (!handledByControl)
			{
				if (!HandleKey(key))
				{
					break;
				}
			}
		}

		for (auto& ctrl : m_controls)
		{
			m_console->SetBackgroundColor(CONSOLE_COLOR_BG);
			m_console->SetForegroundColor(CONSOLE_COLOR_FG);
			ctrl->Draw(m_console);
		}
	}
}