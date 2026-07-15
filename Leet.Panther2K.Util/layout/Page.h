#pragma once
#include <vector>
#include <memory>
#include <windows.h>
#include "Control.h"

namespace Leet::Panther2K::Util { class Console; }

namespace Leet::LibPanther::Layout
{
	typedef enum PageResult
	{
		PageExit,
		PageSuccess,
		PageContinue,
		PageGoBack,
	};

	class Page
	{
	private:
		Leet::Panther2K::Util::Console* m_console;
		std::vector<std::shared_ptr<Control>> m_controls;
		int m_focusedControlIndex = -1;
		int m_currentStackY = 0;
		int m_currentFixedHeight = 0;
	protected:
		virtual void InitPage();
		virtual void CreateLayout();
		virtual bool HandleKey(KEY_EVENT_RECORD* key);

		Leet::Panther2K::Util::Console* GetConsole() const { return m_console; }
		void AddControl(std::shared_ptr<Control> control);
		void Spacer(int height);
		void CycleFocus(bool forward = true);

		void RunPage();

	public:
		Page(Leet::Panther2K::Util::Console* cons);
		virtual ~Page() = default;

		void Show();

	};
}