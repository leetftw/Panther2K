#pragma once
#include "Control.h"
#include <string>

namespace Leet::LibPanther::Layout
{
	class StatusBarControl : public Control
	{
	private:
		std::wstring m_statusText;

	public:
		StatusBarControl(const std::wstring& text);
		void Draw(Leet::Panther2K::Util::Console* console) override;
		void SetText(const std::wstring& text);
	};
}