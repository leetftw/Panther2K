#pragma once
#include "Control.h"
#include <string>

namespace Leet::LibPanther::Layout
{
	class LabelControl : public Control
	{
	private:
		std::wstring m_text;
		bool m_light;

	public:
		LabelControl(const std::wstring& labelText, bool light = false);
		void Draw(Leet::Panther2K::Util::Console* console) override;
		void SetText(const std::wstring& newText);
	};
}