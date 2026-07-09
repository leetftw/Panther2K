#pragma once
#include "Control.h"
#include <string>
#include <vector>

namespace Leet::LibPanther::Layout
{
	class ListViewControl : public Control
	{
	private:
		std::vector<std::wstring> m_items;
		int m_selectedIndex = 0;
		int m_scrollIndex = 0;
		std::wstring m_header;
	public:
		ListViewControl(int displayHeight = -1);

		void Draw(Leet::Panther2K::Util::Console* console) override;
		bool HandleInput(KEY_EVENT_RECORD* key) override;

		void AddItem(const std::wstring& item);
		void SetHeader(const std::wstring& header);
		int GetSelectedIndex() const;
	};
}