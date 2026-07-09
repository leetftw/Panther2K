#pragma once
#include "Control.h"
#include <string>

namespace Leet::LibPanther::Layout
{
	class HeaderControl : public Control
	{
	private:
		std::wstring m_headerText;

	public:
		HeaderControl(const std::wstring& text);
		void Draw(Leet::Panther2K::Util::Console* console) override;
	};
}